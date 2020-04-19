// Copyright 2019 Augmented Perception Corporation

#include "stdafx.h"

#include "DesktopRender.hpp"

namespace xrm {

static logger::Channel Logger("Render");


//------------------------------------------------------------------------------
// DesktopRender

DesktopRender::DesktopRender(core::CameraClient* cameras)
{
	Cameras = cameras;
}

void DesktopRender::Initialize(
	std::shared_ptr<DX::DeviceResources> const& resources)
{
	Calibration.ReadForCurrentHeadset();

	VrDeviceResources = resources;
	MonitorRenderer = std::make_unique<RenderMonitors::ShaderMonitorRenderer>(VrDeviceResources);
	BezelRenderer = std::make_unique<RenderBezels::ShaderBezelRenderer>(VrDeviceResources);
	CameraRenderer = std::make_unique<RenderCameras::ShaderCameraRenderer>(Calibration, VrDeviceResources);

	Enumerator.Initialize();

	GlobalState.SetDPI(128.f);

	CamerasStarted = Cameras->Start();
}

void DesktopRender::CreateVrResources()
{
	Calibration.ReadForCurrentHeadset();

	MonitorRenderer->CreateDeviceDependentResources();
	BezelRenderer->CreateDeviceDependentResources();
	CameraRenderer->CreateDeviceDependentResources(Calibration);
}

void DesktopRender::ReleaseVrResources()
{
	MonitorRenderer->ReleaseDeviceDependentResources();
	BezelRenderer->ReleaseDeviceDependentResources();
	CameraRenderer->ReleaseDeviceDependentResources();
}

void DesktopRender::Shutdown()
{
	Cameras->Stop();

	Enumerator.Shutdown();

	CleanupDuplicates();
	RenderStates.clear();

	ReleaseVrResources();
	VrDeviceResources.reset();
}

void DesktopRender::OnHotkey(int index, bool down)
{
	Logger.Info("Hotkey ", index, " down=", down);

	if (index == 1) {
		if (down) {
			GlobalState.RecenterActive = true;
			GlobalState.RecenterChangeTicks = ::GetTickCount64();
		}
	}
	else if (index == 2 && down) {
		GlobalState.DPI *= 1.5f;
		if (GlobalState.DPI > 256.f) {
			GlobalState.DPI = 256.f;
		}
		GlobalState.SetDPI(GlobalState.DPI);

		Logger.Info("Updated DPI: ", GlobalState.DPI, " PixelsPerMeter: ", GlobalState.PixelsPerMeter);

		GlobalState.RecenterActive = true;
		//SolveDesktopPositions();
	}
	else if (index == 3 && down) {
		if (Sphere.IsScalingToFit()) {
			Logger.Info("Refusing to scale up more because we hit the limits of the sphere");
			return;
		}
		GlobalState.DPI /= 1.5f;
		if (GlobalState.DPI < 1.f) {
			GlobalState.DPI = 1.f;
		}
		GlobalState.SetDPI(GlobalState.DPI);

		Logger.Info("Updated DPI: ", GlobalState.DPI, " PixelsPerMeter: ", GlobalState.PixelsPerMeter);

		GlobalState.RecenterActive = true;
		//SolveDesktopPositions();
	}
}

void DesktopRender::CleanupDuplicates()
{
	for (auto& dupe : Duplicates) {
		dupe->StartShutdown();
	}
	for (auto& dupe : Duplicates) {
		dupe->Shutdown();
	}
	Duplicates.clear();
}

void DesktopRender::UpdateUiSettings()
{
	XrmUiData data;
	Cameras->ReadUiState(data);
	EnablePassthrough = data.EnablePassthrough;
}

void DesktopRender::UpdateMonitorEnumeration()
{
	if (!Enumerator.UpdateEnumeration()) {
		return;
	}

	Logger.Info("Shutting down duplication subsystem");

	CleanupDuplicates();

	const int count = Enumerator.Monitors.size();
	if (count == 0) {
		RenderStates.clear();
		Duplicates.clear();
	}
	else {
		RenderStates.resize(count);
		Duplicates.resize(count);
	}

	DXGI_ADAPTER_DESC adapter_desc{};
	HRESULT hr = VrDeviceResources->GetDXGIAdapter()->GetDesc(&adapter_desc);
	if (FAILED(hr)) {
		Logger.Error("Failed to get VR adapter LUID");
	}
	else {
		Logger.Info("VR adapter LUID: ", LUIDToString(adapter_desc.AdapterLuid));
	}

	Logger.Info("Starting duplication");

	for (int i = 0; i < count; ++i)
	{
		Enumerator.Monitors[i]->LogInfo();

		RenderStates[i] = std::make_shared<MonitorRenderInfo>();

		Duplicates[i] = std::make_shared<D3D11DesktopDuplication>();

		Duplicates[i]->Initialize(
			Enumerator.Monitors[i],
			VrDeviceResources);
	}

	Logger.Info("Solving desktop positions");

	// Update desktop positions
	SolveDesktopPositions();
}

// In a separate function to make it compatible with omp parallel for
static void UpdateVrTexture(D3D11DesktopDuplication* duper)
{
	duper->UpdateVrRenderTexture();
}

void DesktopRender::UpdateDesktopDuplication()
{
	const uint64_t t0 = GetTimeUsec();

	const int count = Enumerator.Monitors.size();

//#pragma omp parallel for
	for (int i = 0; i < count; ++i) {
		UpdateVrTexture(Duplicates[i].get());
	}

	const uint64_t t1 = GetTimeUsec();
	const uint64_t update_usec = t1 - t0;

	if (update_usec > 5000) {
		Logger.Warning("Slow duplication: Blit took ", (t1 - t0), " usec");
	}
}

void DesktopRender::OnDoff(bool doff)
{
	Logger.Info("Handling headset doff: ", doff);

	GlobalState.HaveRecenterPose = false;
}

void DesktopRender::UpdateHeadPose(SpatialLocation const& headset_location)
{
	if (!headset_location) {
		return;
	}
	HeadsetLocation = headset_location;

	// If we have no pose:
	if (!GlobalState.HaveRecenterPose) {
		Logger.Info("Starting to recenter on first head pose");

		Recenter(headset_location);

		GlobalState.HaveRecenterPose = true;
		GlobalState.FirstPoseTicks = ::GetTickCount64();
	}
	else if (GlobalState.FirstPoseTicks != 0) {
		if (::GetTickCount64() - GlobalState.FirstPoseTicks > 1000) {
			Logger.Info("Recentering again 1 second after first pose");

			Recenter(headset_location);

			GlobalState.FirstPoseTicks = 0;
		}
	}
	else if (GlobalState.RecenterActive) {
		GlobalState.RecenterActive = false;
		Recenter(headset_location);
	}
}

void DesktopRender::UpdateSimulation(
	DX::StepTimer const& timer)
{
	for (auto& render_state : RenderStates)
	{
		// Do updates here?
	}
}

void DesktopRender::Render(
	const HolographicCameraPose& camera_pose,
	QuadLayersTracker& layer_tracker,
	DX::CameraResources* camera_resources)
{
#if 0
	// Implement K1 scan here
	CameraRenderer->ReleaseDeviceDependentResources();
	CameraRenderer->CreateDeviceDependentResources(Calibration);
#endif

	const uint64_t t0 = GetTimeUsec();

	CURSORINFO ci;
	ci.cbSize = sizeof(ci);
	::GetCursorInfo(&ci);
	const int x = ci.ptScreenPos.x;
	const int y = ci.ptScreenPos.y;

	for (auto& monitor : Enumerator.Monitors)
	{
		const unsigned monitor_index = monitor->MonitorIndex;
		auto& render_state = *RenderStates[monitor_index];
		auto& dupe = Duplicates[monitor_index];

#if 1
		bool layer_success = layer_tracker.TryUpdateLayer(
			render_state,
			dupe->VrRenderTexture);

		if (!layer_success)
		{
			if (!MonitorRenderer->RenderSphericalMonitor(
				render_state,
				dupe->VrRenderTexture)) {
				Logger.Warning("Monitor render failed for monitor #", monitor_index);
			}
		}
#endif

		// If the cursor is on this monitor:
		RECT coords = monitor->Coords;
		bool has_mouse = false;
		if (x >= coords.left &&
			x < coords.right &&
			y >= coords.top &&
			y < coords.bottom)
		{
			has_mouse = true;
		}

		float3 color;
		if (monitor->HeadlessGhost || monitor->DuetDisplay) {
			if (has_mouse) {
				color = float3(1.f, 0.7f, 0.0f);
			}
			else {
				color = float3(1.f, 0.0f, 0.0f);
			}
		}
		else {
			if (has_mouse) {
				color = float3(1.f, 1.f, 1.f);
			}
			else {
				color = float3(0.f, 0.f, 0.f);
			}
		}

		if (!BezelRenderer->RenderSphericalBezel(
			render_state,
			color)) {
			Logger.Warning("Monitor render failed for monitor #", monitor_index);
		}
	}

	if (EnablePassthrough) {
		RenderCameras(camera_resources);
	}

	const uint64_t t1 = GetTimeUsec();
	const uint64_t render_usec = (t1 - t0);
	if (render_usec > 2000) {
		Logger.Warning("Slow render: ", Enumerator.Monitors.size(), " monitors in ", (t1 - t0), " usec");
	}
}

void DesktopRender::RenderCameras(DX::CameraResources* camera_resources)
{
	if (!CamerasStarted || !HeadsetLocation) {
		return;
	}

	CameraFrame frame;
	if (Cameras->AcquireNextFrame(frame)) {
		if (CameraRenderer->UpdateTexture(frame.CameraImage, frame.Width, frame.Height)) {
			CameraHeadsetLocation = HeadsetLocation;
		}
		Cameras->ReleaseFrame();
	}

	CameraRenderer->RenderCamera(
		Calibration,
		CameraHeadsetLocation,
		camera_resources);
}

void DesktopRender::Recenter(SpatialLocation const& headset_location)
{
	GlobalState.HeadsetReset_Orientation = headset_location.Orientation();
	GlobalState.HeadsetReset_Position = headset_location.Position();

	SolveDesktopPositions();
}

void DesktopRender::SolveDesktopPositions()
{
	Logger.Info("Resolving desktop positions");

	CURSORINFO ci;
	ci.cbSize = sizeof(ci);
	::GetCursorInfo(&ci);
	const int x = ci.ptScreenPos.x;
	const int y = ci.ptScreenPos.y;

	bool first = true;
	RECT extents{};

	for (auto& monitor : Enumerator.Monitors)
	{
		RECT coords = monitor->Coords;

		// If the cursor is on this monitor:
		if (x >= coords.left &&
			x < coords.right &&
			y >= coords.top &&
			y < coords.bottom)
		{
			GlobalState.MouseFocusMonitor = monitor->MonitorIndex;
			GlobalState.ScreenFocusCenterX = monitor->Coords.left + monitor->ScreenSpaceWidth / 2;
			GlobalState.ScreenFocusCenterY = monitor->Coords.top + monitor->ScreenSpaceHeight / 2;
		}

		if (first) {
			first = false;
			extents = monitor->Coords;
		}
		else {
			RECT rect = monitor->Coords;
			if (extents.left > rect.left) {
				extents.left = rect.left;
			}
			if (extents.right < rect.right) {
				extents.right = rect.right;
			}
			if (extents.top < rect.top) {
				extents.top = rect.top;
			}
			if (extents.bottom > rect.bottom) {
				extents.bottom = rect.bottom;
			}
		}
	}

	Sphere.InitializeFromExtent(
		GlobalState.MetersPerPixel,
		extents,
		GlobalState.ScreenFocusCenterX,
		GlobalState.ScreenFocusCenterY,
		GlobalState.HmdFocalDistanceMeters);

	for (auto& monitor : Enumerator.Monitors)
	{
		auto& render_state = RenderStates[monitor->MonitorIndex];

		SetupMonitor(*monitor, *render_state);
		SetupBezel(*monitor, *render_state);
	}
}

void DesktopRender::SetupMonitor(MonitorEnumInfo& monitor, MonitorRenderInfo& render_state)
{
	render_state.SphereMonitor.GenerateVectors(
		Sphere,
		monitor.Coords);

	render_state.SphereMonitor.CreateD3D11Buffers(VrDeviceResources->GetD3DDevice());

	render_state.SphereMonitor.Center = GlobalState.HeadsetReset_Position;
	render_state.SphereMonitor.Orientation = GlobalState.HeadsetReset_Orientation;

	const XMMATRIX monitorOrientation = XMMatrixRotationQuaternion(XMLoadQuaternion(
		&render_state.SphereMonitor.Orientation));

	const XMMATRIX monitorTranslation = XMMatrixTranslationFromVector(XMLoadFloat3(
		&render_state.SphereMonitor.Center));

	render_state.SphereMonitor.ModelTransform = XMMatrixMultiply(monitorOrientation, monitorTranslation);
}

void DesktopRender::SetupBezel(MonitorEnumInfo& monitor, MonitorRenderInfo& render_state)
{
	const unsigned monitor_index = monitor.MonitorIndex;

	render_state.SphereBezel.StartVectors();
	RECT monitor_coords = monitor.Coords;

	const int bezel_width = 16;

	RECT top_rect = monitor_coords;
	top_rect.bottom = top_rect.top;
	top_rect.top -= bezel_width;

	if (TrimBezelRect(monitor_index, top_rect, bezel_width)) {
		render_state.SphereBezel.AddRect(
			Sphere,
			top_rect);
	}

	RECT bottom_rect = monitor_coords;
	bottom_rect.top = bottom_rect.bottom;
	bottom_rect.bottom += bezel_width;

	if (TrimBezelRect(monitor_index, bottom_rect, bezel_width)) {
		render_state.SphereBezel.AddRect(
			Sphere,
			bottom_rect);
	}

	RECT left_rect = monitor_coords;
	left_rect.right = left_rect.left;
	left_rect.left -= bezel_width;

	if (TrimBezelRect(monitor_index, left_rect, bezel_width)) {
		render_state.SphereBezel.AddRect(
			Sphere,
			left_rect);
	}

	RECT right_rect = monitor_coords;
	right_rect.left = left_rect.right;
	right_rect.right += bezel_width;

	if (TrimBezelRect(monitor_index, right_rect, bezel_width)) {
		render_state.SphereBezel.AddRect(
			Sphere,
			right_rect);
	}

	if (!render_state.SphereBezel.IsEmpty())
	{
		render_state.SphereBezel.CreateD3D11Buffers(VrDeviceResources->GetD3DDevice());

		render_state.SphereBezel.Center = GlobalState.HeadsetReset_Position;
		render_state.SphereBezel.Orientation = GlobalState.HeadsetReset_Orientation;

		const XMMATRIX bezelOrientation = XMMatrixRotationQuaternion(XMLoadQuaternion(
			&render_state.SphereBezel.Orientation));

		const XMMATRIX bezelTranslation = XMMatrixTranslationFromVector(XMLoadFloat3(
			&render_state.SphereBezel.Center));

		render_state.SphereBezel.ModelTransform = XMMatrixMultiply(bezelOrientation, bezelTranslation);
	}
}

bool DesktopRender::TrimBezelRect(unsigned monitor_index, RECT& r1, int bezel_width)
{
	for (auto& monitor : Enumerator.Monitors)
	{
		RECT r2 = monitor->Coords;
		if (monitor->MonitorIndex != monitor_index) {
			r2.left -= bezel_width;
			r2.right += bezel_width;
			r2.top -= bezel_width;
			r2.bottom += bezel_width;
		}

		// If they do not intersect:
		if (r2.left > r1.right ||
			r2.right < r1.left ||
			r2.top > r1.bottom ||
			r2.bottom < r1.top)
		{
			continue;
		}

		// Option A: Trim horizontally
		RECT optA = r1;
		if (optA.left > r2.left) {
			// We should trim the left side
			optA.left = r2.right;
		}
		else {
			// We should trim the right side down
			optA.right = r2.left;
		}

		// Option B: Trim vertically
		RECT optB = r1;
		if (optB.top > r2.top) {
			// We should trim the top side
			optB.top = r2.bottom;
		}
		else {
			// We should trim the bottom side down
			optB.bottom = r2.top;
		}

		// Check if the resulting rectangle is okay
		const bool opt_a_empty = optA.left >= optA.right;
		const bool opt_b_empty = optB.top >= optB.bottom;
		const bool opt_a_okay = !opt_a_empty && (r2.top < optA.bottom || r2.bottom < optA.top);
		const bool opt_b_okay = !opt_b_empty && (r2.left < optB.right || r2.right < optB.left);

		if (opt_a_okay && opt_b_okay) {
			// Pick the one with the larger area
			const unsigned area_a = (optA.right - optA.left) * (optA.bottom - optA.top);
			const unsigned area_b = (optB.right - optB.left) * (optB.bottom - optB.top);
			if (area_a > area_b) {
				r1 = optA;
			}
			else {
				r1 = optB;
			}
		}
		else if (opt_a_okay) {
			r1 = optA;
		}
		else if (opt_b_okay) {
			r1 = optB;
		}
		else {
			// No way to fix it
			return false;
		}
	}

	return true;
}


} // namespace xrm
