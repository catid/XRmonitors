// Copyright 2019 Augmented Perception Corporation

#pragma once

#include "stdafx.h"

#include "../../Common/DeviceResources.h"
#include "../../Common/StepTimer.h"
#include "ShaderMonitorStructures.h"
#include "../GlobalRenderState.hpp"

namespace RenderMonitors {

using namespace Microsoft::WRL;


// This sample renderer instantiates a basic rendering pipeline.
class ShaderMonitorRenderer
{
public:
	ShaderMonitorRenderer(std::shared_ptr<DX::DeviceResources> const& deviceResources);
    void CreateDeviceDependentResources();
    void ReleaseDeviceDependentResources();

	// Render spherical monitor
	bool RenderSphericalMonitor(
		xrm::MonitorRenderInfo& render_info,
		ComPtr<ID3D11Texture2D>& texture);

private:
    // Cached pointer to device resources.
    std::shared_ptr<DX::DeviceResources>            m_deviceResources;

    // Direct3D resources for cube geometry.
    Microsoft::WRL::ComPtr<ID3D11InputLayout>       m_inputLayout;
    Microsoft::WRL::ComPtr<ID3D11Buffer>            m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer>            m_indexBuffer;
    Microsoft::WRL::ComPtr<ID3D11VertexShader>      m_vertexShader;
    Microsoft::WRL::ComPtr<ID3D11GeometryShader>    m_geometryShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>       m_pixelShader;
    Microsoft::WRL::ComPtr<ID3D11Buffer>            m_modelConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11SamplerState>      m_samplerState;
	Microsoft::WRL::ComPtr<ID3D11BlendState> BlendState;

    // System resources for cube geometry.
    ModelConstantBuffer                             m_modelConstantBufferData;
    uint32_t                                        m_indexCount = 0;

    // Variables used with the rendering loop.
    bool m_loadingComplete = false;

    // If the current D3D Device supports VPRT, we can avoid using a geometry
    // shader just to set the render target array index.
    bool m_usingVprtShaders = false;
};


} // namespace RenderMonitors
