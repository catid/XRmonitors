// Copyright 2019 Augmented Perception Corporation

#pragma once

#include "core.hpp"

#include "InputWindow.hpp"
#include "OpenXrD3D11SwapChains.hpp"
#include "MonitorRenderView.hpp"
#include "MonitorEnumerator.hpp"
#include "HolographicInputBanner.hpp"
#include "CameraClient.hpp"
#include "MonitorRenderController.hpp"
#include "MonitorRenderModel.hpp"
#include "CameraCalibration.hpp"
#include "ApplicationSettings.hpp"

namespace xrm {


//------------------------------------------------------------------------------
// MainApplication

class MainApplication
{
public:
    bool Run();

protected:
    // Desktop window for the application for keyboard and mouse input
    InputWindow Window;

    // Enumerator for monitors
    MonitorEnumerator Enumerator;

    // Win+Y Auto-Dismiss
    WinYAutoDismiss WinY;

    // Access to headset cameras
    CameraClient Cameras;

    // Camera calibration data
    HeadsetCameraCalibration CameraCalibration;

    // MVC Render Controller
    MonitorRenderController RenderController;

    // MVC Render Model
    MonitorRenderModel RenderModel;

    // MVC Render View: OpenXR render skeleton program
    OpenXrD3D11SwapChains Graphics;

    // Registry settings
    ApplicationSettings Settings;
};


} // namespace xrm
