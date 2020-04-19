// Copyright 2019 Augmented Perception Corporation

#pragma once

#include "OpenXrD3D11.hpp"

#include "MonitorRenderView.hpp"
#include "MonitorEnumerator.hpp"
#include "MonitorRenderController.hpp"
#include "MonitorRenderModel.hpp"
#include "CameraCalibration.hpp"
#include "CameraImager.hpp"
#include "Plugins.hpp"

#include "core_logger.hpp"

namespace xrm {

using namespace core;


//------------------------------------------------------------------------------
// OpenXrD3D11SwapChains

class OpenXrD3D11SwapChains
{
    logger::Channel Logger;

public:
    OpenXrD3D11SwapChains()
        : Logger("OpenXrD3D11SwapChains")
    {
    }

    bool Initialize(
        MonitorRenderController* render_controller,
        MonitorRenderModel* render_model,
        HeadsetCameraCalibration* camera_calibration);
    void Shutdown();

    bool IsTerminated() const
    {
        return Terminated;
    }

protected:
    //--------------------------------------------------------------------------
    // Dependencies

    MonitorRenderController* RenderController = nullptr;
    MonitorRenderModel* RenderModel = nullptr;
    HeadsetCameraCalibration* CameraCalibration = nullptr;


    //--------------------------------------------------------------------------
    // Render Quality Knobs

    // Active SSAA factor (eye-buffer multiplier)
    float SsaaFactor = 1.4f;

    // MSAA sample count we want to target
    unsigned TargetSampleCount = 8;


    //--------------------------------------------------------------------------
    // Rendering

    // OpenXR instance properties that persist between replugging headset
    std::unique_ptr<XrComputerProperties> Computer;

    // OpenXR session properties that reset each time headset is replugged
    std::unique_ptr<XrHeadsetProperties> Headset;

    // Rendering properties
    std::unique_ptr<XrRenderProperties> Rendering;

    // We construct the render view anew each time we go through re-init
    std::unique_ptr<MonitorRenderView> RenderView;

    // We construct the render view anew each time we go through re-init
    std::unique_ptr<PluginManager> Plugins;

    // Camera imager - Maintains textures based on the render device
    std::unique_ptr<CameraImager> Imager;


    //--------------------------------------------------------------------------
    // Worker thread

    std::shared_ptr<std::thread> Thread;
    std::atomic<bool> Terminated = ATOMIC_VAR_INIT(false);

    void StartThread();
    void StopThread();
    void Loop();


    //--------------------------------------------------------------------------
    // Instance creation

    std::vector<const char*> SelectExtensions();
    bool CreateInstanceAndLogger();

    bool DetectHeadsetSystemId();
    void InitializeHeadsetRendering();

    DXGI_FORMAT SelectSwapchainFormat();
    XrEnvironmentBlendMode SelectEnvironmentBlendMode();

    void CreateSwapchains();

    void CreateSwapchain(
        XrD3D11Swapchain& swapchain,
        const XrViewConfigurationView& view_config);

    void CreateSwapchain(
        XrD3D11Swapchain& swapchain,
        uint32_t width,
        uint32_t height);

    void CreateMultisampleRenderTarget(
        std::shared_ptr<XrD3D11ProjectionView>& view);

    // Return true if an event is available, otherwise return false
    bool TryReadNextEvent(XrEventDataBuffer* buffer) const;

    void ProcessEvents(bool* exitRenderLoop, bool* requestRestart);
    void RenderFrame();

    void RenderFrameViews();
    void RenderFrameQuads();
};


} // namespace xrm
