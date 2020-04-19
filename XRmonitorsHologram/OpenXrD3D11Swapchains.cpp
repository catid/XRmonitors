// Copyright 2019 Augmented Perception Corporation

#include "stdafx.h"

#include "OpenXrD3D11SwapChains.hpp"

#include <unordered_map>

#include "core_logger.hpp"

#include "XrUtility/XrMath.h"
#include "WindowsHolographic.hpp"

namespace xrm {

using namespace core;
using namespace xr::math;

static logger::Channel ModuleLogger("OpenXR");


//------------------------------------------------------------------------------
// OpenXrD3D11SwapChains : Worker Thread

void OpenXrD3D11SwapChains::StartThread()
{
    Terminated = false;
    Thread = std::make_shared<std::thread>(&OpenXrD3D11SwapChains::Loop, this);
}

void OpenXrD3D11SwapChains::StopThread()
{
    Terminated = true;
    JoinThread(Thread);
}

void OpenXrD3D11SwapChains::Shutdown()
{
    StopThread();
}

void OpenXrD3D11SwapChains::Loop()
{
    core::SetCurrentThreadName("OpenXrD3D11");

    Logger.Info("OpenXR thread started");

    // Set render thread to highest priority
    ::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

    while (!Terminated)
    {
        // Interactive wait while headset is not functional
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        try {
            // Make sure everything is clear
            Imager.reset();
            RenderView.reset();
            Plugins.reset();
            Rendering.reset();
            Headset.reset();

            // Create objects:

            Headset = std::make_unique<XrHeadsetProperties>();
            Rendering = std::make_unique<XrRenderProperties>();
            Imager = std::make_unique<CameraImager>();
            Plugins = std::make_unique<PluginManager>();

            // QuadLayers get added later in reaction to enumerated monitors.

            RenderView = std::make_unique<MonitorRenderView>(
                RenderController,
                RenderModel,
                CameraCalibration,
                Computer.get(),
                Headset.get(),
                Rendering.get(),
                Plugins.get());

            // PluginManager

            if (!DetectHeadsetSystemId()) {
                Logger.Warning("Headset: Not detected.");
                continue;
            }

            Logger.Info("Headset detected: Initializing rendering");

            InitializeHeadsetRendering();
        }
        catch (std::exception& ex) {
            Logger.Error("Headset rendering initialization failed: ", ex.what());
            continue;
        }

        RenderController->StartRendering(
            Imager.get(),
            Computer.get(),
            Headset.get(),
            Rendering.get(),
            Plugins.get());

        try {
            while (!Terminated)
            {
                bool exitRenderLoop = false, requestRestart = false;
                ProcessEvents(&exitRenderLoop, &requestRestart);

                if (exitRenderLoop) {
                    if (requestRestart) {
                        Logger.Info("Event processing result: Restarting render loop");
                        break;
                    }
                    else {
                        Logger.Info("Event processing result: Exiting render loop");
                        Terminated = true;
                        break;
                    }
                }

                if (Headset->SessionRunning)
                {
                    RenderController->UpdateModel();

                    RenderFrame();
                }
                else {
                    // Throttle loop since xrWaitFrame won't be called.
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
        }
        catch (std::exception& ex) {
            Logger.Error("Exception during render: ", ex.what());
        }
    } // while not terminated

    Terminated = true;

    Logger.Info("OpenXR thread terminated");
}

bool OpenXrD3D11SwapChains::TryReadNextEvent(XrEventDataBuffer* buffer) const
{
    // Reset buffer header for every xrPollEvent function call.
    *buffer = { XR_TYPE_EVENT_DATA_BUFFER };
    const XrResult xr = XR_CHECK_XRCMD(xrPollEvent(Computer->Instance.Get(), buffer));
    return xr != XR_EVENT_UNAVAILABLE;
}


#if 0 // This does not work =(
// Hack to hide the OpenXR window
static LRESULT CALLBACK HackHookProc(
    int nCode,
    WPARAM wParam,
    LPARAM lParam
)
{
    if (nCode < 0) {
        return ::CallNextHookEx(NULL, nCode, wParam, lParam);
    }

    if (nCode == WM_SHOWWINDOW) {
        return -1; // BLOCK
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}
#endif


void OpenXrD3D11SwapChains::ProcessEvents(bool* exitRenderLoop, bool* requestRestart)
{
    *exitRenderLoop = *requestRestart = false;

    XrEventDataBuffer buffer{ XR_TYPE_EVENT_DATA_BUFFER };
    XrEventDataBaseHeader* header = reinterpret_cast<XrEventDataBaseHeader*>(&buffer);

    // Process all pending messages.
    while (TryReadNextEvent(&buffer))
    {
        switch (header->type) {
        case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING: {
            Logger.Debug("OpenXR event: XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING");
            *exitRenderLoop = true;
            *requestRestart = false;
            return;
        }
        case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
            const auto stateEvent = *reinterpret_cast<const XrEventDataSessionStateChanged*>(header);
            XR_CHECK(Headset->Session.Get() != XR_NULL_HANDLE && Headset->Session.Get() == stateEvent.session);
            Headset->SessionState = stateEvent.state;
            switch (stateEvent.state) {
            case XR_SESSION_STATE_IDLE: {
                Logger.Debug("Session changed: XR_SESSION_STATE_IDLE");
                break;
            }
            case XR_SESSION_STATE_READY: {
                Logger.Debug("Session changed: XR_SESSION_STATE_READY - xrBeginSession (NOT rendering)");
                XR_CHECK(Headset->Session.Get() != XR_NULL_HANDLE);

                XrSessionBeginInfo begin_info{ XR_TYPE_SESSION_BEGIN_INFO };
                begin_info.primaryViewConfigurationType = kPrimaryViewConfigurationType;

                XR_CHECK_XRCMD(xrBeginSession(Headset->Session.Get(), &begin_info));

                Headset->SessionRunning = true;
                break;
            }
            case XR_SESSION_STATE_SYNCHRONIZED: {
                Logger.Debug("Session changed: XR_SESSION_STATE_SYNCHRONIZED - NOT rendering");
                break;
            }
            case XR_SESSION_STATE_VISIBLE: {
                Logger.Debug("Session changed: XR_SESSION_STATE_VISIBLE - Rendering starting");
                break;
            }
            case XR_SESSION_STATE_FOCUSED: {
                Logger.Debug("Session changed: XR_SESSION_STATE_FOCUSED - Has input focus (and rendering)");

#if 0 // This does not work =(
                // HACK: Hide the desktop window that the OpenXR compositor forces us to display in the task tray.
                {
                    const HWND hHackWin = ::FindWindowA("WinXR Holographic Class", OPENXR_APP_NAME);
                    if (!hHackWin) {
                        Logger.Info("WinXR Holographic Class window not found: Unable to hide forced desktop window from runtime");
                    }
                    else {
                        DWORD dwThreadId = ::GetWindowThreadProcessId(hHackWin, nullptr);
                        HHOOK hHook = ::SetWindowsHookExA(
                            WH_GETMESSAGE,
                            HackHookProc,
                            ::GetModuleHandleA(nullptr),
                            dwThreadId);
                        if (!hHook) {
                            Logger.Error("WinXR Holographic Class window: Failed to install window hook: ",
                                WindowsErrorString(::GetLastError()));
                        }
                        else
                        {
                            Logger.Info("WinXR Holographic Class window: Hiding from Windows taskbar");
                            ::SetWindowLongA(hHackWin, GWL_EXSTYLE, WS_EX_NOACTIVATE);
                            //::SetWindowLongA(hHackWin, GWL_STYLE, WS_VISIBLE | WS_DISABLED);
                            // This causes rendering to stop
                            //::ShowWindow(hHackWin, SW_HIDE);
                        }
                    }
                }
#endif
                break;
            }
            case XR_SESSION_STATE_STOPPING: {
                Logger.Debug("Session changed: XR_SESSION_STATE_STOPPING - xrEndSession (rendering stopped)");

                XR_CHECK_XRCMD(xrEndSession(Headset->Session.Get()));

                Headset->SessionRunning = false;
                break;
            }
            case XR_SESSION_STATE_LOSS_PENDING: {
                Logger.Debug("Session changed: XR_SESSION_STATE_LOSS_PENDING");
                // Poll for a new systemId
                *exitRenderLoop = true;
                *requestRestart = true;
                break;
            }
            case XR_SESSION_STATE_EXITING: {
                Logger.Debug("Session changed: XR_SESSION_STATE_EXITING");
                // Do not attempt to restart because user closed this session.
                *exitRenderLoop = true;
                *requestRestart = false;
                break;
            }
            default:
                Logger.Debug("Session changed: Unhandled type ", Headset->SessionState);
                break;
            }
            break;
        }
        case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING:
        case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
        default: {
            Logger.Debug("Ignoring event type: ", header->type);
            break;
        }
        }
    }
}

void OpenXrD3D11SwapChains::RenderFrame()
{
    XR_CHECK(Headset->Session.Get() != XR_NULL_HANDLE);

    XrFrameWaitInfo wait_info{ XR_TYPE_FRAME_WAIT_INFO }; // empty
    XrFrameState frame_state{ XR_TYPE_FRAME_STATE }; // Contains predicted display time
    XR_CHECK_XRCMD(xrWaitFrame(Headset->Session.Get(), &wait_info, &frame_state));

    Rendering->PredictedDisplayTime = frame_state.predictedDisplayTime;

    XrFrameBeginInfo begin_info{ XR_TYPE_FRAME_BEGIN_INFO }; // empty
    XR_CHECK_XRCMD(xrBeginFrame(Headset->Session.Get(), &begin_info));

    Rendering->RenderedLayers.clear();

    // Only render when session is visible
    if (Headset->IsSessionVisible()) {
#if 0
        const bool key_down = (::GetAsyncKeyState(VK_RCONTROL) >> 15) != 0;

        if (key_down)
        {
            RenderFrameQuads();
        }
        else
#endif
        {
            RenderFrameViews();
        }
    }

    // Submit the composition layers for the predicted display time.

    XrFrameEndInfo end_info{ XR_TYPE_FRAME_END_INFO };
    end_info.displayTime = frame_state.predictedDisplayTime;
    end_info.environmentBlendMode = Headset->EnvironmentBlendMode;
    end_info.layerCount = (uint32_t)Rendering->RenderedLayers.size();
    end_info.layers = Rendering->RenderedLayers.data();

    XR_CHECK_XRCMD(xrEndFrame(Headset->Session.Get(), &end_info));
}

void OpenXrD3D11SwapChains::RenderFrameViews()
{
    // Read view locations:

    XrViewLocateInfo locate_info{ XR_TYPE_VIEW_LOCATE_INFO };
    locate_info.viewConfigurationType = Rendering->ViewConfigurationType;
    locate_info.displayTime = Rendering->PredictedDisplayTime;
    locate_info.space = Rendering->SceneSpace.Get();

    XrViewState view_state{ XR_TYPE_VIEW_STATE };

    uint32_t view_count = (uint32_t)Rendering->ProjectionLayer.Views.size();

    std::vector<XrView> views(view_count, { XR_TYPE_VIEW });

    XR_CHECK_XRCMD(xrLocateViews(
        Headset->Session.Get(),
        &locate_info,
        &view_state,
        view_count,
        &view_count,
        views.data()));
    XR_CHECK(view_count > 0);

    constexpr XrViewStateFlags kViewPoseValidFlags = XR_VIEW_STATE_POSITION_VALID_BIT | XR_VIEW_STATE_ORIENTATION_VALID_BIT;
    Rendering->ViewPosesValid = ((view_state.viewStateFlags & kViewPoseValidFlags) == kViewPoseValidFlags);

    if (!Rendering->ViewPosesValid) {
        Logger.Warning("View poses are not valid");
        return;
    }

    // Set up views to submit for projection layer:

    Rendering->ProjectionLayer.SubmittedViews.resize(view_count, { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW });

    Rendering->HeadPose.type = XR_TYPE_SPACE_LOCATION;
    XR_CHECK_XRCMD(xrLocateSpace(
        Rendering->HeadSpace.Get(),
        Rendering->SceneSpace.Get(),
        Rendering->PredictedDisplayTime,
        &Rendering->HeadPose));

    OpenXrToQuaternion(Rendering->HeadOrientation, Rendering->HeadPose.pose.orientation);
    OpenXrToVector(Rendering->HeadPosition, Rendering->HeadPose.pose.position);

    // ---------------------------------------------------------------------

#if 0
    XrTime now = GetTimeUsec() * 1000; // Experimental

    XR_CHECK_XRCMD(xrLocateSpace(
        Rendering->HeadSpace.Get(),
        Rendering->SceneSpace.Get(),
        now,
        &Rendering->NowPose));

    OpenXrToQuaternion(Rendering->NowOrientation, Rendering->NowPose.pose.orientation);
    OpenXrToVector(Rendering->NowPosition, Rendering->NowPose.pose.position);
#endif

    // ---------------------------------------------------------------------

    RenderView->FrameRenderStart();

    Plugins->Update();

    // ---------------------------------------------------------------------

#ifdef MONITOR_USE_POSE_FILTER
    Rendering->PoseFilter.Filter(
        views[0].pose,
        views[1].pose,
        Rendering->NowOrientation,
        Rendering->NowPosition);
#endif

    for (uint32_t i = 0; i < view_count; ++i)
    {
        auto& view = Rendering->ProjectionLayer.Views[i];

        // Set pose:

        view->Pose = views[i].pose;
        view->Fov = views[i].fov;

        // Get render target:

        uint32_t acquired_image_index = 0;
        XrSwapchainImageAcquireInfo acquire_info{ XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
        XR_CHECK_XRCMD(xrAcquireSwapchainImage(
            view->Swapchain.Swapchain.Get(),
            &acquire_info,
            &acquired_image_index));

        ScopedFunction acquire_scope([&]() {
            XrSwapchainImageReleaseInfo release_info{ XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
            XR_CHECK_XRCMD(xrReleaseSwapchainImage(
                view->Swapchain.Swapchain.Get(),
                &release_info));
            });

        XR_CHECK(acquired_image_index < view->Swapchain.Textures.size());

        XrSwapchainImageWaitInfo wait_info{ XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
        wait_info.timeout = XR_INFINITE_DURATION;
        XR_CHECK_XRCMD(xrWaitSwapchainImage(
            view->Swapchain.Swapchain.Get(),
            &wait_info));

        view->Swapchain.FrameTexture = view->Swapchain.Textures[acquired_image_index];

        // Get depth stencil:

        if (!view->DepthStencils[acquired_image_index])
        {
            D3D11_TEXTURE2D_DESC depth_desc{};
            depth_desc.Width = view->Swapchain.FirstTexDesc.Width;
            depth_desc.Height = view->Swapchain.FirstTexDesc.Height;
            depth_desc.ArraySize = view->Swapchain.FirstTexDesc.ArraySize;
            depth_desc.MipLevels = 1; // TBD
            depth_desc.Format = DXGI_FORMAT_R32_TYPELESS; // TBD
            depth_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
            depth_desc.SampleDesc.Count = view->MultiSampleCount;

            ComPtr<ID3D11Texture2D> depth_tex;
            XR_CHECK_HRCMD(Rendering->DeviceContext.Device->CreateTexture2D(
                &depth_desc,
                nullptr,
                &depth_tex));

            // Create and cache the depth stencil view.
            ComPtr<ID3D11DepthStencilView> stencil;
            CD3D11_DEPTH_STENCIL_VIEW_DESC stencil_desc(
                view->MultiSamplingEnabled ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D,
                DXGI_FORMAT_D32_FLOAT);
            XR_CHECK_HRCMD(Rendering->DeviceContext.Device->CreateDepthStencilView(
                depth_tex.Get(),
                &stencil_desc,
                &stencil));

            view->DepthStencils[acquired_image_index] = stencil;
        }

        view->DepthStencil = view->DepthStencils[acquired_image_index].Get();

        // Set up submitted view:

        auto& proj = Rendering->ProjectionLayer.SubmittedViews[i];

        proj.pose = view->Pose;
        proj.fov = view->Fov;
        proj.subImage.swapchain = view->Swapchain.Swapchain.Get();
        proj.subImage.imageRect.offset = { 0, 0 };
        proj.subImage.imageRect.extent.width = static_cast<int32_t>(view->Swapchain.Width);
        proj.subImage.imageRect.extent.height = static_cast<int32_t>(view->Swapchain.Height);

        // ---------------------------------------------------------------------

        // Render to this view
        RenderView->RenderProjectiveView(view.get());

        // ---------------------------------------------------------------------

        if (view->MultiSamplingEnabled)
        {
            // Resolve MSAA to the swapchain frame texture
            Rendering->DeviceContext.Context->ResolveSubresource(
                view->Swapchain.FrameTexture,
                0,
                view->MultisampleRT.Get(),
                0,
                Rendering->SwapchainFormat);
        }
    }

    auto& submit_layer = Rendering->ProjectionLayer.SubmittedLayer;

    submit_layer = { XR_TYPE_COMPOSITION_LAYER_PROJECTION };
    // TBD:
    // XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT
    // XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT
    submit_layer.layerFlags = 0;
    submit_layer.space = Rendering->SceneSpace.Get();
    submit_layer.viewCount = view_count;
    submit_layer.views = Rendering->ProjectionLayer.SubmittedViews.data();

    Rendering->RenderedLayers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&submit_layer));
}

void OpenXrD3D11SwapChains::RenderFrameQuads()
{
#if 0
    const unsigned monitor_count = RenderModel->Monitors.size();

    if (monitor_count == 0) {
        Rendering->QuadLayers.clear();
        return;
    }

    // Update quad layers to match monitor sizes:

    Rendering->QuadLayers.resize(monitor_count);

    for (unsigned i = 0; i < monitor_count; ++i)
    {
        auto& monitor = RenderModel->Monitors[i];

        if (!monitor->Dupe->VrRenderTexture) {
            continue;
        }

        const unsigned width = monitor->Dupe->VrRenderTextureDesc.Width;
        const unsigned height = monitor->Dupe->VrRenderTextureDesc.Height;

        auto& quad = Rendering->QuadLayers[i];

        // If the swapchain extent does not match the dupe texture:
        if (!quad || quad->Swapchain.Width != width || quad->Swapchain.Height != height)
        {
            quad.reset();

            quad = std::make_shared<XrD3D11QuadLayerSwapchain>();

            CreateSwapchain(
                quad->Swapchain,
                width,
                height);
        }

        // Get render target:

        uint32_t acquired_image_index = 0;
        XrSwapchainImageAcquireInfo acquire_info{ XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
        XR_CHECK_XRCMD(xrAcquireSwapchainImage(
            quad->Swapchain.Swapchain.Get(),
            &acquire_info,
            &acquired_image_index));

        ScopedFunction acquire_scope([&]() {
            XrSwapchainImageReleaseInfo release_info{ XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
            XR_CHECK_XRCMD(xrReleaseSwapchainImage(
                quad->Swapchain.Swapchain.Get(),
                &release_info));
        });

        XR_CHECK(acquired_image_index < quad->Swapchain.Textures.size());

        XrSwapchainImageWaitInfo wait_info{ XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
        wait_info.timeout = XR_INFINITE_DURATION;
        XR_CHECK_XRCMD(xrWaitSwapchainImage(
            quad->Swapchain.Swapchain.Get(),
            &wait_info));

        quad->Swapchain.FrameTexture = quad->Swapchain.Textures[acquired_image_index];

        // Render to texture:

        RenderView->RenderMonitorToQuadLayer(quad.get(), monitor.get());

        // Add quad layer to render layer list:

        // Note: Adding additional layer flags does not seem to improve quality
        quad->QuadLayer.layerFlags = 0; // XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT | XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT;
        quad->QuadLayer.space = Rendering->SceneSpace.Get();
        quad->QuadLayer.eyeVisibility = XR_EYE_VISIBILITY_BOTH;
        quad->QuadLayer.subImage.imageArrayIndex = 0;
        quad->QuadLayer.subImage.imageRect.extent = {
            static_cast<int32_t>(width),
            static_cast<int32_t>(height)
        };
        quad->QuadLayer.subImage.imageRect.offset = { 0, 0 };
        quad->QuadLayer.subImage.swapchain = quad->Swapchain.Swapchain.Get();

        OpenXrFromQuaternion(quad->QuadLayer.pose.orientation, monitor->Orientation);
        OpenXrFromVector(quad->QuadLayer.pose.position, monitor->Position);

        quad->QuadLayer.size.x = monitor->Size.x;
        quad->QuadLayer.size.y = monitor->Size.y;

        Rendering->RenderedLayers.push_back(
            reinterpret_cast<XrCompositionLayerBaseHeader*>(&quad->QuadLayer));
    }
#endif
}


//------------------------------------------------------------------------------
// OpenXrD3D11SwapChains : Computer Properties

bool OpenXrD3D11SwapChains::Initialize(
    MonitorRenderController* render_controller,
    MonitorRenderModel* render_model,
    HeadsetCameraCalibration* camera_calibration)
{
    RenderController = render_controller;
    RenderModel = render_model;
    CameraCalibration = camera_calibration;

    // Make sure everything is reset
    RenderView.reset();
    Plugins.reset();
    Rendering.reset();
    Headset.reset();
    Computer.reset();

    if (!CreateInstanceAndLogger()) {
        Logger.Error("Failed to create OpenXR instance or log messenger");
        return false;
    }

    StartThread();
    return true;
}

bool OpenXrD3D11SwapChains::CreateInstanceAndLogger()
{
    try {
        XR_CHECK(!Computer);

        Computer = std::make_unique<XrComputerProperties>();

        // Build out the extensions to enable. Some extensions are required and some are optional.
        const std::vector<const char*> enabledExtensions = SelectExtensions();

        // Create the instance with desired extensions.
        XrInstanceCreateInfo createInfo{ XR_TYPE_INSTANCE_CREATE_INFO };
        createInfo.enabledExtensionCount = (uint32_t)enabledExtensions.size();
        createInfo.enabledExtensionNames = enabledExtensions.data();
        createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
        strcpy_s(createInfo.applicationInfo.applicationName, OPENXR_APP_NAME);
        createInfo.applicationInfo.applicationVersion = OPENXR_APP_VERSION;
        strcpy_s(createInfo.applicationInfo.engineName, OPENXR_ENGINE_NAME);
        createInfo.applicationInfo.engineVersion = OPENXR_APP_VERSION;

        XR_CHECK_XRCMD(xrCreateInstance(
            &createInfo,
            Computer->Instance.Put()));

#if 0
        // Set up logging messenger:
        XR_CHECK(Computer->Messenger.Get() == XR_NULL_HANDLE);
#if 0
        XrDebugUtilsObjectNameInfoEXT logn{ XR_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
        logn.objectName = m_applicationName.c_str();
        logn.objectType = XR_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT;
        CHECK_XRCMD(LoaderXrTermSetDebugUtilsObjectNameEXT(m_instance.Get(), &logn));
#endif
        XrDebugUtilsMessengerCreateInfoEXT logc{ XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
        logc.messageSeverities = XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        logc.messageSeverities |= XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
        logc.messageSeverities |= XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
        logc.messageSeverities |= XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
        logc.messageTypes = XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
        logc.messageTypes |= XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
        logc.messageTypes |= XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        logc.userCallback = [](XrDebugUtilsMessageSeverityFlagsEXT messageSeverity,
            XrDebugUtilsMessageTypeFlagsEXT messageTypes,
            const XrDebugUtilsMessengerCallbackDataEXT* callbackData,
            void* userData) -> XrBool32
        {
            OpenXrD3D11SwapChains* thiz = (OpenXrD3D11SwapChains*)userData;

            ModuleLogger.Info("OpenXR: ", callbackData->message);

            return XR_TRUE;
        };
        logc.userData = this;

        XR_CHECK_XRCMD(LoaderXrTermCreateDebugUtilsMessengerEXT(
            Computer->Instance.Get(),
            &logc,
            Computer->Messenger.Put()));
#endif

        return true;
    }
    catch (std::exception& ex) {
        Logger.Error("Exception while creating instance: ", ex.what());
    }

    return false;
}

std::vector<const char*> OpenXrD3D11SwapChains::SelectExtensions()
{
    // Fetch the list of extensions supported by the runtime.
    uint32_t extensionCount;
    XR_CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr));
    std::vector<XrExtensionProperties> extensionProperties(extensionCount, {XR_TYPE_EXTENSION_PROPERTIES});
    XR_CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(nullptr, extensionCount, &extensionCount, extensionProperties.data()));

    std::vector<const char*> enabledExtensions;

    // Add a specific extension to the list of extensions to be enabled, if it is supported.
    auto AddExtIfSupported = [&](const char* extensionName)
    {
        for (uint32_t i = 0; i < extensionCount; i++) {
            if (strcmp(extensionProperties[i].extensionName, extensionName) == 0) {
                enabledExtensions.push_back(extensionName);
                return true;
            }
        }
        return false;
    };

    // D3D11 extension is required so check that it was added.
    XR_CHECK(AddExtIfSupported(XR_KHR_D3D11_ENABLE_EXTENSION_NAME));

    // Additional optional extensions for enhanced functionality. Track whether enabled in m_optionalExtensions.
    Computer->UnboundedRefSpaceSupported = AddExtIfSupported(XR_MSFT_UNBOUNDED_REFERENCE_SPACE_EXTENSION_NAME);
    Computer->SpatialAnchorSupported = AddExtIfSupported(XR_MSFT_SPATIAL_ANCHOR_EXTENSION_NAME);

    // False- Microsoft does not support cylindrical layers
    Computer->CylinderSupported = AddExtIfSupported(XR_KHR_COMPOSITION_LAYER_CYLINDER_EXTENSION_NAME);

    Logger.Debug("OpenXR cylindrical layer supported: ", Computer->CylinderSupported);

    return enabledExtensions;
}


//------------------------------------------------------------------------------
// OpenXrD3D11SwapChains : Headset Properties

bool OpenXrD3D11SwapChains::DetectHeadsetSystemId()
{
    XR_CHECK(Computer->Instance.Get() != XR_NULL_HANDLE);
    XR_CHECK(Headset->SystemId == XR_NULL_SYSTEM_ID);

    XrSystemGetInfo get_info{XR_TYPE_SYSTEM_GET_INFO};
    get_info.formFactor = kFormFactor;

    XrResult result = xrGetSystem(
        Computer->Instance.Get(),
        &get_info,
        &Headset->SystemId);

    if (FAILED(result))
    {
        if (result == XR_ERROR_FORM_FACTOR_UNAVAILABLE) {
            return false; // Headset not plugged in
        }
        XR_CHECK_XRRESULT(result, "xrGetSystem");
        return false; // Should never get here
    }

    return true;
}

XrEnvironmentBlendMode OpenXrD3D11SwapChains::SelectEnvironmentBlendMode()
{
    // Fetch the list of supported environment blend mode of give system
    uint32_t count = 0;
    XR_CHECK_XRCMD(xrEnumerateEnvironmentBlendModes(
        Computer->Instance.Get(),
        Headset->SystemId,
        XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
        0,
        &count,
        nullptr));
    XR_CHECK(count > 0);

    std::vector<XrEnvironmentBlendMode> environmentBlendModes(count);
    XR_CHECK_XRCMD(xrEnumerateEnvironmentBlendModes(
        Computer->Instance.Get(),
        Headset->SystemId,
        XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
        count,
        &count,
        environmentBlendModes.data()));

    for (auto mode : environmentBlendModes) {
        if (mode == XR_ENVIRONMENT_BLEND_MODE_OPAQUE) {
            return mode;
        }
    }

    for (auto mode : environmentBlendModes) {
        if (mode == XR_ENVIRONMENT_BLEND_MODE_ADDITIVE) {
            return mode;
        }
    }

    return environmentBlendModes[0];
}

void OpenXrD3D11SwapChains::InitializeHeadsetRendering()
{
    Headset->EnvironmentBlendMode = SelectEnvironmentBlendMode();

    // Read headset system information:

    XrSystemProperties system_properties{ XR_TYPE_SYSTEM_PROPERTIES };
    XR_CHECK_XRCMD(xrGetSystemProperties(
        Computer->Instance.Get(),
        Headset->SystemId,
        &system_properties));

    Headset->HeadsetVendorId = system_properties.vendorId;

    system_properties.systemName[sizeof(system_properties.systemName) - 1] = '\0';

    Headset->HeadsetModel = GetHeadsetModel();

    Headset->SystemGraphicsProperties = system_properties.graphicsProperties;
    Headset->SystemTrackingProperties = system_properties.trackingProperties;

    Logger.Info("OpenXR headset information for model: `", Headset->HeadsetModel, "` [VendorId:", HexString(Headset->HeadsetVendorId), "]");
    Logger.Info(" *           maxLayerCount: ", Headset->SystemGraphicsProperties.maxLayerCount);
    Logger.Info(" *  maxSwapchainImageWidth: ", Headset->SystemGraphicsProperties.maxSwapchainImageWidth);
    Logger.Info(" * maxSwapchainImageHeight: ", Headset->SystemGraphicsProperties.maxSwapchainImageHeight);
    Logger.Info(" *     orientationTracking: ", Headset->SystemTrackingProperties.orientationTracking);
    Logger.Info(" *        positionTracking: ", Headset->SystemTrackingProperties.positionTracking);

#if 0
    // This is fixed now!
    Headset->NeedsReverbColorHack = StrIStr(Headset->HeadsetModel.c_str(), "reverb") != 0;
    if (Headset->NeedsReverbColorHack) {
        Logger.Info("Headset needs Reverb color hack to fix high-end");
    }
#endif

    // Get view configurations supported:

    uint32_t view_config_count = 0;

    XR_CHECK_XRCMD(xrEnumerateViewConfigurations(
        Computer->Instance.Get(),
        Headset->SystemId,
        0,
        &view_config_count,
        nullptr));

    std::vector<XrViewConfigurationType> view_configs(view_config_count);

    XR_CHECK_XRCMD(xrEnumerateViewConfigurations(
        Computer->Instance.Get(),
        Headset->SystemId,
        view_config_count,
        &view_config_count,
        view_configs.data()));
    CORE_DEBUG_ASSERT(view_config_count == view_configs.size());

    Rendering->ViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO;

    Logger.Info("Supports ", view_config_count, " view configurations:");
    for (auto config : view_configs)
    {
        switch (config)
        {
        case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO:
            Logger.Info(" * Mono");
            break;
        case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO:
            Logger.Info(" * Stereo (Selected)");
            Rendering->ViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
            break;
        case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_QUAD_VARJO:
            Logger.Info(" * Quad-Varjo");
            break;
        default:
            Logger.Info(" * Unknown #", config);
            break;
        }
    }

    XrViewConfigurationProperties view_config_props{ XR_TYPE_VIEW_CONFIGURATION_PROPERTIES };
    XR_CHECK_XRCMD(xrGetViewConfigurationProperties(
        Computer->Instance.Get(),
        Headset->SystemId,
        Rendering->ViewConfigurationType,
        &view_config_props));
    if (view_config_props.fovMutable) {
        Logger.Info(" * FOV may change between frames for selected view type");
    }
    else {
        Logger.Info(" * FOV will NOT change between frames for selected view type");
    }

    // Get adapter information:

    // Create the D3D11 device for the adapter associated with the system.
    XrGraphicsRequirementsD3D11KHR requirements{ XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR };
    XR_CHECK_XRCMD(xrGetD3D11GraphicsRequirementsKHR(
        Computer->Instance.Get(),
        Headset->SystemId,
        &requirements));

    Headset->AdapterLuid = requirements.adapterLuid;

    Logger.Info("Attached graphics adapter LUID: ", LUIDToString(Headset->AdapterLuid));

    // Create Device Context:

    Logger.Info("Creating D3D11 Device for attached adapter..");

    const ComPtr<IDXGIAdapter1> adapter = GetAdapterForLuid(Headset->AdapterLuid);
    XR_CHECK(adapter != nullptr);

    std::vector<D3D_FEATURE_LEVEL> feature_levels = {
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };

    feature_levels.erase(std::remove_if(feature_levels.begin(),
        feature_levels.end(),
        [&](D3D_FEATURE_LEVEL fl) { return fl < requirements.minFeatureLevel; }),
        feature_levels.end());
    XR_CHECK_MSG(!feature_levels.empty(), "Headset D3D minimum feature level cannot be satisfied");

    bool device_context_created = Rendering->DeviceContext.CreateForAdapter(
        adapter.Get(),
        feature_levels);
    XR_CHECK(device_context_created);

    // Create Session:

    Logger.Info("Creating OpenXR session..");

    Headset->GraphicsBinding.device = Rendering->DeviceContext.Device.Get();

    XrSessionCreateInfo session_create_info{XR_TYPE_SESSION_CREATE_INFO};
    session_create_info.next = &Headset->GraphicsBinding;
    session_create_info.systemId = Headset->SystemId;

    XR_CHECK_XRCMD(xrCreateSession(
        Computer->Instance.Get(),
        &session_create_info,
        Headset->Session.Put()));

    // Create Space:

    Logger.Info("Creating OpenXR Space..");

    // Choose space type
    XrReferenceSpaceType space_type = XR_REFERENCE_SPACE_TYPE_LOCAL;
    if (Computer->UnboundedRefSpaceSupported) {
        // Unbounded reference space provides the best scene space for world-scale experiences.
        space_type = XR_REFERENCE_SPACE_TYPE_UNBOUNDED_MSFT;
    }

    XrReferenceSpaceCreateInfo space_create_info{ XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
    space_create_info.referenceSpaceType = space_type;
    space_create_info.poseInReferenceSpace = Pose::Identity();

    XR_CHECK_XRCMD(xrCreateReferenceSpace(
        Headset->Session.Get(),
        &space_create_info,
        Rendering->SceneSpace.Put()));

    space_create_info.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;

    XR_CHECK_XRCMD(xrCreateReferenceSpace(
        Headset->Session.Get(),
        &space_create_info,
        Rendering->HeadSpace.Put()));

    CreateSwapchains();

    // If we got here then initialization was successful!

    Logger.Info("Headset rendering initialization succeeded!");

    RenderView->OnHeadsetRenderingInitialized(Imager.get());

    // Initialize plugins

    Plugins->Initialize(
        Headset.get(),
        Rendering.get(),
        RenderModel);
}

DXGI_FORMAT OpenXrD3D11SwapChains::SelectSwapchainFormat()
{
    uint32_t format_count = 0;
    XR_CHECK_XRCMD(xrEnumerateSwapchainFormats(
        Headset->Session.Get(),
        0,
        &format_count,
        nullptr));
    XR_CHECK(format_count > 0);

    std::vector<int64_t> formats(format_count);
    XR_CHECK_XRCMD(xrEnumerateSwapchainFormats(
        Headset->Session.Get(),
        (uint32_t)formats.size(),
        &format_count,
        formats.data()));
    XR_CHECK(format_count > 0);
    formats.resize(format_count);

    // The swapchain must be DXGI_FORMAT_B8G8R8A8_UNORM because that is the desktop capture format.
    // TBD: Can this change?

    constexpr DXGI_FORMAT kSupportedColorSwapchainFormats[] = {
        DXGI_FORMAT_B8G8R8A8_UNORM,
        DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
        DXGI_FORMAT_R8G8B8A8_UNORM,      ///< Incompatible with desktop!
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, ///< Incompatible with desktop!
    };

    // For each requested format, in priority order:
    for (DXGI_FORMAT supported : kSupportedColorSwapchainFormats) {
        // If it is supported:
        for (int64_t format : formats) {
            if (format == supported) {
                return supported;
            }
        }
    }

    XR_THROW("DXGI_FORMAT_B8G8R8A8_UNORM swapchain format is not supported");
    return DXGI_FORMAT_UNKNOWN;
}

void OpenXrD3D11SwapChains::CreateSwapchain(XrD3D11Swapchain& swapchain, const XrViewConfigurationView& view_config)
{
    uint32_t width = static_cast<uint32_t>(view_config.recommendedImageRectWidth * SsaaFactor);
    uint32_t height = static_cast<uint32_t>(view_config.recommendedImageRectHeight * SsaaFactor);

#if 1
    if (width > view_config.maxImageRectWidth) {
        Logger.Debug("Cannot increase swapchain width further");
        width = view_config.maxImageRectWidth;
    }
    if (height > view_config.maxImageRectHeight) {
        Logger.Debug("Cannot increase swapchain height further");
        height = view_config.maxImageRectHeight;
    }
#endif

    CreateSwapchain(swapchain, width, height);
}

void OpenXrD3D11SwapChains::CreateSwapchain(
    XrD3D11Swapchain& swapchain,
    uint32_t width,
    uint32_t height)
{
    // Create the XrSwapchain object:

    XrSwapchainCreateInfo create_info{ XR_TYPE_SWAPCHAIN_CREATE_INFO };
    create_info.arraySize = 1;
    create_info.format = Rendering->SwapchainFormat;
    create_info.width = width;
    create_info.height = height;
    create_info.mipCount = kDupeMipLevels;
    create_info.faceCount = 1;
    create_info.sampleCount = 1;

    // MSAA render targets are not supported by Microsoft OpenXR.
    // Instead OpenXR provides sample=1 backbuffers that we must resolve to.
    // This is probably better for texture memory because OpenXR provides 3 render textures
    // for each backbuffer, while we only need one multisample render target.
    // It does provide MSAA for quad layers but that defeats the whole purpose.

    // TBD: Can we eliminate some of these for perf?
    create_info.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;

    swapchain.Width = width;
    swapchain.Height = height;

    Logger.Info("Requesting swapchain resolution: ", create_info.width, "x", create_info.height);

    XR_CHECK_XRCMD(xrCreateSwapchain(
        Headset->Session.Get(),
        &create_info,
        swapchain.Swapchain.Put()));

    // Get swapchain images:

    uint32_t image_count = 0;
    XR_CHECK_XRCMD(xrEnumerateSwapchainImages(
        swapchain.Swapchain.Get(),
        0,
        &image_count,
        nullptr));
    XR_CHECK(image_count > 0);

    std::vector<XrSwapchainImageD3D11KHR> images(image_count, { XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR });

    XR_CHECK_XRCMD(xrEnumerateSwapchainImages(
        swapchain.Swapchain.Get(),
        image_count,
        &image_count,
        reinterpret_cast<XrSwapchainImageBaseHeader*>(images.data())));
    XR_CHECK(image_count == images.size());

    swapchain.Textures.resize(image_count, nullptr);

    // Grab the textures
    for (unsigned i = 0; i < image_count; ++i) {
        swapchain.Textures[i] = images[i].texture;
    }

    // Grab the descriptor for the first texture
    images[0].texture->GetDesc(&swapchain.FirstTexDesc);
}

void OpenXrD3D11SwapChains::CreateSwapchains()
{
    Rendering->SwapchainFormat = SelectSwapchainFormat();

    // Query view configuration views:

    uint32_t view_count = 0;
    XR_CHECK_XRCMD(xrEnumerateViewConfigurationViews(
        Computer->Instance.Get(),
        Headset->SystemId,
        kPrimaryViewConfigurationType,
        0,
        &view_count,
        nullptr));
    XR_CHECK(view_count > 0);

    std::vector<XrViewConfigurationView> config_views(view_count, { XR_TYPE_VIEW_CONFIGURATION_VIEW });

    XR_CHECK_XRCMD(xrEnumerateViewConfigurationViews(
        Computer->Instance.Get(),
        Headset->SystemId,
        kPrimaryViewConfigurationType,
        view_count,
        &view_count,
        config_views.data()));
    XR_CHECK(view_count > 0);
    config_views.resize(view_count);

    Rendering->ProjectionLayer.Views.clear();
    Rendering->ProjectionLayer.Views.resize(view_count);

    for (uint32_t i = 0; i < view_count; i++)
    {
        std::shared_ptr<XrD3D11ProjectionView> view = std::make_shared<XrD3D11ProjectionView>();

        CreateSwapchain(view->Swapchain, config_views[i]);

        CreateMultisampleRenderTarget(view);

        view->DepthStencils.resize(view->Swapchain.Textures.size());

        view->ViewIndex = i;
        Rendering->ProjectionLayer.Views[i] = view;
    }
}

void OpenXrD3D11SwapChains::CreateMultisampleRenderTarget(
    std::shared_ptr<XrD3D11ProjectionView>& view)
{
    if (TargetSampleCount <= 1) {
        Logger.Info("MSAA disabled by configuration");
    }

    unsigned sample_count = 0;

    DXGI_FORMAT format_index = Rendering->SwapchainFormat;
    UINT format_support = 0;

    XR_CHECK_HRCMD(Rendering->DeviceContext.Device->CheckFormatSupport(
        format_index,
        &format_support));

    if ((format_support & D3D11_FORMAT_SUPPORT_MULTISAMPLE_RESOLVE) != 0 &&
        (format_support & D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET) != 0)
    {
        Logger.Info("Format ", format_index, " supports MSAA");

        static const unsigned kMaxSampleCount = 8;

        for (unsigned j = 1; j <= kMaxSampleCount; j *= 2)
        {
            UINT num_quality_levels = 0;

            XR_CHECK_HRCMD(Rendering->DeviceContext.Device->CheckMultisampleQualityLevels(
                format_index,
                j,
                &num_quality_levels));

            if (num_quality_levels > 0) {
                sample_count = j;

                if (sample_count >= TargetSampleCount) {
                    break;
                }
            }
        }
    }

    if (sample_count == 0) {
        Logger.Info("MSAA is not supported for texture format ", format_index, " on this platform");

        view->MultiSamplingEnabled = false;
        view->MultiSampleCount = 1;

        return;
    }

    Logger.Info("Enabling ", sample_count, "xMSAA");

    D3D11_TEXTURE2D_DESC rt_desc{};

    rt_desc.Format = Rendering->SwapchainFormat;
    rt_desc.Width = static_cast<UINT>(view->Swapchain.Width);
    rt_desc.Height = static_cast<UINT>(view->Swapchain.Height);
    rt_desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    rt_desc.MipLevels = 1;
    rt_desc.ArraySize = 1;
    rt_desc.SampleDesc.Count = sample_count;
    rt_desc.SampleDesc.Quality = 0; // D3D11_STANDARD_MULTISAMPLE_PATTERN;

    // Create a surface that's multisampled.
    XR_CHECK_HRCMD(Rendering->DeviceContext.Device->CreateTexture2D(
        &rt_desc,
        nullptr,
        &view->MultisampleRT));

    CD3D11_RENDER_TARGET_VIEW_DESC rtv_desc(D3D11_RTV_DIMENSION_TEXTURE2DMS);

    XR_CHECK_HRCMD(Rendering->DeviceContext.Device->CreateRenderTargetView(
        view->MultisampleRT.Get(),
        &rtv_desc,
        &view->MultisampleRTV));
}


} // namespace xrm
