// Copyright 2019 Augmented Perception Corporation

#include "stdafx.h"
#include "ShaderMonitorRenderer.h"
#include "../../Common/DirectXHelper.h"

#include "MonitorPixelShader.inl"
#include "MonitorVertexShader.inl"
#include "MonitorVPRTVertexShader.inl"

#include "core_logger.hpp"
#include "core_string.hpp"
using namespace core;

static logger::Channel Logger("Main");


namespace RenderMonitors {

using namespace DirectX;
using namespace winrt::Windows::Foundation::Numerics;
using namespace winrt::Windows::UI::Input::Spatial;
using namespace Microsoft::WRL;
using namespace winrt::Windows::Perception::Spatial;

// Loads vertex and pixel shaders from files and instantiates the cube geometry.
ShaderMonitorRenderer::ShaderMonitorRenderer(std::shared_ptr<DX::DeviceResources> const& deviceResources) :
    m_deviceResources(deviceResources)
{
    CreateDeviceDependentResources();
}


//------------------------------------------------------------------------------
// RenderSphericalMonitor

bool ShaderMonitorRenderer::RenderSphericalMonitor(
	xrm::MonitorRenderInfo& render_info,
	ComPtr<ID3D11Texture2D>& texture)
{
	// Loading is asynchronous. Resources must be created before drawing can occur.
	if (!m_loadingComplete) {
		return true; // Expected error
	}

	// The view and projection matrices are provided by the system; they are associated
	// with holographic cameras, and updated on a per-camera basis.
	// Here, we provide the model transform for the sample hologram. The model transform
	// matrix is transposed to prepare it for the shader.
	XMStoreFloat4x4(&m_modelConstantBufferData.model, XMMatrixTranspose(render_info.SphereMonitor.ModelTransform));

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
		render_info.SphereMonitor.VertexBuffer.GetAddressOf(),
		&stride,
		&offset
	);
	context->IASetIndexBuffer(
		render_info.SphereMonitor.IndexBuffer.Get(),
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

	if (texture)
	{
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shaderResourceView;
		try {
			winrt::check_hresult(m_deviceResources->GetD3DDevice()->
				CreateShaderResourceView(
					texture.Get(),
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
		render_info.SphereMonitor.IndexCount,   // Index count per instance.
		2,              // Instance count.
		0,              // Start index location.
		0,              // Base vertex location.
		0               // Start instance location.
	);

	return true;
}


//------------------------------------------------------------------------------
// Setup

void ShaderMonitorRenderer::CreateDeviceDependentResources()
{
    m_usingVprtShaders = m_deviceResources->GetDeviceSupportsVprt();

    // On devices that do support the D3D11_FEATURE_D3D11_OPTIONS3::
    // VPAndRTArrayIndexFromAnyShaderFeedingRasterizer optional feature
    // we can avoid using a pass-through geometry shader to set the render
    // target array index, thus avoiding any overhead that would be 
    // incurred by setting the geometry shader stage.

	const BYTE* shader_code = m_usingVprtShaders ? g_MonitorVPRTVertexShader : g_MonitorVertexShader;
	const size_t shader_bytes = m_usingVprtShaders ? sizeof(g_MonitorVPRTVertexShader) : sizeof(g_MonitorVertexShader);

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

    winrt::check_hresult(
        m_deviceResources->GetD3DDevice()->CreatePixelShader(
            g_MonitorPixelShader,
			sizeof(g_MonitorPixelShader),
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
        std::vector<byte> geometryShaderFileData = co_await DX::ReadDataAsync(L"MonitorGeometryShader.cso");

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

	D3D11_BLEND_DESC blendDesc{};
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	auto& rtblend = blendDesc.RenderTarget[0];
	rtblend.BlendEnable = TRUE;
	rtblend.BlendOp = D3D11_BLEND_OP_ADD;
	rtblend.SrcBlend = D3D11_BLEND_SRC_ALPHA;
	rtblend.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	rtblend.SrcBlendAlpha = D3D11_BLEND_ONE;
	rtblend.DestBlendAlpha = D3D11_BLEND_ZERO;
	rtblend.BlendOpAlpha = D3D11_BLEND_OP_ADD;
	rtblend.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	// Create the texture sampler state.
	winrt::check_hresult(m_deviceResources->GetD3DDevice()->CreateBlendState(
		&blendDesc,
		&BlendState));

    // Once the cube is loaded, the object is ready to be rendered.
    m_loadingComplete = true;
};

void ShaderMonitorRenderer::ReleaseDeviceDependentResources()
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


} // namespace RenderMonitors
