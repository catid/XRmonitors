// Copyright 2019 Augmented Perception Corporation

#pragma once

#include "stdafx.h"

#include "../../Common/DeviceResources.h"
#include "../../Common/StepTimer.h"
#include "ShaderBezelStructures.h"
#include "../GlobalRenderState.hpp"

namespace RenderBezels {

using namespace Microsoft::WRL;
using namespace winrt::Windows::Foundation::Numerics;


//------------------------------------------------------------------------------
// ShaderBezelRenderer

class ShaderBezelRenderer
{
public:
	ShaderBezelRenderer(std::shared_ptr<DX::DeviceResources> const& deviceResources);
    void CreateDeviceDependentResources();
    void ReleaseDeviceDependentResources();

	// Render spherical bezel
	bool RenderSphericalBezel(
		xrm::MonitorRenderInfo& render_info,
		const float3& color);

private:
    // Cached pointer to device resources.
    std::shared_ptr<DX::DeviceResources>            m_deviceResources;

    // Direct3D resources for cube geometry.
    Microsoft::WRL::ComPtr<ID3D11InputLayout>       m_inputLayout;
    Microsoft::WRL::ComPtr<ID3D11VertexShader>      m_vertexShader;
    Microsoft::WRL::ComPtr<ID3D11GeometryShader>    m_geometryShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>       m_pixelShader;
    Microsoft::WRL::ComPtr<ID3D11Buffer>            m_modelConstantBuffer;

    // System resources for cube geometry.
    ModelConstantBuffer                             m_modelConstantBufferData;

    // Variables used with the rendering loop.
    bool m_loadingComplete = false;

    // If the current D3D Device supports VPRT, we can avoid using a geometry
    // shader just to set the render target array index.
    bool m_usingVprtShaders = false;
};


} // namespace RenderBezels
