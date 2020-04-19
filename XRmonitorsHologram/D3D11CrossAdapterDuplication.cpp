// Copyright 2019 Augmented Perception Corporation

#include "stdafx.h"

#include "D3D11CrossAdapterDuplication.hpp"

#include <immintrin.h>
#include <cstdint>
#include <comdef.h>

namespace xrm {

static logger::Channel Logger("XDuplication");


//------------------------------------------------------------------------------
// D3D11CrossAdapterDuplication

void D3D11CrossAdapterDuplication::Initialize(
    std::shared_ptr<MonitorEnumInfo> info,
    D3D11DeviceContext* vr_device_resources)
{
    Info = info;
    VrDeviceResources = vr_device_resources;

    std::ostringstream oss;
    oss << "Cross:[ #" << Info->MonitorIndex << " ] ";
    Logger.SetPrefix(oss.str());

    Logger.Debug("Initializing");

    DupeDC.Device = info->DC.Device;
    DupeDC.Context = info->DC.Context;
    DxgiOutput1 = info->DxgiOutput1;

    FirstDesktopFrame = true;
    WaitEvent = ::CreateEventW(nullptr, FALSE, FALSE, nullptr);
    Terminated = false;
    Thread = std::make_shared<std::thread>(&D3D11CrossAdapterDuplication::Loop, this);
}

void D3D11CrossAdapterDuplication::StartShutdown()
{
    Terminated = true;

    ::SetEvent(WaitEvent.Get());
}

void D3D11CrossAdapterDuplication::Shutdown()
{
    Logger.Debug("Shutdown");

    StartShutdown();

    JoinThread(Thread);

    WaitEvent.Clear();

    VrDeviceResources = nullptr;

    DXGIOutputDuplication.Reset();
    VrRenderTexture.Reset();

    DupeStaging.Reset();
    DupeStagingMouse.Reset();
    VrStaging.Reset();
    VrStagingPool.Shutdown();
}

bool D3D11CrossAdapterDuplication::AcquireVrRenderTexture()
{
    // Check if a frame is ready
    if (!FrameReady) {
        return false;
    }
    ScopedFunction sync_scope([&]() {
        // Regardless of how it goes, make sure we release the background thread
        FrameReady = false;
        ::SetEvent(WaitEvent.Get());
    });

    //Logger.Info("GOT: DirtyCount=", DirtyCount, " MoveCount=", MoveCount);

    /*
        From the main thread, when a frame ready flag is set:
        (0) Create textures if needed.
        (1) Map main staging texture for this monitor for write.
        (2) memcpy() the dirty and moved rects between staging textures.
        (3) Unmap main staging texture.
        (4) CopySubresourceRegion1() moved and dirty rects to render texture.
        (5) Signal background thread to wake up and continue.
    */

    if (!CreateVrRenderTexture()) {
        OnCaptureFailure();
        return false;
    }

#if 1
    if (CanUseVrStagingPool())
    {
        if (!CopyToVrStagingPool()) {
            OnCaptureFailure();
            return false;
        }
    }
    else
#endif
    {
        if (!CopyToVrStaging()) {
            OnCaptureFailure();
            return false;
        }
    }

    return true;
}

bool D3D11CrossAdapterDuplication::CopyToVrStaging()
{
    bool vr_staging = VrStaging.Prepare(
        *VrDeviceResources,
        StagingTexture::RW::WriteOnly,
        DesktopDesc.Width,
        DesktopDesc.Height,
        DesktopDesc.MipLevels,
        DesktopDesc.Format,
        DesktopDesc.SampleDesc.Count);

    if (!vr_staging) {
        OnCaptureFailure();
        return false;
    }

    if (!VrStaging.Map(*VrDeviceResources)) {
        return false;
    }

#ifdef DD_LOG_RECTS
    Logger.Info("MoveCount: ", CopyDesc.MoveCount, " DirtyCount: ", CopyDesc.DirtyCount);
#endif

    if (!CopyDesc.CursorToErase->Empty())
    {
        RECT rect{};
        rect.right = CopyDesc.CursorToErase->Width;
        rect.bottom = CopyDesc.CursorToErase->Height;

        const unsigned dest_offset = CopyDesc.CursorToErase->Y * VrStaging.GetMappedPitch() \
            + CopyDesc.CursorToErase->X * 4;

        CopyRectBGRA(
            rect,
            VrStaging.GetMappedData() + dest_offset,
            VrStaging.GetMappedPitch(),
            CopyDesc.CursorToErase->Rgba.data(),
            CopyDesc.CursorToErase->Width * 4);

#ifdef DD_LOG_RECTS
        Logger.Info("Mouse erase rect size: ",
            rect.right - rect.left,
            " x ",
            rect.bottom - rect.top,
            " @ (",
            CopyDesc.CursorToErase->X,
            ", ",
            CopyDesc.CursorToErase->Y,
            ")");
#endif
    }

    const unsigned move_count = CopyDesc.MoveCount;
    for (unsigned i = 0; i < move_count; ++i)
    {
        const RECT& rect = CopyDesc.MoveRects[i].DestinationRect;

        CopyRectBGRA(
            rect,
            VrStaging.GetMappedData(),
            VrStaging.GetMappedPitch(),
            DupeStaging.GetMappedData(),
            DupeStaging.GetMappedPitch());

#ifdef DD_LOG_RECTS
        Logger.Info("Move rect size: ",
            rect.right - rect.left,
            " x ",
            rect.bottom - rect.top,
            " @ (",
            rect.left,
            ", ",
            rect.top,
            ")");
#endif
    }

    const unsigned dirty_count = CopyDesc.DirtyCount;
    for (unsigned i = 0; i < dirty_count; ++i)
    {
        const RECT& rect = CopyDesc.DirtyRects[i];

        CopyRectBGRA(
            rect,
            VrStaging.GetMappedData(),
            VrStaging.GetMappedPitch(),
            DupeStaging.GetMappedData(),
            DupeStaging.GetMappedPitch());

#ifdef DD_LOG_RECTS
        Logger.Info("Dirty rect size: ",
            rect.right - rect.left,
            " x ",
            rect.bottom - rect.top,
            " @ (",
            rect.left,
            ", ",
            rect.top,
            ")");
#endif
    }

    if (!CopyDesc.CursorToWrite->Empty())
    {
        RECT rect{};
        rect.right = CopyDesc.CursorToWrite->Width;
        rect.bottom = CopyDesc.CursorToWrite->Height;

        const unsigned dest_offset = CopyDesc.CursorToWrite->Y * VrStaging.GetMappedPitch() \
            + CopyDesc.CursorToWrite->X * 4;

        CopyRectBGRA(
            rect,
            VrStaging.GetMappedData() + dest_offset,
            VrStaging.GetMappedPitch(),
            CopyDesc.CursorToWrite->Rgba.data(),
            CopyDesc.CursorToWrite->Width * 4);

#ifdef DD_LOG_RECTS
        Logger.Info("Mouse rect size: ",
            rect.right - rect.left,
            " x ",
            rect.bottom - rect.top,
            " @ (",
            CopyDesc.CursorToWrite->X,
            ", ",
            CopyDesc.CursorToWrite->Y,
            ")");
#endif
    }

    VrStaging.Unmap(*VrDeviceResources);

    // Copy to VR render texture:

    const auto& context = VrDeviceResources->Context;

    if (!CopyDesc.CursorToErase->Empty())
    {
        const unsigned x = CopyDesc.CursorToErase->X;
        const unsigned y = CopyDesc.CursorToErase->Y;

        D3D11_BOX Box;
        Box.left = x;
        Box.top = y;
        Box.front = 0;
        Box.right = x + CopyDesc.CursorToErase->Width;
        Box.bottom = y + CopyDesc.CursorToErase->Height;
        Box.back = 1;

        context->CopySubresourceRegion1(
            VrRenderTexture.Get(), // destination
            0,
            x, // destination x
            y, // destination y
            0,
            VrStaging.GetTexture(), // source
            0,
            &Box, // source
            D3D11_COPY_DISCARD);
    }

    for (unsigned i = 0; i < move_count; ++i)
    {
        const RECT& dest_rect = CopyDesc.MoveRects[i].DestinationRect;

        D3D11_BOX Box;
        Box.left = dest_rect.left;
        Box.top = dest_rect.top;
        Box.front = 0;
        Box.right = dest_rect.right;
        Box.bottom = dest_rect.bottom;
        Box.back = 1;

        context->CopySubresourceRegion1(
            VrRenderTexture.Get(), // destination
            0,
            dest_rect.left, // destination x
            dest_rect.top, // destination y
            0,
            VrStaging.GetTexture(), // source
            0,
            &Box, // source
            D3D11_COPY_DISCARD);
    }

    for (unsigned i = 0; i < dirty_count; ++i)
    {
        const RECT& dirty_rect = CopyDesc.DirtyRects[i];

        D3D11_BOX Box;
        Box.left = dirty_rect.left;
        Box.top = dirty_rect.top;
        Box.front = 0;
        Box.right = dirty_rect.right;
        Box.bottom = dirty_rect.bottom;
        Box.back = 1;

        context->CopySubresourceRegion1(
            VrRenderTexture.Get(), // destination
            0,
            dirty_rect.left, // destination x
            dirty_rect.top, // destination y
            0,
            VrStaging.GetTexture(), // source
            0,
            &Box, // source
            D3D11_COPY_DISCARD);
    }

    if (!CopyDesc.CursorToWrite->Empty())
    {
        const unsigned x = CopyDesc.CursorToWrite->X;
        const unsigned y = CopyDesc.CursorToWrite->Y;

        D3D11_BOX Box;
        Box.left = x;
        Box.top = y;
        Box.front = 0;
        Box.right = x + CopyDesc.CursorToWrite->Width;
        Box.bottom = y + CopyDesc.CursorToWrite->Height;
        Box.back = 1;

        context->CopySubresourceRegion1(
            VrRenderTexture.Get(), // destination
            0,
            x, // destination x
            y, // destination y
            0,
            VrStaging.GetTexture(), // source
            0,
            &Box, // source
            D3D11_COPY_DISCARD);
    }

    return true;
}

bool D3D11CrossAdapterDuplication::CanUseVrStagingPool()
{
    size_t total = CopyDesc.DirtyCount + CopyDesc.MoveCount + 2;
    if (CopyDesc.CursorToWrite->Empty()) {
        --total;
    }
    if (CopyDesc.CursorToErase->Empty()) {
        --total;
    }
    if (total > StagingTexturePool::kMaxAllocated) {
        return false;
    }

    const unsigned max_width = 1024;
    const unsigned max_height = 512;

    const unsigned move_count = CopyDesc.MoveCount;
    for (unsigned i = 0; i < move_count; ++i)
    {
        const RECT& rect = CopyDesc.MoveRects[i].DestinationRect;
        const int width = rect.right - rect.left;
        const int height = rect.bottom - rect.top;
        if (width > max_width || height > max_height) {
            return false;
        }
    }

    const unsigned dirty_count = CopyDesc.DirtyCount;
    for (unsigned i = 0; i < dirty_count; ++i)
    {
        const RECT& rect = CopyDesc.DirtyRects[i];
        const int width = rect.right - rect.left;
        const int height = rect.bottom - rect.top;
        if (width > max_width || height > max_height) {
            return false;
        }
    }

    if (!CopyDesc.CursorToWrite->Empty()) {
        if (CopyDesc.CursorToWrite->Width > max_width ||
            CopyDesc.CursorToWrite->Height > max_height)
        {
            return false;
        }
    }

    if (!CopyDesc.CursorToErase->Empty()) {
        if (CopyDesc.CursorToErase->Width > max_width ||
            CopyDesc.CursorToErase->Height > max_height)
        {
            return false;
        }
    }

    return true;
}

bool D3D11CrossAdapterDuplication::CopyToVrStagingPool()
{
    ++StagingPoolEpoch;

#ifdef DD_LOG_RECTS
    Logger.Info("Optimized - MoveCount: ", CopyDesc.MoveCount, " DirtyCount: ", CopyDesc.DirtyCount);
#endif

    if (!CopyDesc.CursorToErase->Empty())
    {
        if (!CopyToVrStagingPool_Mouse(CopyDesc.CursorToErase)) {
            return false;
        }
    }

    const unsigned move_count = CopyDesc.MoveCount;
    for (unsigned i = 0; i < move_count; ++i)
    {
        const RECT& rect = CopyDesc.MoveRects[i].DestinationRect;
        if (!CopyToVrStagingPool_SingleRect(rect)) {
            return false;
        }

#ifdef DD_LOG_RECTS
        Logger.Info("Move rect size: ",
            rect.right - rect.left,
            " x ",
            rect.bottom - rect.top,
            " @ (",
            rect.left,
            ", ",
            rect.top,
            ")");
#endif
    }

    const unsigned dirty_count = CopyDesc.DirtyCount;
    for (unsigned i = 0; i < dirty_count; ++i)
    {
        const RECT& rect = CopyDesc.DirtyRects[i];
        if (!CopyToVrStagingPool_SingleRect(rect)) {
            return false;
        }

#ifdef DD_LOG_RECTS
        Logger.Info("Dirty rect size: ",
            rect.right - rect.left,
            " x ",
            rect.bottom - rect.top,
            " @ (",
            rect.left,
            ", ",
            rect.top,
            ")");
#endif
    }

    if (!CopyDesc.CursorToWrite->Empty())
    {
        if (!CopyToVrStagingPool_Mouse(CopyDesc.CursorToWrite)) {
            return false;
        }
    }

    return true;
}

bool D3D11CrossAdapterDuplication::CopyToVrStagingPool_Mouse(StoredCursor* cursor)
{
    const unsigned x = cursor->X;
    const unsigned y = cursor->Y;
    const int width = cursor->Width;
    const int height = cursor->Height;

    EpochStagingTexture* texture = VrStagingPool.Acquire(
        DesktopDesc,
        *VrDeviceResources,
        StagingTexture::RW::WriteOnly,
        width,
        height,
        StagingPoolEpoch);
    if (!texture || !texture->Texture.Map(*VrDeviceResources)) {
        return false;
    }

    RECT tiny;
    tiny.left = 0;
    tiny.right = width;
    tiny.top = 0;
    tiny.bottom = height;

    CopyRectBGRA(
        tiny,
        texture->Texture.GetMappedData(),
        texture->Texture.GetMappedPitch(),
        cursor->Rgba.data(),
        width * 4);

    texture->Texture.Unmap(*VrDeviceResources);

    D3D11_BOX Box{};
    Box.right = width;
    Box.bottom = height;
    Box.back = 1;

    VrDeviceResources->Context->CopySubresourceRegion1(
        VrRenderTexture.Get(), // destination
        0,
        x, // destination x
        y, // destination y
        0,
        texture->Texture.GetTexture(), // source
        0,
        &Box, // source
        D3D11_COPY_DISCARD);

#ifdef DD_LOG_RECTS
    if (cursor == CopyDesc.CursorToErase) {
        Logger.Info("Erase rect size: ",
            width, " x ", height,
            " @ (", x, ", ", y, ")");
    }
    else {
        Logger.Info("Mouse rect size: ",
            width, " x ", height,
            " @ (", x, ", ", y, ")");
    }
#endif

    return true;
}

bool D3D11CrossAdapterDuplication::CopyToVrStagingPool_SingleRect(const RECT& rect)
{
    const int width = rect.right - rect.left;
    const int height = rect.bottom - rect.top;

    EpochStagingTexture* texture = VrStagingPool.Acquire(
        DesktopDesc,
        *VrDeviceResources,
        StagingTexture::RW::WriteOnly,
        width,
        height,
        StagingPoolEpoch);
    if (!texture) {
        return false;
    }

    if (!texture->Texture.Map(*VrDeviceResources)) {
        return false;
    }

    const unsigned src_offset = rect.top * DupeStaging.GetMappedPitch() + rect.left * 4;

    RECT tiny;
    tiny.left = 0;
    tiny.right = width;
    tiny.top = 0;
    tiny.bottom = height;

    CopyRectBGRA(
        tiny,
        texture->Texture.GetMappedData(),
        texture->Texture.GetMappedPitch(),
        DupeStaging.GetMappedData() + src_offset,
        DupeStaging.GetMappedPitch());

    texture->Texture.Unmap(*VrDeviceResources);

    D3D11_BOX Box{};
    Box.right = width;
    Box.bottom = height;
    Box.back = 1;

    VrDeviceResources->Context->CopySubresourceRegion1(
        VrRenderTexture.Get(), // destination
        0,
        rect.left, // destination x
        rect.top, // destination y
        0,
        texture->Texture.GetTexture(), // source
        0,
        &Box, // source
        D3D11_COPY_DISCARD);

    return true;
}

void D3D11CrossAdapterDuplication::Loop()
{
    SetCurrentThreadName("DesktopDupe");

    try {
        while (!Terminated)
        {
            if (!ReadFrame()) {
                OnCaptureFailure();
                Logger.Debug("ReadFrame failed: Stopping desktop duplication");
                break;
            }
        }
    }
    catch (_com_error& error)
    {
        Logger.Error("Duplication failed: ", error.ErrorMessage());
        Terminated = true;
    }

    Logger.Debug("Loop terminated");
}

bool D3D11CrossAdapterDuplication::ReadFrame()
{
    if (!DXGIOutputDuplication) {
        HRESULT hr = DxgiOutput1->DuplicateOutput(
            DupeDC.Device.Get(),
            &DXGIOutputDuplication);
        if (FAILED(hr)) {
            if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE) {
                Logger.Error("DuplicateOutput failed: DXGI_ERROR_NOT_CURRENTLY_AVAILABLE - Too many applications are using desktop duplication");
            }
            else {
                Logger.Warning("DuplicateOutput failed: ", HresultString(hr));
            }
            return false;
        }
    }

    ComPtr<IDXGIResource> resource;
    DXGI_OUTDUPL_FRAME_INFO frame_info{};
    HRESULT hr = DXGIOutputDuplication->AcquireNextFrame(
        100, // Interactive timeout in milliseconds
        &frame_info,
        &resource);

    if (FAILED(hr)) {
        if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
            // No new frame to report
            return true; // Still healthy
        }
        Logger.Error("AcquireNextFrame failed: ", HresultString(hr));
        return false; // Capture failed - Needs to be restarted
    }
    ScopedFunction resource_scope([&]() {
        resource.Reset();
        DXGIOutputDuplication->ReleaseFrame();
    });

    if (frame_info.AccumulatedFrames > 1) {
        //Logger.Warning("Skipped ", frame_info.AccumulatedFrames - 1, " frames");
    }

    const bool pointer_updated = frame_info.LastMouseUpdateTime.QuadPart != 0;
    if (pointer_updated) {
        const unsigned pointer_bytes = frame_info.PointerShapeBufferSize;
        if (pointer_bytes > 0) {
            if (PointerShapeBuffer.size() < pointer_bytes) {
                PointerShapeBuffer.resize(pointer_bytes);
            }

            PointerShapeBufferBytes = 0;
            hr = DXGIOutputDuplication->GetFramePointerShape(
                pointer_bytes,
                (void*)& PointerShapeBuffer[0],
                &PointerShapeBufferBytes,
                &PointerShapeInfo);
            if (FAILED(hr)) {
                Logger.Warning("GetFramePointerShape: ", HresultString(hr));
            }
        }

#if 0
        Logger.Info("Pointer updated: ", frame_info.LastMouseUpdateTime.QuadPart,
            " X: ", frame_info.PointerPosition.Position.x,
            " Y: ", frame_info.PointerPosition.Position.y,
            " Visible: ", frame_info.PointerPosition.Visible,
            " Bytes: ", PointerShapeBufferBytes);
#endif
    }

    const bool screen_updated = frame_info.LastPresentTime.QuadPart != 0 && resource != nullptr;
    if (!screen_updated)
    {
        // If there is a screen to modify:
        if (pointer_updated && DupeStaging.GetTexture() != nullptr)
        {
            CopyDesc.MoveCount = 0;
            CopyDesc.DirtyCount = 0;

            if (!DupeStaging.Map(DupeDC)) {
                return false;
            }

            WriteMouseCursor(frame_info);

            CopyDesc.CursorToWrite = &Cursor;
            CopyDesc.CursorToErase = &UnderCursor[UnderCurrentIndex ^ 1];

            // If there is anything to deliver:
            if (!CopyDesc.CursorToErase->Empty() ||
                !CopyDesc.CursorToWrite->Empty())
            {
                DeliverFrame();
            }

            DupeStaging.Unmap(DupeDC);

            UnderCurrentIndex ^= 1;
            CopyDesc.CursorToErase->Rgba.clear();
            CopyDesc.CursorToWrite->Rgba.clear();
        }

        return true; // Still healthy
    }

    ComPtr<ID3D11Texture2D> DesktopTexture;
    hr = resource.As(&DesktopTexture);
    if (FAILED(hr)) {
        Logger.Error("resource.As failed: ", HresultString(hr));
        return false;
    }

    DesktopTexture->GetDesc(&DesktopDesc);

    /*
        New frame has arrived!

        (0) Create DupeStagingTexture if needed
        (1) CopySubresourceRegion1() moved and dirty rects to staging texture.
        (2) Map staging texture.
        (3) Flag that a frame is ready.
        (4) Sleep until signalled.
        (5) Unmap staging texture.
    */

    CopyDesc.MoveRects = nullptr;
    CopyDesc.MoveCount = 0;
    CopyDesc.DirtyRects = nullptr;
    CopyDesc.DirtyCount = 0;
    const UINT metadata_size = frame_info.TotalMetadataBufferSize;

    if (FirstDesktopFrame) {
        // Copy the whole screen on the first capture
        FirstDesktopFrame = false;
        FullDesktopRect.left = 0;
        FullDesktopRect.top = 0;
        FullDesktopRect.right = DesktopDesc.Width;
        FullDesktopRect.bottom = DesktopDesc.Height;
        CopyDesc.DirtyRects = &FullDesktopRect;
        CopyDesc.DirtyCount = 1;
        CopyDesc.MoveCount = 0;
    } else {
        if (metadata_size > MetadataBuffer.size()) {
            MetadataBuffer.resize(metadata_size);
        }

        CopyDesc.MoveRects = (DXGI_OUTDUPL_MOVE_RECT*)&MetadataBuffer[0];
        UINT used_bytes = metadata_size;
        hr = DXGIOutputDuplication->GetFrameMoveRects(
            metadata_size,
            CopyDesc.MoveRects,
            &used_bytes);
        if (FAILED(hr)) {
            Logger.Error("GetFrameMoveRects failed: ", HresultString(hr));
            return false;
        }
        CopyDesc.MoveCount = used_bytes / sizeof(DXGI_OUTDUPL_MOVE_RECT);

        CopyDesc.DirtyRects = (RECT*)&MetadataBuffer[used_bytes];
        used_bytes = metadata_size - used_bytes;
        hr = DXGIOutputDuplication->GetFrameDirtyRects(
            used_bytes,
            CopyDesc.DirtyRects,
            &used_bytes);
        if (FAILED(hr)) {
            Logger.Error("GetFrameDirtyRects failed: ", HresultString(hr));
            return false;
        }
        CopyDesc.DirtyCount = used_bytes / sizeof(RECT);
    }

    // Bug: Expand dirty rects by 16 pixels because GDI cursor otherwise leaves trails
    const unsigned dirty_count = CopyDesc.DirtyCount;
    for (unsigned i = 0; i < dirty_count; ++i)
    {
        CopyDesc.DirtyRects[i].right += 16;
        CopyDesc.DirtyRects[i].bottom += 16;
        if (CopyDesc.DirtyRects[i].right > Info->DeviceSpaceWidth) {
            CopyDesc.DirtyRects[i].right = Info->DeviceSpaceWidth;
        }
        if (CopyDesc.DirtyRects[i].bottom > Info->DeviceSpaceHeight) {
            CopyDesc.DirtyRects[i].bottom = Info->DeviceSpaceHeight;
        }
    }

    if (!CopyToStagingTexture(DesktopTexture)) {
        return false;
    }

    // Note: MapDesktopSurface() seems to not be supported on modern NVidia
    // graphics cards so not bothering with it.

    // Write mouse cursor even if not updated, in case the screen was redrawn.
    // TBD: Only redraw cursor if needed
    WriteMouseCursor(frame_info);

    CopyDesc.CursorToWrite = &Cursor;
    CopyDesc.CursorToErase = &UnderCursor[UnderCurrentIndex ^ 1];

    DeliverFrame();

    UnderCurrentIndex ^= 1;
    CopyDesc.CursorToErase->Rgba.clear();
    CopyDesc.CursorToWrite->Rgba.clear();

    DupeStaging.Unmap(DupeDC);

    return true;
}

void D3D11CrossAdapterDuplication::DeliverFrame()
{
    FrameReady = true;

    int sleep_ms = 2;
    do
    {
        DWORD result = ::WaitForSingleObject(WaitEvent.Get(), INFINITE);
        if (result != WAIT_OBJECT_0) {
            Logger.Error("WaitForSingleObject failed: ", WindowsErrorString(::GetLastError()));
            ::Sleep(sleep_ms);
            sleep_ms *= 2;
            if (sleep_ms > 100) {
                sleep_ms = 100;
            }
            continue;
        }
    } while (FrameReady && !Terminated);
}

bool D3D11CrossAdapterDuplication::WriteMouseCursor(const DXGI_OUTDUPL_FRAME_INFO& frame_info)
{
    if (PointerShapeBufferBytes == 0) {
        return false; // No pointer
    }

    int width = PointerShapeInfo.Width;
    const unsigned pitch = PointerShapeInfo.Pitch;
    int height = PointerShapeInfo.Height;
    if (PointerShapeInfo.Type == DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MONOCHROME) {
        height /= 2;
    }

    int cursor_screen_x = frame_info.PointerPosition.Position.x;
    int cursor_screen_y = frame_info.PointerPosition.Position.y;

    // When the cursor is still, this is set to 0 and no cursor position is provided
    if (!frame_info.PointerPosition.Visible) {
        if (cursor_screen_x != 0 || cursor_screen_y != 0) {
            return false; // No pointer
        }

        CURSORINFO info{};
        info.cbSize = sizeof(info);
        if (!::GetCursorInfo(&info)) {
            Logger.Error("GetCursorInfo failed: ", WindowsErrorString(::GetLastError()));
            return false;
        }
        if ((info.flags & CURSOR_SHOWING) == 0) {
            return false; // Cursor hidden
        }

        const RECT screen_rect = Info->Coords;
        if (info.ptScreenPos.x < screen_rect.left ||
            info.ptScreenPos.x > screen_rect.right ||
            info.ptScreenPos.y < screen_rect.top ||
            info.ptScreenPos.y > screen_rect.bottom)
        {
            return false; // Pointer is off the screen
        }

        // FIXME: Find a better way to differentiate between dragging and "no update"
        const int approx_x = info.ptScreenPos.x - screen_rect.left;
        const int approx_y = info.ptScreenPos.y - screen_rect.top;
        if (abs(approx_x - LastValidCursorX) > 32 || abs(approx_y - LastValidCursorY) > 32)
        {
            return false; // Dragging: Cursor belongs to the window being dragged
        }

        // Use the previous cursor position.
        // The one from GetCursorInfo() is at a different offset.
        cursor_screen_x = LastValidCursorX;
        cursor_screen_y = LastValidCursorY;
    }

    // Keep track of prior cursor position for logic above
    LastValidCursorX = cursor_screen_x;
    LastValidCursorY = cursor_screen_y;

    // Read pixels under cursor from DupeStagingTextureData into UnderCursor:

    int offset_x = 0;
    int offset_y = 0;

    if (cursor_screen_x < 0) {
        offset_x = -cursor_screen_x;
        if (offset_x >= width) {
            return false; // Not visible
        }
        cursor_screen_x = 0;
        width -= offset_x;
    }
    else if (cursor_screen_x + width > DesktopDesc.Width) {
        if (cursor_screen_x >= DesktopDesc.Width) {
            return false; // Not visible
        }
        width = DesktopDesc.Width - cursor_screen_x;
    }

    if (cursor_screen_y < 0) {
        offset_y = -cursor_screen_y;
        if (offset_y >= height) {
            return false; // Not visible
        }
        cursor_screen_y = 0;
        height -= offset_y;
    }
    else if (cursor_screen_y + height > DesktopDesc.Height) {
        if (cursor_screen_y >= DesktopDesc.Height) {
            return false; // Not visible
        }
        height = DesktopDesc.Height - cursor_screen_y;
    }

    if (width <= 0 || height <= 0) {
        return false; // Not visible
    }

    //Logger.Info("Writing cursor at: ", cursor_screen_x, ", ", cursor_screen_y);

    // Update UnderCursor:

    StoredCursor* under_cursor = &UnderCursor[UnderCurrentIndex];
    under_cursor->PrepareWrite(width, height, cursor_screen_x, cursor_screen_y);

    const unsigned screen_pitch = DupeStaging.GetMappedPitch();
    const uint8_t* screen_src = DupeStaging.GetMappedData() + cursor_screen_x * 4 + cursor_screen_y * screen_pitch;

    // Copy the screen data under the cursor to UnderCursor
    uint8_t* copy_dst = under_cursor->Rgba.data();
    const uint8_t* copy_src = screen_src;
    const unsigned row_copy_bytes = width * 4;

    for (unsigned i = 0; i < height; ++i) {
        memcpy(copy_dst, copy_src, row_copy_bytes);
        copy_src += screen_pitch;
        copy_dst += row_copy_bytes;
    }

    // Update Cursor:

    Cursor.PrepareWrite(width, height, cursor_screen_x, cursor_screen_y);

    const unsigned cursor_pitch = width * 4;
    uint8_t* cursor_ptr = Cursor.Rgba.data();

    // Documented here:
    // https://docs.microsoft.com/en-us/windows/desktop/api/dxgi1_2/ne-dxgi1_2-dxgi_outdupl_pointer_shape_type
    if (PointerShapeInfo.Type == DXGI_OUTDUPL_POINTER_SHAPE_TYPE_COLOR)
    {
        const uint8_t* pointer_ptr = &PointerShapeBuffer[0];
        pointer_ptr += offset_x * 4 + offset_y * pitch;

        for (int i = 0; i < height; ++i)
        {
            const uint32_t* cursor_row = reinterpret_cast<const uint32_t*>(pointer_ptr);
            const uint32_t* input_row = reinterpret_cast<const uint32_t*>(screen_src);
            uint32_t* output_row = reinterpret_cast<uint32_t*>(cursor_ptr);

            for (int j = 0; j < width; ++j)
            {
                // FIXME: Support partial alpha?

                // If alpha is 50% or higher:
                if (cursor_row[j] >= 0x80000000) {
                    output_row[j] = cursor_row[j];
                }
                else {
                    output_row[j] = input_row[j];
                }
            } // Next column

            cursor_ptr += cursor_pitch;
            screen_src += screen_pitch;
            pointer_ptr += pitch;
        } // Next row

        return true;
    }

    if (PointerShapeInfo.Type == DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MASKED_COLOR)
    {
        const uint8_t* pointer_ptr = &PointerShapeBuffer[0];
        pointer_ptr += offset_x * 4 + offset_y * pitch;

        for (int i = 0; i < height; ++i)
        {
            const uint32_t* cursor_row = reinterpret_cast<const uint32_t*>(pointer_ptr);
            const uint32_t* input_row = reinterpret_cast<const uint32_t*>(screen_src);
            uint32_t* output_row = reinterpret_cast<uint32_t*>(cursor_ptr);

            for (int j = 0; j < width; ++j)
            {
                // FIXME: Support partial alpha?

                // If mask value is 0:
                if (input_row[j] < 0x80000000) {
                    output_row[j] = (input_row[j] & 0x00ffffff) | 0xff000000;
                }
                else {
                    output_row[j] ^= input_row[j] & 0x00ffffff;
                }
            } // Next column

            cursor_ptr += cursor_pitch;
            screen_src += screen_pitch;
            pointer_ptr += pitch;
        } // Next row

        return true;
    }

    // Assuming DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MONOCHROME:

    const uint8_t* shape_data = &PointerShapeBuffer[0];
    const unsigned xor_offset = PointerShapeInfo.Height / 2 * pitch;
    const unsigned skip_x = offset_x;
    const unsigned skip_y = offset_y;
    shape_data += skip_y * pitch;

    for (int i = 0; i < height; ++i)
    {
        const uint32_t* input_row = reinterpret_cast<const uint32_t*>(screen_src);
        uint32_t* output_row = reinterpret_cast<uint32_t*>(cursor_ptr);

        // Set mask
        BYTE Mask = 0x80;
        Mask = Mask >> (skip_x % 8);

        for (int j = 0; j < width; ++j)
        {
            // Get masks using appropriate offsets
            const unsigned mask_offset = (j + skip_x) / 8;
            const BYTE AndMask = shape_data[mask_offset] & Mask;
            const BYTE XorMask = shape_data[mask_offset + xor_offset] & Mask;

            uint32_t AndMask32 = (AndMask != 0) ? 0xFFFFFFFF : 0xFF000000;
            uint32_t XorMask32 = (XorMask != 0) ? 0x00FFFFFF : 0x00000000;

            const uint32_t original_pixel = input_row[j];
            const uint32_t result_pixel = (original_pixel & AndMask32) ^ XorMask32;
            output_row[j] = result_pixel;

            if (Mask == 0x01) {
                Mask = 0x80;
            }
            else {
                Mask >>= 1;
            }
        } // Next column

        cursor_ptr += cursor_pitch;
        screen_src += screen_pitch;
        shape_data += pitch;
    } // Next row

    return true;
}

bool D3D11CrossAdapterDuplication::CopyToStagingTexture(ComPtr<ID3D11Texture2D>& DesktopTexture)
{
    bool create_dupe = DupeStaging.Prepare(
        DupeDC,
        StagingTexture::RW::ReadOnly,
        DesktopDesc.Width,
        DesktopDesc.Height,
        DesktopDesc.MipLevels,
        DesktopDesc.Format,
        DesktopDesc.SampleDesc.Count);

    if (!create_dupe) {
        return false;
    }

    const unsigned move_count = CopyDesc.MoveCount;
    for (unsigned i = 0; i < move_count; ++i)
    {
        const RECT& move_rect = CopyDesc.MoveRects[i].DestinationRect;

#if 0
        if (this->Info->MonitorIndex != 0) {
            Logger.Debug("Move ", i, ": (",
                move_rect.DestinationRect.left, ", ",
                move_rect.DestinationRect.top, ") [w=",
                move_rect.DestinationRect.right - move_rect.DestinationRect.left, " h=",
                move_rect.DestinationRect.bottom - move_rect.DestinationRect.top, "]");
        }
#endif

        D3D11_BOX Box;
        Box.left = move_rect.left;
        Box.top = move_rect.top;
        Box.front = 0;
        Box.right = move_rect.right;
        Box.bottom = move_rect.bottom;
        Box.back = 1;

        DupeDC.Context->CopySubresourceRegion1(
            DupeStaging.GetTexture(), // destination
            0,
            move_rect.left, // destination x
            move_rect.top, // destination y
            0,
            DesktopTexture.Get(), // source
            0,
            &Box, // source
            D3D11_COPY_DISCARD);
    }

    const unsigned dirty_count = CopyDesc.DirtyCount;
    for (unsigned i = 0; i < dirty_count; ++i)
    {
        const RECT& dirty_rect = CopyDesc.DirtyRects[i];

#if 0
        if (this->Info->MonitorIndex == 1) {
            Logger.Debug("Dirty ", i, ": (",
                dirty_rect.left, ", ",
                dirty_rect.top, ") [w=",
                dirty_rect.right - dirty_rect.left, " h=",
                dirty_rect.bottom - dirty_rect.top, "]");
        }
#endif

        D3D11_BOX Box;
        Box.left = dirty_rect.left;
        Box.top = dirty_rect.top;
        Box.front = 0;
        Box.right = dirty_rect.right;
        Box.bottom = dirty_rect.bottom;
        Box.back = 1;

        DupeDC.Context->CopySubresourceRegion1(
            DupeStaging.GetTexture(), // destination
            0,
            dirty_rect.left, // destination x
            dirty_rect.top, // destination y
            0,
            DesktopTexture.Get(), // source
            0,
            &Box, // source
            D3D11_COPY_DISCARD);
    }

    return DupeStaging.Map(DupeDC);
}

bool D3D11CrossAdapterDuplication::CreateVrRenderTexture()
{
    // If texture must be recreated:
    if (!VrRenderTexture ||
        DesktopDesc.Width != VrRenderTextureDesc.Width ||
        DesktopDesc.Height != VrRenderTextureDesc.Height ||
        DesktopDesc.Format != VrRenderTextureDesc.Format)
    {
        Logger.Info("Recreating VrRenderTexture ( ",
            DesktopDesc.Width, "x", DesktopDesc.Height, " )");

        VrRenderTexture.Reset();

        VrRenderTextureDesc = DesktopDesc;
#ifdef ENABLE_DUPE_MIP_LEVELS
        VrRenderTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        VrRenderTextureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
#else
        VrRenderTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        VrRenderTextureDesc.MiscFlags = 0;
#endif
        VrRenderTextureDesc.CPUAccessFlags = 0;
        VrRenderTextureDesc.Usage = D3D11_USAGE_DEFAULT;
        VrRenderTextureDesc.MipLevels = kDupeMipLevels;

        HRESULT hr = VrDeviceResources->Device->CreateTexture2D(
            &VrRenderTextureDesc,
            nullptr,
            &VrRenderTexture);
        if (FAILED(hr)) {
            Logger.Error("CreateTexture2D failed: ", HresultString(hr));
            return false;
        }
    }

    return true;
}

void D3D11CrossAdapterDuplication::OnCaptureFailure()
{
    if (!CaptureFailure) {
        Terminated = true;
        CaptureFailure = true;
        Logger.Error("Unrecoverable desktop capture failure encountered");
    }
}


} // namespace xrm
