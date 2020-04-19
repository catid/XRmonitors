// Copyright 2019 Augmented Perception Corporation

#include "stdafx.h"

#include "QuadLayersTracker.hpp"

namespace xrm {

static logger::Channel Logger("Overlays");


//------------------------------------------------------------------------------
// QuadLayersTracker

void QuadLayersTracker::Start()
{
	MaxQuadLayerCount = Camera->MaxQuadLayerCount();

	Overlays.clear();

	// FIXME: Set to 0/1 for now for testing
	MaxQuadLayerCount = 0;
}

void QuadLayersTracker::End()
{
	if (!Overlays.empty())
	{
		winrt::array_view<const HolographicQuadLayer> layers(Overlays.data(), Overlays.data() + Overlays.size());

		Camera->QuadLayers().ReplaceAll(layers);
		Camera->IsPrimaryLayerEnabled(false);
	}
}

bool QuadLayersTracker::TryUpdateLayer(
	MonitorRenderInfo & render_state,
	ComPtr<ID3D11Texture2D> & texture)
{
	if (!texture) {
		return false;
	}

	if (Overlays.size() >= MaxQuadLayerCount) {
		return false; // Too many quad layers
	}

	Logger.Info("Rendering with quad layer");

	try {
		D3D11_TEXTURE2D_DESC desc{};
		texture->GetDesc(&desc);

		winrt::Windows::Foundation::Size size((float)desc.Width, (float)desc.Height);

		HolographicQuadLayer quad_layer(size);

		auto quadLayerUpdateParams = Frame->GetQuadLayerUpdateParameters(quad_layer);
		quadLayerUpdateParams.UpdateExtents({ 1.f, 1.f });
		quadLayerUpdateParams.UpdateLocationWithDisplayRelativeMode(float3{ 0.f, 0.f, -2.f }, quaternion::identity());

		auto surface = quadLayerUpdateParams.AcquireBufferToUpdateContent();

		ComPtr<IDXGISurface2> quadLayerBackBufferSurface;
		winrt::check_hresult(
			surface.as<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>()->GetInterface(
				IID_PPV_ARGS(&quadLayerBackBufferSurface)));

		UINT32 subresourceIndex = 0;
		ComPtr<ID3D11Texture2D> quadLayerBackBuffer;
		winrt::check_hresult(quadLayerBackBufferSurface->GetResource(
			IID_PPV_ARGS(&quadLayerBackBuffer),
			&subresourceIndex));

		D3D11_TEXTURE2D_DESC desc2{};
		quadLayerBackBuffer->GetDesc(&desc2);

		Context->CopyResource(
			quadLayerBackBuffer.Get(),
			texture.Get());

		Overlays.push_back(quad_layer);
	}
	catch (winrt::hresult_error & err) {
		Logger.Error("Render error: ", err.message().c_str());
		return false;
	}

	return true;
}


} // namespace xrm
