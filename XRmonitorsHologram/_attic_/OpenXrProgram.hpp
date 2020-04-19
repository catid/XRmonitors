// Copyright 2019 Augmented Perception Corporation

#pragma once

#include "OpenXrTools.hpp"
#include "OpenXrD3D11Graphics.hpp"
#include "MonitorEnumerator.hpp"
#include "MonitorRenderController.hpp"
#include "MonitorRenderModel.hpp"
#include "CameraCalibration.hpp"

#include "core_logger.hpp"

namespace xrm {

using namespace core;

using XrDebugMessengerHandle = XrHandle<XrDebugUtilsMessengerEXT, LoaderXrTermDestroyDebugUtilsMessengerEXT>;


//------------------------------------------------------------------------------
// XrGlobalProperties

struct XrGlobalProperties
{

};


//------------------------------------------------------------------------------
// XrD3D11ViewSwapchain

// Swapchain for each main view requested by OS
struct XrD3D11ViewSwapchain
{
    XrViewConfigurationView ConfigView;

    XrView View;

    XrSwapchain Handle;
    int32_t Width;
    int32_t Height;

    std::vector<XrSwapchainImageD3D11KHR> Images;
};


//------------------------------------------------------------------------------
// OpenXrD3D11Swapchains

class OpenXrD3D11Swapchains
{
    logger::Channel Logger;

public:
    OpenXrD3D11Swapchains()
        : Logger("OpenXrD3D11Swapchains")
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
    // Dependencies:
    std::unique_ptr<IGraphicsPlugin> GraphicsPlugin;
    MonitorRenderController* RenderController = nullptr;
    MonitorRenderModel* RenderModel = nullptr;
    HeadsetCameraCalibration* CameraCalibration = nullptr;

    const XrFormFactor m_formFactor{ XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY };
    const XrViewConfigurationType m_primaryViewConfigurationType{ XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO };

    XrRenderState State;

    struct {
        bool UnboundedRefSpaceSupported = false;
        bool SpatialAnchorSupported = false;
        bool CylinderSupported = false;
    } m_optionalExtensions;

    constexpr static uint32_t LeftSide = 0;
    constexpr static uint32_t RightSide = 1;
    std::array<XrPath, 2> m_subactionPaths{};

    XrActionSetHandle m_actionSet;
    XrActionHandle m_placeAction;
    XrActionHandle m_poseAction;
    XrActionHandle m_vibrateAction;

    XrInstanceHandle m_instance;
    XrDebugMessengerHandle m_messenger;

    uint64_t m_systemId = XR_NULL_SYSTEM_ID;

    bool m_sessionRunning = false;
    XrSessionState m_sessionState{ XR_SESSION_STATE_UNKNOWN };

    std::shared_ptr<std::thread> Thread;
    std::atomic<bool> Terminated = ATOMIC_VAR_INIT(false);

    uint32_t RequestedMsaa = 8;
    float SsaaFactor = 1.4f;


    constexpr bool IsSessionVisible() const
    {
        return m_sessionState == XR_SESSION_STATE_VISIBLE || m_sessionState == XR_SESSION_STATE_FOCUSED;
    }

    constexpr bool IsSessionFocused() const
    {
        return m_sessionState == XR_SESSION_STATE_FOCUSED;
    }

    XrPath GetXrPath(std::string_view string) const
    {
        XrPath path;
        CHECK_XRCMD(xrStringToPath(m_instance.Get(), string.data(), &path));
        return path;
    }


    std::vector<const char*> SelectExtensions();
    bool InitializeSystem();
    XrEnvironmentBlendMode SelectEnvironmentBlendMode();
    void InitializeSession();
    void CreateActions();
    void CreateSpaces();
    void CreateSwapchains();

    // Return true if an event is available, otherwise return false.
    bool TryReadNextEvent(XrEventDataBuffer* buffer) const;

    void ProcessEvents(bool* exitRenderLoop, bool* requestRestart);
    void PollActions();
    void RenderFrame();

    void Loop();
};


} // namespace xrm
