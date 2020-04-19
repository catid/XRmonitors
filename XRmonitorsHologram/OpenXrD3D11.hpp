// Copyright 2019 Augmented Perception Corporation

#pragma once

// OpenXR vendored headers
#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D11
#include "openxr/openxr.h"
#include "openxr/openxr_platform.h"
#include "openxr/openxr_platform_defines.h"

// OpenXR generated headers - Needed for debug messenger
#include "MsXrLoader/xr_generated_dispatch_table.h"
#include "MsXrLoader/xr_generated_loader.hpp"

#include "XrUtility/XrHandle.h"
#include "XrUtility/XrError.h"
#include "XrUtility/XrMath.h"

#include "D3D11Tools.hpp"

#include "core_logger.hpp"

#include <vector>
#include <memory>

namespace xrm {

using namespace core;
using namespace DirectX::SimpleMath;


//--------------------------------------------------------------------------
// Constants

// Attempt to use pose filter to reduce aliasing artifacts
//#define MONITOR_USE_POSE_FILTER

static const XrFormFactor kFormFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

static const XrViewConfigurationType \
    kPrimaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

#define OPENXR_APP_NAME     "XRmonitorsHologram"
#define OPENXR_APP_VERSION  100
#define OPENXR_ENGINE_NAME  "XRmonitors"

// We extend the Microsoft code a bit to allow for our own debug message handler
using XrDebugMessengerHandle = XrHandle<XrDebugUtilsMessengerEXT, LoaderXrTermDestroyDebugUtilsMessengerEXT>;


//------------------------------------------------------------------------------
// Tools

void OpenXrToQuaternion(
    DirectX::SimpleMath::Quaternion& to,
    const XrQuaternionf& from);

void OpenXrToVector(
    DirectX::SimpleMath::Vector3& to,
    const XrVector3f& from);

void OpenXrFromQuaternion(
    XrQuaternionf& to,
    const DirectX::SimpleMath::Quaternion& from);

void OpenXrFromVector(
    XrVector3f& to,
    const DirectX::SimpleMath::Vector3& from);


//------------------------------------------------------------------------------
// XrD3D11Swapchain

struct XrD3D11Swapchain
{
    // Swapchain extents
    uint32_t Width = 0;
    uint32_t Height = 0;

    // Swapchain handle for each view
    XrSwapchainHandle Swapchain;

    // D3D11 textures for each image
    std::vector<ID3D11Texture2D*> Textures;

    // Descriptor for the first texture in the vector
    D3D11_TEXTURE2D_DESC FirstTexDesc{};

    // Texture render target for the current frame
    ID3D11Texture2D* FrameTexture = nullptr;
};


//------------------------------------------------------------------------------
// PoseAliasingFilter

class PoseAliasingFilter
{
public:
    void Filter(
        const XrPosef& pose0,
        const XrPosef& pose1,
        const DirectX::SimpleMath::Quaternion& head_orientation,
        const DirectX::SimpleMath::Vector3& head_position);

    XrPosef FilteredPoses[2];

protected:
    DirectX::SimpleMath::Vector3 LastAcceptedHeadOrientationResult;
    DirectX::SimpleMath::Vector3 LastAcceptedHeadPosition;
};


//------------------------------------------------------------------------------
// XrD3D11ProjectionView

struct XrD3D11ProjectionView
{
    uint32_t ViewIndex = 0;

    XrD3D11Swapchain Swapchain;

    // Valid if Rendering->ViewPosesValid is true
    XrPosef Pose;
    XrFovf Fov;

    // Depth stencil associated with each texture.
    // This is populated on demand and is initially the correct size but all
    // entries are null
    std::vector<ComPtr<ID3D11DepthStencilView>> DepthStencils;

    // Depth stencil for the current frame
    ID3D11DepthStencilView* DepthStencil = nullptr;

    // Is multi-sampling (MSAA) enabled?
    bool MultiSamplingEnabled = false;

    // MSAA sample count
    uint32_t MultiSampleCount = 1;

    // Multisample intermediate render target
    ComPtr<ID3D11Texture2D> MultisampleRT;
    ComPtr<ID3D11RenderTargetView> MultisampleRTV;
};


//------------------------------------------------------------------------------
// XrD3D11ProjectionLayer

struct XrD3D11ProjectionLayer
{
    // Layer submitted to xrEndFrame()
    XrCompositionLayerProjection SubmittedLayer{ XR_TYPE_COMPOSITION_LAYER_PROJECTION };

    // Views submitted to xrEndFrame()
    std::vector<XrCompositionLayerProjectionView> SubmittedViews;

    // Views requested by OS, with their swapchains
    std::vector<std::shared_ptr<XrD3D11ProjectionView>> Views;
};


//------------------------------------------------------------------------------
// XrD3D11QuadLayerSwapchain

struct XrD3D11QuadLayerSwapchain
{
    // Composition layer object for rendering
    XrCompositionLayerQuad QuadLayer{ XR_TYPE_COMPOSITION_LAYER_QUAD };

    // Swapchain for this quad layer
    XrD3D11Swapchain Swapchain;
};


//------------------------------------------------------------------------------
// XrComputerProperties

// These persist between replugging the headset
struct XrComputerProperties
{
    // Optional extensions available?
    bool UnboundedRefSpaceSupported = false;
    bool SpatialAnchorSupported = false;
    bool CylinderSupported = false;

    // Instance data about our application (engine name, enabled exts, etc)
    XrInstanceHandle Instance;

    XrPath GetXrPath(std::string_view string) const
    {
        XrPath path;
        XR_CHECK_XRCMD(xrStringToPath(Instance.Get(), string.data(), &path));
        return path;
    }

    // OpenXR debugging callbacks
    XrDebugMessengerHandle Messenger;
};


//------------------------------------------------------------------------------
// XrHeadsetProperties

// These are reset each time the headset is replugged
struct XrHeadsetProperties
{
    //--------------------------------------------------------------------------
    // Headset Info

    // Headset System Id - This identifies one attached headset
    uint64_t SystemId = XR_NULL_SYSTEM_ID;

    // Configured blend mode based on available modes (opaque, additive for AR, etc)
    XrEnvironmentBlendMode EnvironmentBlendMode{};

    // Vendor-provided headset vendor id
    uint32_t HeadsetVendorId = 0;

    // Vendor-provided headset model name.
    // This is identical to the one provided by Windows Holographic API
    std::string HeadsetModel;

    // The HP Reverb mura correction has a bug where bright colors are distorted.
    // This hack rescales color so it looks correct
    bool NeedsReverbColorHack = false;

    // Limits on the swapchain images, max view/layer count
    XrSystemGraphicsProperties SystemGraphicsProperties;
    XrSystemTrackingProperties SystemTrackingProperties;

    // LUID for the graphics adapter connected to the HMD
    LUID AdapterLuid{};


    //--------------------------------------------------------------------------
    // OpenXR Session

    // TBD: Not sure if we need this to be persistent beyond xrCreateSession() call?
    XrGraphicsBindingD3D11KHR GraphicsBinding{ XR_TYPE_GRAPHICS_BINDING_D3D11_KHR };

    // Session handle - Created based on the SystemId
    XrSessionHandle Session;

    // Based on OpenXR events: Are we connected to the compositor?
    bool SessionRunning = false;

    // Current OpenXR session state
    XrSessionState SessionState{ XR_SESSION_STATE_UNKNOWN };


    // Should this application render?
    constexpr bool IsSessionVisible() const
    {
        return SessionState == XR_SESSION_STATE_VISIBLE || SessionState == XR_SESSION_STATE_FOCUSED;
    }

    // Does this application have focus?
    constexpr bool IsSessionFocused() const
    {
        return SessionState == XR_SESSION_STATE_FOCUSED;
    }
};


//------------------------------------------------------------------------------
// XrRenderProperties

// This answers the question "How do we render the RenderModel within the RenderView?"
struct XrRenderProperties
{
    // Main scene space created from session
    XrSpaceHandle SceneSpace;

    // View-relative pose
    XrSpaceHandle HeadSpace;

    // Head pose predicted for render time
    XrSpaceRelation HeadPose;
    DirectX::SimpleMath::Quaternion HeadOrientation; ///< Parsed from HeadPose
    DirectX::SimpleMath::Vector3 HeadPosition; ///< Parsed from HeadPose

#ifdef MONITOR_USE_POSE_FILTER
    // Filter for poses to reduce aliasing crawl
    PoseAliasingFilter PoseFilter;
#endif

    // D3D11 device created to target the headset
    D3D11DeviceContext DeviceContext;

    // Format for the swapchain render target textures
    DXGI_FORMAT SwapchainFormat = DXGI_FORMAT_UNKNOWN;

    // Predicted display time for the current frame
    XrTime PredictedDisplayTime;

    // Is the tracking pose valid for this view for this frame?
    bool ViewPosesValid = false;

    // There is one traditional 3D projection base layer
    XrD3D11ProjectionLayer ProjectionLayer;

    // There are multiple quad layers rendered at higher quality
    std::vector<std::shared_ptr<XrD3D11QuadLayerSwapchain>> QuadLayers;

    // These are submitted to xrEndFrame() each frame
    std::vector<XrCompositionLayerBaseHeader*> RenderedLayers;
};


} // namespace xrm
