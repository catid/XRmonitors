// Copyright 2019 Augmented Perception Corporation

#pragma once

#include "stdafx.h"

#include "MonitorTools.hpp"
#include "D3D11DesktopDuplication.hpp"
#include "MonitorEnumerator.hpp"
#include "render_monitors/ShaderMonitorRenderer.h"
#include "render_bezels/ShaderBezelRenderer.h"
#include "render_cameras/ShaderCameraRenderer.h"
#include "GlobalRenderState.hpp"
#include "QuadLayersTracker.hpp"
#include "../Common/DeviceResources.h"
#include "CameraClient.hpp"
#include "CameraCalibration.hpp"

namespace xrm {


//------------------------------------------------------------------------------
// DesktopRender

class DesktopRender
{
public:
	DesktopRender(core::CameraClient* cameras);

	void Initialize(
		std::shared_ptr<DX::DeviceResources> const& deviceResources);
	void Shutdown();

	void CreateVrResources();
	void ReleaseVrResources();

	void OnHotkey(int index, bool down);
	void OnDoff(bool doff);


	void UpdateUiSettings();
	void UpdateMonitorEnumeration();
	void UpdateDesktopDuplication();
	void UpdateHeadPose(SpatialLocation const& location);
	void UpdateSimulation(DX::StepTimer const& timer);

	void Render(
		const HolographicCameraPose& camera_pose,
		QuadLayersTracker& layer_tracker,
		DX::CameraResources* camera_resources);

protected:
	std::shared_ptr<DX::DeviceResources> VrDeviceResources;
	std::shared_ptr<RenderMonitors::ShaderMonitorRenderer> MonitorRenderer;
	std::shared_ptr<RenderBezels::ShaderBezelRenderer> BezelRenderer;
	std::shared_ptr<RenderCameras::ShaderCameraRenderer> CameraRenderer;

	// Settings:
	bool EnablePassthrough = false;

	core::CameraClient* Cameras = nullptr;
	bool CamerasStarted = false;
	SpatialLocation HeadsetLocation = nullptr;
	SpatialLocation CameraHeadsetLocation = nullptr;

	MonitorEnumerator Enumerator;

	HeadsetCameraCalibration Calibration;

	GlobalRenderState GlobalState;
	SphericalGeometry Sphere;
	std::vector<std::shared_ptr<MonitorRenderInfo>> RenderStates;
	std::vector<std::shared_ptr<D3D11DesktopDuplication>> Duplicates;

	unsigned LastMouseMonitorIndex = 0;


	// Recenter on current pose
	void Recenter(SpatialLocation const& location);

	void CleanupDuplicates();

	// Called on enumeration or recenter
	void SolveDesktopPositions();

	// Trim a Bezel rect so that it does not overlap with any monitors
	// Returns false if it is entirely invisible
	bool TrimBezelRect(unsigned monitor_index, RECT& bezel_rect, int bezel_width);

	// Set up rendering
	void SetupMonitor(MonitorEnumInfo& monitor, MonitorRenderInfo& render_state);
	void SetupBezel(MonitorEnumInfo& monitor, MonitorRenderInfo& render_state);

	// Render
	void RenderCameras(DX::CameraResources* camera_resources);
};


} // namespace xrm
