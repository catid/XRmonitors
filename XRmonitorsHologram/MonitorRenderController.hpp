// Copyright 2019 Augmented Perception Corporation

#pragma once

#include "MonitorTools.hpp"
#include "MonitorEnumerator.hpp"
#include "MonitorRenderModel.hpp"
#include "D3D11CrossAdapterDuplication.hpp"
#include "D3D11SameAdapterDuplication.hpp"
#include "CameraCalibration.hpp"
#include "CameraClient.hpp"
#include "CameraImager.hpp"
#include "ApplicationSettings.hpp"
#include "HolographicInputBanner.hpp"
#include "InputWindow.hpp"
#include "Plugins.hpp"

namespace xrm {


//------------------------------------------------------------------------------
// Datatypes

// One monitor in a sorted list of monitors
struct SortedMonitor
{
    unsigned MonitorIndex;
    int ScreenCenterX, ScreenCenterY;

    bool operator<(const SortedMonitor& rhs) const
    {
        // If they are roughly the same X position:
        if (std::abs(ScreenCenterX - rhs.ScreenCenterX) < 16)
        {
            // Pick the top one
            return ScreenCenterY < rhs.ScreenCenterY;
        }

        // Pick the left one
        return ScreenCenterX < rhs.ScreenCenterX;
    }


    void SetFromMonitorEnumInfo(const MonitorEnumInfo& info);
};


//------------------------------------------------------------------------------
// MonitorRenderController

class MonitorRenderController
{
public:
    void Initialize(
        WinYAutoDismiss* win_y,
        MonitorEnumerator* enumerator,
        MonitorRenderModel* render_model,
        HeadsetCameraCalibration* camera_calibration,
        CameraClient* cameras,
        ApplicationSettings* settings,
        InputWindow* input_window);
    void Shutdown();

    /// Start render loop
    void StartRendering(
        CameraImager* imager,
        XrComputerProperties* computer,
        XrHeadsetProperties* headset,
        XrRenderProperties* rendering,
        PluginManager* plugins);

    /// Update the MonitorRenderModel at the start of a frame
    void UpdateModel();

protected:
    WinYAutoDismiss* WinY = nullptr;
    MonitorEnumerator* Enumerator = nullptr;
    MonitorRenderModel* RenderModel = nullptr;

    HeadsetCameraCalibration* CameraCalibration = nullptr;
    CameraClient* Cameras = nullptr;
    CameraImager* Imager = nullptr;

    ApplicationSettings* Settings = nullptr;

    XrComputerProperties* Computer = nullptr;
    XrHeadsetProperties* Headset = nullptr;
    XrRenderProperties* Rendering = nullptr;

    InputWindow* Window = nullptr;
    KeyboardInput* Keyboard = nullptr;

    PluginManager* Plugins = nullptr;

    std::vector<std::shared_ptr<ID3D11DesktopDuplication>> Duplicates;

    std::vector<SortedMonitor> SortedMonitors;
    unsigned CenteredMonitorIndex = 0;

    bool RecenterOnFirstEnum = false;


    void UpdateMonitorEnumeration();
    void CleanupDuplicates();
    void UpdateDesktopDuplication();
    void SolveDesktopPositions();

    void HandleKeystrokes();
    void OnRecenter();
    void OnIncrease();
    void OnDecrease();
    void OnPanLeft();
    void OnPanRight();

    void UpdatePitchYaw();

    void SetCenteredMonitorForMouse();

    void UpdateMonitorModel();

    void GenerateCylindricalGeometry(
        MonitorEnumInfo* enum_info,
        MonitorRenderState* render_state);

    void UpdateGaze();
};


} // namespace xrm
