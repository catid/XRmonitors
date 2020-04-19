// Copyright 2019 Augmented Perception Corporation

#include "stdafx.h"
#include "ShaderCameraRenderer.h"
#include "../../Common/DirectXHelper.h"
#include "../MonitorTools.hpp"
using namespace xrm;

#include "CameraPixelShader.inl"
#include "CameraVertexShader.inl"
#include "CameraVPRTVertexShader.inl"

#include "core_logger.hpp"
#include "core_string.hpp"
using namespace core;

static logger::Channel Logger("Main");


namespace RenderCameras {

using namespace DirectX;
using namespace winrt::Windows::Foundation::Numerics;
using namespace winrt::Windows::UI::Input::Spatial;

// Loads vertex and pixel shaders from files and instantiates the cube geometry.
ShaderCameraRenderer::ShaderCameraRenderer(
	const xrm::HeadsetCameraCalibration& calibration,
	std::shared_ptr<DX::DeviceResources> const& deviceResources)
	: m_deviceResources(deviceResources)
{
    CreateDeviceDependentResources(calibration);
}


//------------------------------------------------------------------------------
// RenderCamera

bool ShaderCameraRenderer::CreateTextures(unsigned width, unsigned height)
{
	// If texture must be recreated:
	if (!RenderTexture ||
		width != RenderTextureDesc.Width ||
		height != RenderTextureDesc.Height)
	{
		Logger.Info("Recreating RenderTexture ( ", width, "x", height, " )");

		RenderTexture.Reset();

		memset(&RenderTextureDesc, 0, sizeof(RenderTextureDesc));
		RenderTextureDesc.Format = DXGI_FORMAT_R8_UNORM;
		RenderTextureDesc.MipLevels = 1;
		RenderTextureDesc.ArraySize = 1;
		RenderTextureDesc.SampleDesc.Count = 1;
		RenderTextureDesc.Width = width;
		RenderTextureDesc.Height = height;
		RenderTextureDesc.Usage = D3D11_USAGE_DEFAULT;
		RenderTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		HRESULT hr = m_deviceResources->GetD3DDevice()->CreateTexture2D(
			&RenderTextureDesc,
			nullptr,
			&RenderTexture);
		if (FAILED(hr)) {
			Logger.Error("CreateTexture2D failed: ", HresultString(hr));
			return false;
		}
	}

	// If texture must be recreated:
	if (!StagingTexture ||
		RenderTextureDesc.Width != StagingTextureDesc.Width ||
		RenderTextureDesc.Height != StagingTextureDesc.Height ||
		RenderTextureDesc.Format != StagingTextureDesc.Format)
	{
		Logger.Info("Recreating StagingTexture ( ", width, "x", height, " )");

		StagingTexture.Reset();

		StagingTextureDesc = RenderTextureDesc;
		StagingTextureDesc.BindFlags = 0;
		StagingTextureDesc.Usage = D3D11_USAGE_STAGING;
		StagingTextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		HRESULT hr = m_deviceResources->GetD3DDevice()->CreateTexture2D(
			&StagingTextureDesc,
			nullptr,
			&StagingTexture);
		if (FAILED(hr)) {
			Logger.Error("CreateTexture2D failed: ", HresultString(hr));
			return false;
		}
	}

	return true;
}

bool ShaderCameraRenderer::UpdateTexture(
	const uint8_t* image,
	unsigned width,
	unsigned height)
{
	if (!CreateTextures(width, height)) {
		return false;
	}

	const auto& context = m_deviceResources->GetD3DDeviceContext();

	const UINT subresource_index = D3D11CalcSubresource(0, 0, 0);
	D3D11_MAPPED_SUBRESOURCE subresource{};
	HRESULT hr = context->Map(
		StagingTexture.Get(),
		subresource_index,
		D3D11_MAP_WRITE,
		0, // flags
		&subresource);
	if (FAILED(hr) || !subresource.pData) {
		Logger.Error("Map failed: ", HresultString(hr));
		return false;
	}

	uint8_t* dest = reinterpret_cast<uint8_t*>( subresource.pData );
	const unsigned pitch = subresource.RowPitch;

	// HACK: Remove 32 byte tags from the image
	const unsigned offset = 23264 + 1312 - 32;
	unsigned next_tag_offset = offset - 1312 + 32;

	unsigned bright_sum_count = 0;
	int bright_sum = 0;

	for (unsigned i = 0; i < height; ++i) {
		if (next_tag_offset < width) {
			memcpy(dest, image, next_tag_offset);
			image += 32;
			memcpy(dest + next_tag_offset, image + next_tag_offset, width - next_tag_offset);
			next_tag_offset = offset - (width - next_tag_offset);
		}
		else {
			memcpy(dest, image, width);
			next_tag_offset -= width;

			for (unsigned j = 0; j < width; j += 32) {
				bright_sum += image[j];
			}
			++bright_sum_count;
		}
		image += width;
		dest += pitch;
	}

	context->Unmap(
		StagingTexture.Get(),
		subresource_index);

	int bright_avg = bright_sum / bright_sum_count * 20;
	if (bright_avg < LastAcceptedBright / 4 && FrameSkipped < 7) {
		return false;
	}
	LastAcceptedBright = bright_avg;
	FrameSkipped = 0;


#if 0
	// Reduce framerate
	static uint64_t last_t = 0;
	uint64_t now = GetTimeUsec();
	if (now - last_t < 1000000) {
		return false;
	}
	last_t = now;
#endif


	context->CopyResource(RenderTexture.Get(), StagingTexture.Get());

	return true;
}

bool ShaderCameraRenderer::RenderCamera(
	const xrm::HeadsetCameraCalibration& calibration,
	SpatialLocation const& headset_location,
	DX::CameraResources* camera_resources)
{
	// Loading is asynchronous. Resources must be created before drawing can occur.
	if (!m_loadingComplete) {
		return true; // Expected error
	}

	if (!headset_location) {
		return true; // Expected error
	}

	const XMMATRIX modelOrientation = XMMatrixRotationQuaternion(XMLoadQuaternion(
		&headset_location.Orientation()));

	float3 center = headset_location.Position();
	center.z -= 10.f;
	const XMMATRIX modelTranslation = XMMatrixTranslationFromVector(XMLoadFloat3(
		&center));

#if 0
	// (1) Pick scale first - 10 makes everything seem small
	// 14 seems normal sized
	static constexpr float scale = 14.f;
#else
	const float scale = calibration.Scale;
#endif
	const XMMATRIX modelScale = XMMatrixScaling(scale, scale, scale);

#if 0
	static float offset_x = 0.253f;
	static float offset_y = -0.153f;
	// (2) Then adjust the translation offsets so that eyes can fuse the top of real monitors at eye-level
	if ((::GetAsyncKeyState(VK_LSHIFT) >> 15) == 0) {
		offset_y -= 0.001f;
	}
	if ((::GetAsyncKeyState(VK_RSHIFT) >> 15) == 0) {
		offset_y += 0.001f;
	}
	if ((::GetAsyncKeyState(VK_LCONTROL) >> 15) == 0) {
		offset_x -= 0.001f;
	}
	if ((::GetAsyncKeyState(VK_RCONTROL) >> 15) == 0) {
		offset_x += 0.001f;
	}
#else
	const float offset_x = calibration.OffsetX;
	const float offset_y = calibration.OffsetY;
#endif
	float3 left_translate;
	left_translate.x = -offset_x;
	left_translate.y = offset_y;
	left_translate.z = 0.f;
	const XMMATRIX leftTranslate = XMMatrixTranslationFromVector(XMLoadFloat3(
		&left_translate));
	float3 right_translate;
	right_translate.x = offset_x;
	right_translate.y = offset_y;
	right_translate.z = 0.f;
	const XMMATRIX rightTranslate = XMMatrixTranslationFromVector(XMLoadFloat3(
		&right_translate));

#if 0
	static float eye_cant_x = -0.381f;
	static float eye_cant_y = -0.195f;
	static float eye_cant_z = 0.07f;

	// (3) Then adjust the cant angles so that vertical bars in the room fuse at top and bottom
	if ((::GetAsyncKeyState(VK_LSHIFT) >> 15) == 0) {
		eye_cant_x -= 0.001f;
	}
	if ((::GetAsyncKeyState(VK_RSHIFT) >> 15) == 0) {
		eye_cant_x += 0.001f;
	}
	if ((::GetAsyncKeyState(VK_LCONTROL) >> 15) == 0) {
		eye_cant_y -= 0.001f;
	}
	if ((::GetAsyncKeyState(VK_RCONTROL) >> 15) == 0) {
		eye_cant_y += 0.001f;
	}
	if ((::GetAsyncKeyState(VK_F7) >> 15) == 0) {
		eye_cant_z -= 0.01f;
	}
	if ((::GetAsyncKeyState(VK_F8) >> 15) == 0) {
		eye_cant_z += 0.01f;
	}
#else
	const float eye_cant_x = calibration.EyeCantX;
	const float eye_cant_y = calibration.EyeCantY;
	const float eye_cant_z = calibration.EyeCantZ;
#endif
	const XMMATRIX leftRotate = XMMatrixRotationRollPitchYaw(eye_cant_y, -eye_cant_x, -eye_cant_z);
	const XMMATRIX rightRotate = XMMatrixRotationRollPitchYaw(eye_cant_y, eye_cant_x, eye_cant_z);

	const XMMATRIX leftTransform =
		XMMatrixMultiply(
			XMMatrixMultiply(
				XMMatrixMultiply(
					XMMatrixMultiply(
						leftRotate,
						leftTranslate
					),
					modelScale
				),
				modelTranslation
			),
			modelOrientation
		);
	const XMMATRIX rightTransform =
		XMMatrixMultiply(
			XMMatrixMultiply(
				XMMatrixMultiply(
					XMMatrixMultiply(
						rightRotate,
						rightTranslate
					),
					modelScale
				),
				modelTranslation
			),
			modelOrientation
		);

	XMStoreFloat4x4(&m_modelConstantBufferData.model[0], XMMatrixTranspose(leftTransform));
	XMStoreFloat4x4(&m_modelConstantBufferData.model[1], XMMatrixTranspose(rightTransform));

#if 0
	const XMMATRIX monitorOrientation = XMMatrixRotationQuaternion(XMLoadQuaternion(
		&headset_location.Orientation()));

	float3 center = headset_location.Position();
	center.z -= 2.f;

	const XMMATRIX monitorTranslation = XMMatrixTranslationFromVector(XMLoadFloat3(
		&center));

	const XMMATRIX monitorScale = XMMatrixScaling(Scale, Scale, Scale);

	const XMMATRIX ModelTransform = XMMatrixMultiply(monitorScale, monitorTranslation);

	// The view and projection matrices are provided by the system; they are associated
	// with holographic cameras, and updated on a per-camera basis.
	// Here, we provide the model transform for the sample hologram. The model transform
	// matrix is transposed to prepare it for the shader.
	XMStoreFloat4x4(&m_modelConstantBufferData.model, XMMatrixTranspose(ModelTransform));

	const float eye_cant = 0.f;
	const XMMATRIX leftEyeRotate = XMMatrixRotationY(-eye_cant);
	const XMMATRIX rightEyeRotate = XMMatrixRotationY(eye_cant);

	const float eye_sep = 0.f;
	const XMMATRIX leftEyeTrans = XMMatrixTranslationFromVector(XMLoadFloat3(&float3(-eye_sep, 0.f, 0.f)));
	const XMMATRIX rightEyeTrans = XMMatrixTranslationFromVector(XMLoadFloat3(&float3(eye_sep, 0.f, 0.f)));

	const XMMATRIX leftEyeProj = XMLoadFloat4x4(&camera_resources->LeftTransform);
	const XMMATRIX rightEyeProj = XMLoadFloat4x4(&camera_resources->RightTransform);

	XMStoreFloat4x4(&m_modelConstantBufferData.cameraViewProjection[0],
		XMMatrixMultiply(
			XMMatrixMultiply(
				XMMatrixMultiply(
					leftEyeProj,
					leftEyeRotate),
				(leftEyeTrans)),
			XMMatrixTranspose(monitorOrientation)));
	XMStoreFloat4x4(&m_modelConstantBufferData.cameraViewProjection[1],
		XMMatrixMultiply(
			XMMatrixMultiply(
				XMMatrixMultiply(
					rightEyeProj,
					rightEyeRotate),
				(rightEyeTrans)),
			XMMatrixTranspose(monitorOrientation)));
#endif

#if 0
	static float shift_x = 0.3f;
	static float shift_y = -0.3f;
	static float scale_x = 1.f;
	static float scale_y = 1.f;
#if 0
	if ((::GetAsyncKeyState(VK_LSHIFT) >> 15) == 0) {
		shift_x -= 0.001f;
	}
	if ((::GetAsyncKeyState(VK_RSHIFT) >> 15) == 0) {
		shift_x += 0.001f;
	}
	if ((::GetAsyncKeyState(VK_LCONTROL) >> 15) == 0) {
		shift_y -= 0.001f;
	}
	if ((::GetAsyncKeyState(VK_RCONTROL) >> 15) == 0) {
		shift_y += 0.001f;
	}
	Logger.Info("Shift: ", shift_x, ", ", shift_y);
#endif
#if 1
	if ((::GetAsyncKeyState(VK_LSHIFT) >> 15) == 0) {
		shift_x -= 0.001f;
	}
	if ((::GetAsyncKeyState(VK_RSHIFT) >> 15) == 0) {
		shift_x += 0.001f;
	}
	if ((::GetAsyncKeyState(VK_LCONTROL) >> 15) == 0) {
		scale_x -= 0.001f;
	}
	if ((::GetAsyncKeyState(VK_RCONTROL) >> 15) == 0) {
		scale_x += 0.001f;
	}
	Logger.Info("Scale: ", scale_x);
#endif

	m_modelConstantBufferData.cameraViewProjection[0]._11 = -shift_x;
	m_modelConstantBufferData.cameraViewProjection[1]._11 = shift_x;
	m_modelConstantBufferData.cameraViewProjection[0]._12 = shift_y;
	m_modelConstantBufferData.cameraViewProjection[1]._12 = shift_y;
	m_modelConstantBufferData.cameraViewProjection[0]._13 = scale_x;
	m_modelConstantBufferData.cameraViewProjection[1]._13 = scale_x;
	m_modelConstantBufferData.cameraViewProjection[0]._14 = scale_y;
	m_modelConstantBufferData.cameraViewProjection[1]._14 = scale_y;
#endif

#if 0
	static float eye_cant_x = 0.63f;
	static float eye_cant_y = 0.37f;
	const XMMATRIX leftRotate = XMMatrixRotationRollPitchYaw(-eye_cant_x, eye_cant_y, 0.f);
	const XMMATRIX rightRotate = XMMatrixRotationRollPitchYaw(eye_cant_x, eye_cant_y, 0.f);

	if ((::GetAsyncKeyState(VK_LSHIFT) >> 15) == 0) {
		eye_cant_x -= 0.01f;
	}
	if ((::GetAsyncKeyState(VK_RSHIFT) >> 15) == 0) {
		eye_cant_x += 0.01f;
	}
	if ((::GetAsyncKeyState(VK_LCONTROL) >> 15) == 0) {
		eye_cant_y -= 0.01f;
	}
	if ((::GetAsyncKeyState(VK_RCONTROL) >> 15) == 0) {
		eye_cant_y += 0.01f;
	}

	static float translation_x = 0.3f;
	static float translation_y = -0.26f;
	const XMMATRIX leftEyeTrans = XMMatrixTranslationFromVector(XMLoadFloat3(&float3(-translation_x, translation_y, 0.f)));
	const XMMATRIX rightEyeTrans = XMMatrixTranslationFromVector(XMLoadFloat3(&float3(translation_x, translation_y, 0.f)));

	XMStoreFloat4x4(&m_modelConstantBufferData.cameraViewProjection[0],
		XMMatrixTranspose(
			XMMatrixMultiply(leftEyeTrans, leftRotate)
		));

	XMStoreFloat4x4(&m_modelConstantBufferData.cameraViewProjection[1],
		XMMatrixTranspose(
			XMMatrixMultiply(rightEyeTrans, rightRotate)
		));
#endif

	// Use the D3D device context to update Direct3D device-based resources.
	const auto context = m_deviceResources->GetD3DDeviceContext();

	// Update the model transform buffer for the hologram.
	context->UpdateSubresource(
		m_modelConstantBuffer.Get(),
		0,
		nullptr,
		&m_modelConstantBufferData,
		0,
		0
	);

	// Each vertex is one instance of the VertexPositionColor struct.
	const UINT stride = sizeof(VertexPosition);
	const UINT offset = 0;
	context->IASetVertexBuffers(
		0,
		1,
		m_vertexBuffer.GetAddressOf(),
		&stride,
		&offset
	);
	context->IASetIndexBuffer(
		m_indexBuffer.Get(),
		DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
		0
	);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetInputLayout(m_inputLayout.Get());

	// Attach the vertex shader.
	context->VSSetShader(
		m_vertexShader.Get(),
		nullptr,
		0
	);
	// Apply the model constant buffer to the vertex shader.
	context->VSSetConstantBuffers(
		0,
		1,
		m_modelConstantBuffer.GetAddressOf()
	);

	if (!m_usingVprtShaders)
	{
		// On devices that do not support the D3D11_FEATURE_D3D11_OPTIONS3::
		// VPAndRTArrayIndexFromAnyShaderFeedingRasterizer optional feature,
		// a pass-through geometry shader is used to set the render target 
		// array index.
		context->GSSetShader(
			m_geometryShader.Get(),
			nullptr,
			0
		);
	}

	// Attach the pixel shader.
	context->PSSetShader(
		m_pixelShader.Get(),
		nullptr,
		0
	);

	if (RenderTexture)
	{
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shaderResourceView;
		try {
			winrt::check_hresult(m_deviceResources->GetD3DDevice()->
				CreateShaderResourceView(
					RenderTexture.Get(),
					nullptr,
					&shaderResourceView));
		}
		catch (winrt::hresult_error & err) {
			Logger.Error("CreateShaderResourceView failed: ", err.message().c_str());
			return false;
		}

		context->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());
		context->PSSetShaderResources(0, 1, shaderResourceView.GetAddressOf());
	}
	else {
		ID3D11SamplerState* sampler_list = nullptr;
		ID3D11ShaderResourceView* srv_list = nullptr;

		context->PSSetShaderResources(0, 0, &srv_list);
		context->PSSetSamplers(0, 0, &sampler_list);
	}

	// Draw the objects.
	context->DrawIndexedInstanced(
		m_indexCount,   // Index count per instance.
		2,              // Instance count.
		0,              // Start index location.
		0,              // Base vertex location.
		0               // Start instance location.
	);

	return true;
}


//------------------------------------------------------------------------------
// Setup

void ShaderCameraRenderer::CreateDeviceDependentResources(
	const xrm::HeadsetCameraCalibration& calibration)
{
    m_usingVprtShaders = m_deviceResources->GetDeviceSupportsVprt();

    // On devices that do support the D3D11_FEATURE_D3D11_OPTIONS3::
    // VPAndRTArrayIndexFromAnyShaderFeedingRasterizer optional feature
    // we can avoid using a pass-through geometry shader to set the render
    // target array index, thus avoiding any overhead that would be 
    // incurred by setting the geometry shader stage.
    std::wstring vertexShaderFileName = m_usingVprtShaders ?
		L"CameraVprtVertexShader.cso" : L"CameraVertexShader.cso";

	const BYTE* shader_code = m_usingVprtShaders ? g_CameraVPRTVertexShader : g_CameraVertexShader;
	const size_t shader_bytes = m_usingVprtShaders ? sizeof(g_CameraVPRTVertexShader) : sizeof(g_CameraVertexShader);

    winrt::check_hresult(
        m_deviceResources->GetD3DDevice()->CreateVertexShader(
			shader_code,
			shader_bytes,
            nullptr,
            &m_vertexShader
        ));

    constexpr std::array<D3D11_INPUT_ELEMENT_DESC, 2> vertexDesc =
        { {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		} };

    winrt::check_hresult(
        m_deviceResources->GetD3DDevice()->CreateInputLayout(
            vertexDesc.data(),
			static_cast<UINT>(vertexDesc.size()),
			shader_code,
			static_cast<UINT>(shader_bytes),
			&m_inputLayout
        ));

    // After the pixel shader file is loaded, create the shader and constant buffer.
    winrt::check_hresult(
        m_deviceResources->GetD3DDevice()->CreatePixelShader(
            g_CameraPixelShader,
            sizeof(g_CameraPixelShader),
            nullptr,
            &m_pixelShader
        ));

    const CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
    winrt::check_hresult(
        m_deviceResources->GetD3DDevice()->CreateBuffer(
            &constantBufferDesc,
            nullptr,
            &m_modelConstantBuffer
        ));

#if 0
	// FIXME
    if (!m_usingVprtShaders)
    {
        // Load the pass-through geometry shader.
        std::vector<byte> geometryShaderFileData = co_await DX::ReadDataAsync(L"CameraGeometryShader.cso");

        // After the pass-through geometry shader file is loaded, create the shader.
        winrt::check_hresult(
            m_deviceResources->GetD3DDevice()->CreateGeometryShader(
                geometryShaderFileData.data(),
                geometryShaderFileData.size(),
                nullptr,
                &m_geometryShader
            ));
    }
#endif

#if 0
    // Load mesh vertices. Each vertex has a position and a color.
    // Note that the cube size has changed from the default DirectX app
    // template. Windows Holographic is scaled in meters, so to draw the
    // cube at a comfortable size we made the cube width 0.2 m (20 cm).
    static const std::array<VertexPosition, 8> cubeVertices =
        { {
            { XMFLOAT3(-0.5f, -0.5f, 0.f), XMFLOAT2(0.0f, 0.0f) },
            { XMFLOAT3(-0.5f, -0.5f, 0.f), XMFLOAT2(0.0f, 1.0f) }, // 1
            { XMFLOAT3(-0.5f,  0.5f, 0.f), XMFLOAT2(0.0f, 1.0f) },
            { XMFLOAT3(-0.5f,  0.5f, 0.f), XMFLOAT2(0.0f, 0.0f) }, // 3
            { XMFLOAT3( 0.5f, -0.5f, 0.f), XMFLOAT2(1.0f, 0.0f) },
            { XMFLOAT3( 0.5f, -0.5f, 0.f), XMFLOAT2(1.0f, 1.0f) }, // 5
            { XMFLOAT3( 0.5f,  0.5f, 0.f), XMFLOAT2(1.0f, 1.0f) },
            { XMFLOAT3( 0.5f,  0.5f, 0.f), XMFLOAT2(1.0f, 0.0f) }, // 7
        } };

    D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
    vertexBufferData.pSysMem = cubeVertices.data();
    vertexBufferData.SysMemPitch = 0;
    vertexBufferData.SysMemSlicePitch = 0;
    const CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(VertexPosition) * static_cast<UINT>(cubeVertices.size()), D3D11_BIND_VERTEX_BUFFER);
    winrt::check_hresult(
        m_deviceResources->GetD3DDevice()->CreateBuffer(
            &vertexBufferDesc,
            &vertexBufferData,
            &m_vertexBuffer
        ));

    // Load mesh indices. Each trio of indices represents
    // a triangle to be rendered on the screen.
    // For example: 2,1,0 means that the vertices with indexes
    // 2, 1, and 0 from the vertex buffer compose the
    // first triangle of this mesh.
    // Note that the winding order is clockwise by default.
    constexpr std::array<unsigned short, 6> cubeIndices =
        { {
#if 0
            2,1,0, // -x
            2,3,1,
#endif
#if 0
            6,4,5, // +x
            6,5,7,
#endif
#if 0
            0,1,5, // -y
            0,5,4,
#endif
#if 0
            2,6,7, // +y
            2,7,3,
#endif
#if 0
            0,4,6, // -z
            0,6,2,
#endif
#if 1
            1,3,7, // +z
            1,7,5,
#endif
        } };

    m_indexCount = static_cast<unsigned int>(cubeIndices.size());

    D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
    indexBufferData.pSysMem = cubeIndices.data();
    indexBufferData.SysMemPitch = 0;
    indexBufferData.SysMemSlicePitch = 0;
    CD3D11_BUFFER_DESC indexBufferDesc(sizeof(unsigned short) * static_cast<UINT>(cubeIndices.size()), D3D11_BIND_INDEX_BUFFER);
    winrt::check_hresult(
        m_deviceResources->GetD3DDevice()->CreateBuffer(
            &indexBufferDesc,
            &indexBufferData,
            &m_indexBuffer
        ));
#else

	GenerateMesh(calibration.K1);

	D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
	vertexBufferData.pSysMem = Vertices.data();
	vertexBufferData.SysMemPitch = 0;
	vertexBufferData.SysMemSlicePitch = 0;
	const CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(VertexPosition)* static_cast<UINT>(Vertices.size()), D3D11_BIND_VERTEX_BUFFER);
	winrt::check_hresult(
		m_deviceResources->GetD3DDevice()->CreateBuffer(
			&vertexBufferDesc,
			&vertexBufferData,
			&m_vertexBuffer
		));

	m_indexCount = static_cast<unsigned int>(Indices.size());

	D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
	indexBufferData.pSysMem = Indices.data();
	indexBufferData.SysMemPitch = 0;
	indexBufferData.SysMemSlicePitch = 0;
	CD3D11_BUFFER_DESC indexBufferDesc(sizeof(unsigned short)* static_cast<UINT>(Indices.size()), D3D11_BIND_INDEX_BUFFER);
	winrt::check_hresult(
		m_deviceResources->GetD3DDevice()->CreateBuffer(
			&indexBufferDesc,
			&indexBufferData,
			&m_indexBuffer
		));

#endif


	// Best so far: D3D11_FILTER_ANISOTROPIC
	// Feels like both my eyes see the same thing

	// D3D11_FILTER_MIN_MAG_MIP_POINT
	// D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR
	// Head-locked: Left/right eye disparity with text.
	// Not head-locked: Text is jumbled up with any rotation.

	// D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT
	// Loses bits of text

	// D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR
	// Loses the - in the E

	// D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR
	// Head-locked: Little wobble in the _s
	// Free: Little wobble

	// D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT
	// Very small wobble

	// D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT
	// Very very small wobble
	// Very similar to D3D11_FILTER_MIN_MAG_MIP_LINEAR

	// Create a texture sampler state description.
	D3D11_SAMPLER_DESC samplerDesc{};
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = D3D11_DEFAULT_MAX_ANISOTROPY;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS; // FIXME: Depth buffer...
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	// Create the texture sampler state.
	winrt::check_hresult(m_deviceResources->GetD3DDevice()->CreateSamplerState(
		&samplerDesc,
		&m_samplerState));

	if (!m_samplerState) {
		Logger.Error("ERROR");
	}

    // Once the cube is loaded, the object is ready to be rendered.
    m_loadingComplete = true;
};

void ShaderCameraRenderer::WarpVertex(VertexPosition& vp, float k1)
{
	// We assume that 0 is already the image center
	float x = vp.pos.x;
	float y = vp.pos.y;

	const float r_sqr = x * x + y * y;
	const float k_inv = 1.f / (1.f + k1 * r_sqr);

	x *= k_inv;
	y *= k_inv;

	vp.pos.x = x;
	vp.pos.y = y;
}

void ShaderCameraRenderer::GenerateMesh(float k1)
{
	Vertices.clear();
	Indices.clear();

	const float aspect = 640.f / 480.f;

	const unsigned width = 50;
	const unsigned pitch = width + 1;
	const unsigned height = 50;
	const float dx = 1.f / width;
	const float dy = 1.f / height;

	float v_y = -0.5f;
	float v = 1.f;
	unsigned lr_index = 0;

	for (unsigned y = 0; y <= height; ++y)
	{
		float v_x = -0.5f;
		float u = 0.f;

		for (unsigned x = 0; x <= width; ++x)
		{
			VertexPosition vp;
			vp.pos.x = v_x * aspect;
			vp.pos.y = v_y;
			vp.pos.z = 0.f;
			vp.tex.x = u;
			vp.tex.y = v;

			WarpVertex(vp, k1);

			Vertices.push_back(vp);

			if (x > 0 && y > 0) {
				Indices.push_back(lr_index - pitch);
				Indices.push_back(lr_index - pitch - 1);
				Indices.push_back(lr_index - 1);

				Indices.push_back(lr_index);
				Indices.push_back(lr_index - pitch);
				Indices.push_back(lr_index - 1);
			}

			v_x += dx;
			u += dx;
			++lr_index;
		}

		v_y += dy;
		v -= dy;
	}
}

void ShaderCameraRenderer::ReleaseDeviceDependentResources()
{
    m_loadingComplete = false;
    m_usingVprtShaders = false;
    m_vertexShader.Reset();
    m_inputLayout.Reset();
    m_pixelShader.Reset();
	m_samplerState.Reset();
    m_geometryShader.Reset();
    m_modelConstantBuffer.Reset();
    m_vertexBuffer.Reset();
    m_indexBuffer.Reset();
}


} // namespace RenderCameras
