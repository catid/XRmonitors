// Copyright 2019 Augmented Perception Corporation

#pragma once

#include "stdafx.h"

#include "MonitorTools.hpp"
#include "render_monitors/ShaderMonitorRenderer.h"

namespace xrm {

using namespace winrt::Windows::Graphics::Holographic;
using namespace winrt::Windows::Perception::Spatial;


//------------------------------------------------------------------------------
// QuadLayersTracker

struct QuadLayersTracker
{
	// Set these first
	HolographicCamera* Camera = nullptr;
	const HolographicFrame* Frame = nullptr;
	ID3D11DeviceContext3* Context = nullptr;


	// Start tracking layers for this camera's render work
	void Start();

	// Returns false if no more layers available
	bool TryUpdateLayer(
		MonitorRenderInfo& render_state,
		ComPtr<ID3D11Texture2D>& texture);

	// Submit any active layers
	void End();


	// This is filled in by Start()
	unsigned MaxQuadLayerCount = 0;

	std::vector<HolographicQuadLayer> Overlays;
};


} // namespace xrm
