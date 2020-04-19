// Copyright 2019 Augmented Perception Corporation

#pragma once

#include "OpenXrD3D11.hpp"
#include "MonitorRenderController.hpp"
#include "CameraRenderer.hpp"
#include "Plugins.hpp"

#include <wrl/client.h>

#include <DirectXColors.h>
#include <D3Dcompiler.h>
#include "SimpleMath.h"

namespace xrm {

using Microsoft::WRL::ComPtr;
using namespace DirectX;


//------------------------------------------------------------------------------
// Common


// RGB
constexpr Vector3 Red{ 1, 0, 0 };
constexpr Vector3 DarkRed{ 0.25f, 0, 0 };
constexpr Vector3 Green{ 0, 1, 0 };
constexpr Vector3 DarkGreen{ 0, 0.25f, 0 };
constexpr Vector3 Blue{ 0, 0, 1 };
constexpr Vector3 DarkBlue{ 0, 0, 0.25f };

struct ModelViewProjectionConstantBuffer
{
    XMFLOAT4X4 ModelViewProjection;
};


//------------------------------------------------------------------------------
// Cube

struct Cube
{
    XrSpace Space{ XR_NULL_HANDLE };
    XrVector3f Scale{ 0.1f, 0.1f, 0.1f };

    XrBool32 PoseValid{ false };
    XrPosef Pose = xr::math::Pose::Identity();


    ComPtr<ID3D11VertexShader> VertexShader;
    ComPtr<ID3D11PixelShader> PixelShader;
    ComPtr<ID3D11InputLayout> InputLayout;
    ComPtr<ID3D11Buffer> MvpCBuffer;
    ComPtr<ID3D11Buffer> VertexBuffer;
    ComPtr<ID3D11Buffer> IndexBuffer;


    void SetPosition(XrSpace space);
    void InitializeD3D(D3D11DeviceContext& device_context);
    void Render(
        ID3D11DeviceContext* context,
        const DirectX::SimpleMath::Matrix& view_projection_matrix);
};


//------------------------------------------------------------------------------
// MonitorQuadRenderer

struct MonitorQuadRenderParams
{
    ID3D11Texture2D* Texture = nullptr;

    DXGI_FORMAT TextureFormat;
    bool MultisamplingEnabled = false;

    DirectX::SimpleMath::Quaternion Orientation;
    DirectX::SimpleMath::Vector3 Position;
    DirectX::SimpleMath::Vector2 Size;
};

struct MonitorQuadRenderer
{
    ComPtr<ID3D11VertexShader> VertexShader;
    ComPtr<ID3D11PixelShader> PixelShader;
    ComPtr<ID3D11InputLayout> InputLayout;
    ComPtr<ID3D11Buffer> MvpCBuffer;
    ComPtr<ID3D11Buffer> VertexBuffer;
    ComPtr<ID3D11Buffer> IndexBuffer;
    ComPtr<ID3D11SamplerState> SamplerState;
    ComPtr<ID3D11BlendState> BlendState;


    void InitializeD3D(D3D11DeviceContext& device_context);

    void Render(
        D3D11DeviceContext& device_context,
        const DirectX::SimpleMath::Matrix& view_projection_matrix,
        const MonitorQuadRenderParams& params);
};


//------------------------------------------------------------------------------
// MonitorCylinderRenderer

struct MonitorCylinderParams
{
    ID3D11Texture2D* Texture = nullptr;

    DXGI_FORMAT TextureFormat;
    bool MultisamplingEnabled = false;
    bool EnableBlueLightFilter = false;

    float ColorScale = 1.f;

    // Geometry
    ID3D11Buffer* VertexBuffer = nullptr;
    ID3D11Buffer* IndexBuffer = nullptr;
    unsigned IndexCount = 0;
};

struct MonitorCylinderRenderer
{
    ComPtr<ID3D11VertexShader> VertexShader;
    ComPtr<ID3D11PixelShader> PixelShader;
    ComPtr<ID3D11InputLayout> InputLayout;
    ComPtr<ID3D11Buffer> MvpCBuffer;
    ComPtr<ID3D11Buffer> ColorCBuffer;
    ComPtr<ID3D11SamplerState> SamplerState;
    ComPtr<ID3D11BlendState> BlendState;


    void InitializeD3D(D3D11DeviceContext& device_context);

    void Render(
        D3D11DeviceContext& device_context,
        const DirectX::SimpleMath::Matrix& view_projection_matrix,
        const MonitorCylinderParams& params);
};


//------------------------------------------------------------------------------
// PoseHistory

struct OldPose
{
    uint64_t TimeUsec = 0;
    DirectX::SimpleMath::Quaternion Orientation;
    DirectX::SimpleMath::Vector3 Position;
};

struct PoseHistory
{
    static const int kPoseCount = 16;

    OldPose Poses[kPoseCount];
    int NextWriteIndex = 0;


    void WritePose(
        const DirectX::SimpleMath::Quaternion& q,
        const DirectX::SimpleMath::Vector3& p);
    bool FindPose(uint64_t t, OldPose& pose_out);
};


//------------------------------------------------------------------------------
// MonitorRenderView

class MonitorRenderView
{
    logger::Channel Logger;

public:
    MonitorRenderView(
        MonitorRenderController* render_controller,
        MonitorRenderModel* render_model,
        HeadsetCameraCalibration* camera_calibration,
        XrComputerProperties* computer,
        XrHeadsetProperties* headset,
        XrRenderProperties* rendering,
        PluginManager* plugins)
        : Logger("MonitorRenderView")
    {
        RenderController = render_controller;
        RenderModel = render_model;
        CameraCalibration = camera_calibration;
        Computer = computer;
        Headset = headset;
        Rendering = rendering;
        Plugins = plugins;
    }

    // Called just after Headset and Rendering initialization.
    void OnHeadsetRenderingInitialized(CameraImager* imager);

    // Called just before rendering all the views
    void FrameRenderStart();

    // Called for each view to render
    void RenderProjectiveView(XrD3D11ProjectionView* view);

    // Called for each monitor
    void RenderMonitorToQuadLayer(XrD3D11QuadLayerSwapchain* quad, MonitorRenderState* monitor);

private:
    MonitorRenderController* RenderController = nullptr;
    MonitorRenderModel* RenderModel = nullptr;
    HeadsetCameraCalibration* CameraCalibration = nullptr;
    XrComputerProperties* Computer = nullptr;
    XrHeadsetProperties* Headset = nullptr;
    XrRenderProperties* Rendering = nullptr;
    CameraImager* Imager = nullptr;
    PluginManager* Plugins = nullptr;

    // Cube demo
    Cube m_Cube;

    // Renderer for monitors using a quad
    MonitorQuadRenderer MonitorQuad;

    // Renderer for monitors using a cylinder
    MonitorCylinderRenderer MonitorCylinder;

    // Renderer for the cameras
    CameraRenderer CameraRender;

    // Jitter seed
    uint32_t NextJitterSeed = 0;

    // Camera pose history
    PoseHistory CameraPoseHistory[2];
};


} // namespace xrm
