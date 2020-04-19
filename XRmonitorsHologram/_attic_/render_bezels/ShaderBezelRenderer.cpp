// Copyright 2019 Augmented Perception Corporation

#include "stdafx.h"
#include "ShaderBezelRenderer.h"
#include "../../Common/DirectXHelper.h"

#include "BezelPixelShader.inl"
#include "BezelVertexShader.inl"
#include "BezelVPRTVertexShader.inl"

#include "core_logger.hpp"
#include "core_string.hpp"
using namespace core;

static logger::Channel Logger("Main");

namespace RenderBezels {

using namespace DirectX;
using namespace winrt::Windows::Foundation::Numerics;
using namespace winrt::Windows::UI::Input::Spatial;
using namespace Microsoft::WRL;
using namespace winrt::Windows::Perception::Spatial;

// Loads vertex and pixel shaders from files and instantiates the cube geometry.
ShaderBezelRenderer::ShaderBezelRenderer(std::shared_ptr<DX::DeviceResources> const& deviceResources) :
    m_deviceResources(deviceResources)
{
    CreateDeviceDependentResources();
}


//------------------------------------------------------------------------------
// RenderSphericalMonitor

bool ShaderBezelRenderer::RenderSphericalBezel(
	xrm::MonitorRenderInfo& render_info,
	const float3& color)
{
	// Loading is asynchronous. Resources must be created before drawing can occur.
	if (!m_loadingComplete) {
		return true; // Expected error
	}

	if (render_info.SphereBezel.IsEmpty()) {
		return true;
	}

	// The view and projection matrices are provided by the system; they are associated
	// with holographic cameras, and updated on a per-camera basis.
	// Here, we provide the model transform for the sample hologram. The model transform
	// matrix is transposed to prepare it for the shader.
	XMStoreFloat4x4(&m_modelConstantBufferData.model, XMMatrixTranspose(render_info.SphereBezel.ModelTransform));
	m_modelConstantBufferData.color.x = color.x;
	m_modelConstantBufferData.color.y = color.y;
	m_modelConstantBufferData.color.z = color.z;
	m_modelConstantBufferData.color.w = 1.f;

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
		render_info.SphereBezel.VertexBuffer.GetAddressOf(),
		&stride,
		&offset
	);
	context->IASetIndexBuffer(
		render_info.SphereBezel.IndexBuffer.Get(),
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

	// Draw the objects.
	context->DrawIndexedInstanced(
		render_info.SphereBezel.IndexCount,   // Index count per instance.
		2,              // Instance count.
		0,              // Start index location.
		0,              // Base vertex location.
		0               // Start instance location.
	);

	return true;
}


//------------------------------------------------------------------------------
// Setup

void ShaderBezelRenderer::CreateDeviceDependentResources()
{
    m_usingVprtShaders = m_deviceResources->GetDeviceSupportsVprt();

    // On devices that do support the D3D11_FEATURE_D3D11_OPTIONS3::
    // VPAndRTArrayIndexFromAnyShaderFeedingRasterizer optional feature
    // we can avoid using a pass-through geometry shader to set the render
    // target array index, thus avoiding any overhead that would be 
    // incurred by setting the geometry shader stage.

	const BYTE* shader_code = m_usingVprtShaders ? g_BezelVPRTVertexShader : g_BezelVertexShader;
	const size_t shader_bytes = m_usingVprtShaders ? sizeof(g_BezelVPRTVertexShader) : sizeof(g_BezelVertexShader);

	winrt::check_hresult(
        m_deviceResources->GetD3DDevice()->CreateVertexShader(
			shader_code,
			shader_bytes,
            nullptr,
            &m_vertexShader
        ));

    constexpr std::array<D3D11_INPUT_ELEMENT_DESC, 1> vertexDesc =
        { {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
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
            g_BezelPixelShader,
            sizeof(g_BezelPixelShader),
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
        std::vector<byte> geometryShaderFileData = co_await DX::ReadDataAsync(L"BezelGeometryShader.cso");

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

    // Once the cube is loaded, the object is ready to be rendered.
    m_loadingComplete = true;
};

void ShaderBezelRenderer::ReleaseDeviceDependentResources()
{
    m_loadingComplete = false;
    m_usingVprtShaders = false;
    m_vertexShader.Reset();
    m_inputLayout.Reset();
    m_pixelShader.Reset();
    m_geometryShader.Reset();
    m_modelConstantBuffer.Reset();
}


} // namespace RenderBezels
