// Copyright 2019 Augmented Perception Corporation

#pragma once

#include "D3D11Tools.hpp"
#include "CameraCalibration.hpp"
#include "CameraImager.hpp"

namespace xrm {

using Microsoft::WRL::ComPtr;
using namespace DirectX;


//------------------------------------------------------------------------------
// CameraRenderer

enum class CameraColors
{
    Sepia,
    AllBusinessBlue,
    White,
    Grey,
};

struct CameraRenderParams
{
    int EyeIndex = 0;

    DXGI_FORMAT TextureFormat;
    bool MultisamplingEnabled = false;

    CameraColors Color;

    // Eye camera pose
    DirectX::SimpleMath::Quaternion CameraOrientation;
    DirectX::SimpleMath::Vector3 CameraPosition;
};

class CameraRenderer
{
public:
    void InitializeD3D(
        CameraImager* imager,
        HeadsetCameraCalibration* camera_calibration,
        D3D11DeviceContext& device_context);

    void Render(
        D3D11DeviceContext& device_context,
        const DirectX::SimpleMath::Matrix& view_projection_matrix,
        const CameraRenderParams& params);

protected:
    CameraImager* Imager = nullptr;
    HeadsetCameraCalibration* CameraCalibration = nullptr;

    ComPtr<ID3D11VertexShader> VertexShader;
    ComPtr<ID3D11PixelShader> PixelShader;
    ComPtr<ID3D11InputLayout> InputLayout;
    ComPtr<ID3D11Buffer> MvpCBuffer;
    ComPtr<ID3D11Buffer> ColorCBuffer;
    ComPtr<ID3D11SamplerState> SamplerState;

    static const unsigned kEyeViewCount = 2;

    ComPtr<ID3D11Buffer> VertexBuffer[kEyeViewCount];
    ComPtr<ID3D11Buffer> IndexBuffer;
    unsigned IndexCount = 0;

    std::vector<VertexPositionTexture> Vertices[kEyeViewCount];
    std::vector<uint16_t> Indices;


    void CreateTextures(
        D3D11DeviceContext& device_context,
        unsigned width,
        unsigned height);
    void GenerateMesh(float k1, float k2);
    void WarpVertex(float k1, float k2, float u, float v, float& s, float& t);
};


} // namespace xrm
