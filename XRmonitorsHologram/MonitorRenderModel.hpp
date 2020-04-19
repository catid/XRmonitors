// Copyright 2019 Augmented Perception Corporation

#pragma once

#include "MonitorTools.hpp"
#include "D3D11DuplicationCommon.hpp"
#include "CameraClient.hpp"
#include "xrm_plugins_abi.hpp"

#include <SimpleMath.h> // Quaternion

namespace xrm {

using namespace DirectX::SimpleMath;


//------------------------------------------------------------------------------
// Constants

#define XRM_MAX_DPI 80.f
#define XRM_MIN_DPI 1.f
#define XRM_METERS_PER_INCH 0.0254f


//------------------------------------------------------------------------------
// MonitorRenderState

// State for each monitor
struct MonitorRenderState
{
    // Enumeration information about monitor
    MonitorEnumInfo* MonitorInfo = nullptr;

    // This contains the render texture for the desktop
    ID3D11DesktopDuplication* Dupe = nullptr;

#if 0
    // Quad position in space
    DirectX::SimpleMath::Quaternion Orientation;
    Vector3 Position;
    Vector2 Size;
#endif

    // Cylinder geometry for each monitor
    ComPtr<ID3D11Buffer> VertexBuffer;
    ComPtr<ID3D11Buffer> IndexBuffer;
    int IndexCount = 0;
};


//------------------------------------------------------------------------------
// PluginRenderInfo

struct PluginRenderInfo
{
    // Set by application:

    bool EnableBlueLightFilter = false;

    // Set by Read() function:

    ID3D11Texture2D* Texture = nullptr;
    int X = 0;
    int Y = 0;
    int Width = 0;
    int Height = 0;

    DXGI_FORMAT TextureFormat;
    bool MultisamplingEnabled = false;

    // Geometry
    ID3D11Buffer* VertexBuffer = nullptr;
    ID3D11Buffer* IndexBuffer = nullptr;
    unsigned IndexCount = 0;
};


//------------------------------------------------------------------------------
// MonitorRenderModel

/*
    This describes everything to render each frame, set up before each frame
    by the MonitorRenderController.
*/
struct MonitorRenderModel
{
    XrmUiData UiData{};

    // Based on HMD type
    float HmdFocalDistanceMeters = 1.4f;

    // Dots (pixels) per inch
    float DPI = XRM_MAX_DPI;
    float PixelsPerMeter = 0.f;
    float MetersPerPixel = 0.0001f;

    DirectX::SimpleMath::Quaternion RecenterOrientation;
    Vector3 RecenterPosition;

    // Transform that takes neutral cylinder geometry to world space
    // e.g. 0,0,0 = origin of cylinder -> recenter position
    Matrix PoseTransform;

    // Transform that takes world space coordinates to neutral geometry
    // e.g. recenter position -> 0,0,0
    Matrix InversePoseTransform;

    std::vector<std::shared_ptr<MonitorRenderState>> Monitors;

    int ScreenFocusCenterX = 0;
    int ScreenFocusCenterY = 0;

    bool UpdatedCamerasThisFrame = false;
    uint64_t ExposureTimeUsec = 0;

    // Eye camera poses
    DirectX::SimpleMath::Quaternion CameraOrientation[2];
    Vector3 CameraPosition[2];

    // Monitor curvature parameters
    float CurveRadiusMeters = 0.f;
    Vector3 CurveCenter;

    // Which pixel the user is looking at
    bool GazeValid = false;
    int GazeX = -1;
    int GazeY = -1;

    PluginRenderInfo Plugins[XRM_PLUGIN_COUNT];


    void SetDpi(float dpi);
};


} // namespace xrm
