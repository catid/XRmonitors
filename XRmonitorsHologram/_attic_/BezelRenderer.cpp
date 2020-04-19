// Copyright 2019 Augmented Perception Corporation

#include "stdafx.h"

#include "BezelRenderer.hpp"

namespace xrm {

static logger::Channel Logger("Bezels");


//------------------------------------------------------------------------------
// BezelRenderer

void BezelRenderer::UpdateMonitor(
    D3D11DeviceContext& dc,
    MonitorEnumerator* enumerator,
    std::vector<std::shared_ptr<ID3D11DesktopDuplication>>& dupes,
    int monitor_index)
{
    const uint64_t t0 = GetTimeUsec();

    if (monitor_index >= dupes.size() ||
        monitor_index >= enumerator->Monitors.size() ||
        !dupes[monitor_index]->VrRenderTexture.Get())
    {
        Logger.Warning("No VR render texture while rendering bezels");
        return;
    }

    const RECT monitor_coords = enumerator->Monitors[monitor_index]->Coords;
    D3D11_TEXTURE2D_DESC& desc = dupes[monitor_index]->VrRenderTextureDesc;
    ID3D11Texture2D* vr_tex = dupes[monitor_index]->VrRenderTexture.Get();

    RECT dest_rect = monitor_coords;
    dest_rect.bottom = dest_rect.top;
    dest_rect.top -= kBezelPixels;

    if (TrimBezelRect(enumerator, monitor_index, dest_rect)) {
        bool success = RenderBezel(
            dc,
            dest_rect,
            desc,
            vr_tex,
            monitor_coords,
            BezelType::Top,
            BezelColor::Red);
        if (!success) {
            Logger.Error("RenderBezel failed");
            return;
        }
    }

    dest_rect = monitor_coords;
    dest_rect.top = dest_rect.bottom;
    dest_rect.bottom += kBezelPixels;

    if (TrimBezelRect(enumerator, monitor_index, dest_rect)) {
        bool success = RenderBezel(
            dc,
            dest_rect,
            desc,
            vr_tex,
            monitor_coords,
            BezelType::Bottom,
            BezelColor::Red);
        if (!success) {
            Logger.Error("RenderBezel failed");
            return;
        }
    }

    dest_rect = monitor_coords;
    dest_rect.right = dest_rect.left;
    dest_rect.left -= kBezelPixels;
    dest_rect.top -= kBezelPixels;
    dest_rect.bottom += kBezelPixels;

    if (TrimBezelRect(enumerator, monitor_index, dest_rect)) {
        bool success = RenderBezel(
            dc,
            dest_rect,
            desc,
            vr_tex,
            monitor_coords,
            BezelType::Left,
            BezelColor::Red);
        if (!success) {
            Logger.Error("RenderBezel failed");
            return;
        }
    }

    dest_rect = monitor_coords;
    dest_rect.left = dest_rect.right;
    dest_rect.right += kBezelPixels;
    dest_rect.top -= kBezelPixels;
    dest_rect.bottom += kBezelPixels;

    if (TrimBezelRect(enumerator, monitor_index, dest_rect)) {
        bool success = RenderBezel(
            dc,
            dest_rect,
            desc,
            vr_tex,
            monitor_coords,
            BezelType::Right,
            BezelColor::Red);
        if (!success) {
            Logger.Error("RenderBezel failed");
            return;
        }
    }

    const uint64_t t1 = GetTimeUsec();
    Logger.Info("Bezel render took: ", (t1 - t0), " usec");
}

bool BezelRenderer::RenderBezel(
    D3D11DeviceContext& dc,
    const RECT& bezel_rect,
    const D3D11_TEXTURE2D_DESC& vr_tex_desc,
    ID3D11Texture2D* vr_tex,
    const RECT& monitor_coords,
    BezelType type,
    BezelColor color)
{
    const int width = bezel_rect.right - bezel_rect.left;
    const int height = bezel_rect.bottom - bezel_rect.top;

    StagingTexture staging;
    bool result = staging.Prepare(
        dc,
        StagingTexture::RW::WriteOnly,
        width,
        height,
        vr_tex_desc.MipLevels,
        vr_tex_desc.Format,
        vr_tex_desc.SampleDesc.Count);
    if (!result)
    {
        Logger.Error("Failed to prepare staging texture for bezels");
        return false;
    }
    result = staging.Map(dc);
    if (!result)
    {
        Logger.Error("Failed to map staging texture for bezels");
        return false;
    }

    uint8_t* data = staging.GetMappedData();
    unsigned pitch = staging.GetMappedPitch();

    uint32_t argb = 0;
    switch (color)
    {
    case BezelColor::Clear:
    default:
        break;
    case BezelColor::Black:
        argb = 0xff000000;
        break;
    case BezelColor::Red:
        argb = 0xffff0000;
        break;
    }

    if (type == BezelType::Top)
    {
        memset(data, 0, width * 4);

        for (int i = 1; i < height; ++i) {
            uint32_t* argb_ptr = reinterpret_cast<uint32_t*>(data + pitch * i);
            for (int j = 0; j < width; ++j) {
                argb_ptr[j] = argb;
            }
        }
    }
    else if (type == BezelType::Bottom)
    {
        for (int i = 0; i + 1 < height; ++i) {
            uint32_t* argb_ptr = reinterpret_cast<uint32_t*>(data + pitch * i);
            for (int j = 0; j < width; ++j) {
                argb_ptr[j] = argb;
            }
        }

        memset(data + pitch * (height - 1), 0, width * 4);
    }
    else if (type == BezelType::Left)
    {
        memset(data, 0, width * 4);

        for (int i = 1; i + 1 < height; ++i) {
            uint32_t* argb_ptr = reinterpret_cast<uint32_t*>(data + pitch * i);
            argb_ptr[0] = 0;
            for (int j = 1; j < width; ++j) {
                argb_ptr[j] = argb;
            }
        }

        memset(data + pitch * (height - 1), 0, width * 4);
    }
    else if (type == BezelType::Right)
    {
        memset(data, 0, width * 4);

        for (int i = 1; i + 1 < height; ++i) {
            uint32_t* argb_ptr = reinterpret_cast<uint32_t*>(data + pitch * i);
            for (int j = 0; j + 1 < width; ++j) {
                argb_ptr[j] = argb;
            }
            argb_ptr[width - 1] = 0;
        }

        memset(data + pitch * (height - 1), 0, width * 4);
    }

    staging.Unmap(dc);

    D3D11_BOX Box;
    Box.left = 0;
    Box.top = 0;
    Box.front = 0;
    Box.right = width;
    Box.bottom = height;
    Box.back = 1;

    dc.Context->CopySubresourceRegion1(
        vr_tex, // destination
        0,
        kBezelPixels + bezel_rect.left - monitor_coords.left, // destination x
        kBezelPixels + bezel_rect.top - monitor_coords.top, // destination y
        0,
        staging.GetTexture(), // source
        0,
        &Box, // source
        D3D11_COPY_DISCARD);

    return true;
}

bool BezelRenderer::TrimBezelRect(
    MonitorEnumerator* enumerator,
    unsigned monitor_index,
    RECT& r1)
{
    for (auto& monitor : enumerator->Monitors)
    {
        RECT r2 = monitor->Coords;
        if (monitor->MonitorIndex == monitor_index) {
            continue;
        }

        // If they do not intersect:
        if (r2.left > r1.right ||
            r2.right < r1.left ||
            r2.top > r1.bottom ||
            r2.bottom < r1.top)
        {
            continue;
        }

        // Option A: Trim horizontally
        RECT optA = r1;
        if (optA.left > r2.left) {
            // We should trim the left side
            optA.left = r2.right;
        }
        else {
            // We should trim the right side down
            optA.right = r2.left;
        }

        // Option B: Trim vertically
        RECT optB = r1;
        if (optB.top > r2.top) {
            // We should trim the top side
            optB.top = r2.bottom;
        }
        else {
            // We should trim the bottom side down
            optB.bottom = r2.top;
        }

        // Check if the resulting rectangle is okay
        const bool opt_a_empty = optA.left >= optA.right;
        const bool opt_b_empty = optB.top >= optB.bottom;
        const bool opt_a_okay = !opt_a_empty && (r2.top < optA.bottom || r2.bottom < optA.top);
        const bool opt_b_okay = !opt_b_empty && (r2.left < optB.right || r2.right < optB.left);

        if (opt_a_okay && opt_b_okay) {
            // Pick the one with the larger area
            const unsigned area_a = (optA.right - optA.left) * (optA.bottom - optA.top);
            const unsigned area_b = (optB.right - optB.left) * (optB.bottom - optB.top);
            if (area_a > area_b) {
                r1 = optA;
            }
            else {
                r1 = optB;
            }
        }
        else if (opt_a_okay) {
            r1 = optA;
        }
        else if (opt_b_okay) {
            r1 = optB;
        }
        else {
            // No way to fix it
            return false;
        }
    }

    return true;
}


} // namespace xrm
