// Copyright 2019 Augmented Perception Corporation

#include "stdafx.h"
#include "OpenXrProgram.hpp"

#include <list>

#include <wrl/client.h>

#include <DirectXColors.h>
#include <D3Dcompiler.h>

#include "XrUtility/XrMath.h"
#include "D3D11Tools.hpp"

namespace xrm {

using Microsoft::WRL::ComPtr;
using namespace DirectX;


//------------------------------------------------------------------------------
// Cube

struct Cube
{
    XrSpace Space{ XR_NULL_HANDLE };
    XrVector3f Scale{ 0.1f, 0.1f, 0.1f };

    XrBool32 PoseValid{ false };
    XrPosef Pose = xr::math::Pose::Identity();


    ComPtr<ID3D11VertexShader> VertexShader;
    ComPtr<ID3D11PixelShader> PixelShader;
    ComPtr<ID3D11InputLayout> InputLayout;
    ComPtr<ID3D11Buffer> MvpCBuffer;
    ComPtr<ID3D11Buffer> VertexBuffer;
    ComPtr<ID3D11Buffer> IndexBuffer;


    void SetPosition(XrSpace space);
    void InitializeD3D(D3D11DeviceContext& device_context);
};


//------------------------------------------------------------------------------
// OpenXrD3D11Graphics

class OpenXrD3D11Graphics : public IGraphicsPlugin
{
    logger::Channel Logger;

public:
    OpenXrD3D11Graphics()
        : Logger("OpenXrD3D11Graphics")
    {
    }

	void InitializeDevice(XrInstance instance, XrSystemId systemId) override;
	void OnSessionInit(XrRenderState* state) override;

	const XrGraphicsBindingD3D11KHR* GetGraphicsBinding() const override;
	int64_t SelectColorSwapchainFormat(const std::vector<int64_t>& runtimeFormats) const override;
	std::vector<XrSwapchainImageBaseHeader*> AllocateSwapchainImageStructs(uint32_t capacity, const XrSwapchainCreateInfo& /*swapchainCreateInfo*/) override;
	void RenderLayers(XrTime predictedDisplayTime, std::vector<XrCompositionLayerBaseHeader*>& layers) override;

private:
    D3D11DeviceContext DeviceContext;
    XrGraphicsBindingD3D11KHR m_xrGraphicsBindingD3D11{XR_TYPE_GRAPHICS_BINDING_D3D11_KHR};
    std::list<std::vector<XrSwapchainImageD3D11KHR>> m_swapchainImageBuffers;

    XrRenderState* State = nullptr;

    // Cube
    Cube m_Cube;

    // Map color buffer to associated depth buffer. This map is populated on demand.
    std::map<ID3D11Texture2D*, ComPtr<ID3D11DepthStencilView>> m_colorToDepthMap;

#if 0
	// Data passed to OpenXrProgram: Left and Right Camera Images
	XrCompositionLayerProjection LeftCameraLayer{ XR_TYPE_COMPOSITION_LAYER_PROJECTION };
	std::vector<XrCompositionLayerProjectionView> LeftCameraProjectionLayerViews;
	XrCompositionLayerProjection RightCameraLayer{ XR_TYPE_COMPOSITION_LAYER_PROJECTION };
	std::vector<XrCompositionLayerProjectionView> RightCameraProjectionLayerViews;

	XrCompositionLayerCylinderKHR MonitorLayer{ XR_TYPE_COMPOSITION_LAYER_PROJECTION };
#endif

    // The projection layer consists of projection layer views.
    XrCompositionLayerProjection BaseLayer{ XR_TYPE_COMPOSITION_LAYER_PROJECTION };
    std::vector<XrCompositionLayerProjectionView> ProjectionLayerViews;


	ComPtr<ID3D11DepthStencilView> GetDepthStencilView(ID3D11Texture2D* colorTexture);

	void RenderView(const XrCompositionLayerProjectionView& layerView,
		const XrSwapchainImageBaseHeader* swapchainImage,
		const XrEnvironmentBlendMode environmentBlendMode,
		int64_t colorSwapchainFormat);

	bool RenderLayer(XrTime predictedDisplayTime, XrCompositionLayerProjection& layer);
};


} // namespace xrm
