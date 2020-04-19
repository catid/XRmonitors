// Copyright 2019 Augmented Perception Corporation

#include "stdafx.h"

#include "CameraImager.hpp"

namespace xrm {

using namespace core;

static logger::Channel Logger("CameraImager");


//------------------------------------------------------------------------------
// CameraImager

bool CameraImager::AcquireImage(
    D3D11DeviceContext& device_context,
    core::CameraClient* cameras)
{
    CameraFrame frame;
    if (cameras->AcquireNextFrame(frame)) {
        ScopedFunction frame_scope([&]() {
            cameras->ReleaseFrame();
        });

        if (UpdateTexture(
            device_context,
            frame.CameraImage,
            frame.Width,
            frame.Height))
        {
            HasCameraImage = true;
            ExposureTimeUsec = frame.ExposureTimeUsec;
            LastImageTicks = ::GetTickCount64();
            return true;
        }
    }

    const uint64_t timeout_ticks = 500;
    if (::GetTickCount64() - LastImageTicks > timeout_ticks) {
        HasCameraImage = false;
    }

    return false;
}

void CameraImager::CreateTextures(
    D3D11DeviceContext& device_context,
    unsigned width,
    unsigned height)
{
    // If texture must be recreated:
    if (!RenderTexture ||
        width != RenderTextureDesc.Width ||
        height != RenderTextureDesc.Height)
    {
        Logger.Info("Recreating RenderTexture ( ", width, "x", height, " )");

        RenderTexture.Reset();

        memset(&RenderTextureDesc, 0, sizeof(RenderTextureDesc));
        RenderTextureDesc.Format = DXGI_FORMAT_R8_UNORM;
        RenderTextureDesc.MipLevels = 1;
        RenderTextureDesc.ArraySize = 1;
        RenderTextureDesc.SampleDesc.Count = 1;
        RenderTextureDesc.Width = width;
        RenderTextureDesc.Height = height;
        RenderTextureDesc.Usage = D3D11_USAGE_DEFAULT;
        RenderTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        XR_CHECK_HRCMD(device_context.Device->CreateTexture2D(
            &RenderTextureDesc,
            nullptr,
            &RenderTexture));
    }

    // If texture must be recreated:
    if (!StagingTexture ||
        RenderTextureDesc.Width != StagingTextureDesc.Width ||
        RenderTextureDesc.Height != StagingTextureDesc.Height ||
        RenderTextureDesc.Format != StagingTextureDesc.Format)
    {
        Logger.Info("Recreating StagingTexture ( ", width, "x", height, " )");

        StagingTexture.Reset();

        StagingTextureDesc = RenderTextureDesc;
        StagingTextureDesc.BindFlags = 0;
        StagingTextureDesc.Usage = D3D11_USAGE_STAGING;
        StagingTextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        XR_CHECK_HRCMD(device_context.Device->CreateTexture2D(
            &StagingTextureDesc,
            nullptr,
            &StagingTexture));
    }
}

bool CameraImager::UpdateTexture(
    D3D11DeviceContext& device_context,
    const uint8_t* image,
    unsigned width,
    unsigned height)
{
    CreateTextures(device_context, width, height);

    const UINT subresource_index = D3D11CalcSubresource(0, 0, 0);
    D3D11_MAPPED_SUBRESOURCE subresource{};

    XR_CHECK_HRCMD(device_context.Context->Map(
        StagingTexture.Get(),
        subresource_index,
        D3D11_MAP_WRITE,
        0, // flags
        &subresource));

    uint8_t* dest = reinterpret_cast<uint8_t*>(subresource.pData);
    const unsigned pitch = subresource.RowPitch;

    // HACK: Remove 32 byte tags from the image
    const unsigned offset = 23264 + 1312 - 32;
    unsigned next_tag_offset = offset - 1312 + 32;

    unsigned bright_sum_count = 0;
    int bright_sum = 0;

    for (unsigned i = 0; i < height; ++i) {
        if (next_tag_offset < width) {
            memcpy(dest, image, next_tag_offset);
            //memset(dest, 128, next_tag_offset);
            image += 32;
            memcpy(dest + next_tag_offset, image + next_tag_offset, width - next_tag_offset);
            //memset(dest + next_tag_offset, 128, width - next_tag_offset);
            next_tag_offset = offset - (width - next_tag_offset);
        }
        else {
            memcpy(dest, image, width);
            //memset(dest, 128, width);
            next_tag_offset -= width;

            for (unsigned j = 0; j < width; j += 32) {
                bright_sum += image[j];
            }
            ++bright_sum_count;
        }
        image += width;
        dest += pitch;
    }

    device_context.Context->Unmap(
        StagingTexture.Get(),
        subresource_index);

    int bright_avg = bright_sum / bright_sum_count * 20;
    if (bright_avg < LastAcceptedBright / 4 && FrameSkipped < 7) {
        return false;
    }
    LastAcceptedBright = bright_avg;
    FrameSkipped = 0;

#if 0
    // Reduce framerate
    static uint64_t last_t = 0;
    uint64_t now = GetTimeUsec();
    if (now - last_t < 1000000) {
        return false;
    }
    last_t = now;
#endif

    device_context.Context->CopyResource(RenderTexture.Get(), StagingTexture.Get());

    return true;
}


} // namespace xrm
