// Copyright 2019 Augmented Perception Corporation

#pragma once

#include "D3D11Tools.hpp"
#include "CameraCalibration.hpp"
#include "CameraClient.hpp"

#include "SimpleMath.h"

namespace xrm {

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::SimpleMath;


//------------------------------------------------------------------------------
// CameraImager

class CameraImager
{
public:
    // Attempt to acquire a new camera image
    bool AcquireImage(
        D3D11DeviceContext& device_context,
        core::CameraClient* cameras);


    // Do we have a camera image to render?
    bool HasCameraImage = false;

    // Texture that we can map to CPU memory (on VR device)
    ComPtr<ID3D11Texture2D> RenderTexture;
    D3D11_TEXTURE2D_DESC RenderTextureDesc;

    // Exposure time for the camera frame
    uint64_t ExposureTimeUsec = 0;

protected:
    // Texture that we can map to CPU memory (on dupe device)
    ComPtr<ID3D11Texture2D> StagingTexture;
    D3D11_TEXTURE2D_DESC StagingTextureDesc;

    // Last time when a camera image was captured
    uint64_t LastImageTicks = 0;

    int LastAcceptedBright = 0;
    unsigned FrameSkipped = 0;


    // Called when camera image updates
    bool UpdateTexture(
        D3D11DeviceContext& device_context,
        const uint8_t* image,
        unsigned width,
        unsigned height);

    void CreateTextures(
        D3D11DeviceContext& device_context,
        unsigned width,
        unsigned height);
};


} // namespace xrm
