// Copyright 2019 Augmented Perception Corporation

#include "stdafx.h"

#include "D3D11SameAdapterDuplication.hpp"

#include <immintrin.h>
#include <cstdint>
#include <comdef.h>

namespace xrm {

static logger::Channel Logger("SDuplication");


//------------------------------------------------------------------------------
// D3D11SameAdapterDuplication

void D3D11SameAdapterDuplication::Initialize(
    std::shared_ptr<MonitorEnumInfo> info,
    D3D11DeviceContext* vr_device_resources)
{
    Info = info;
    VrDeviceResources = vr_device_resources;

    std::ostringstream oss;
    oss << "Same:[ #" << Info->MonitorIndex << " ] ";
    Logger.SetPrefix(oss.str());

    Logger.Debug("Initializing");

    DupeDC.Device = info->DC.Device;
    DupeDC.Context = info->DC.Context;
    DxgiOutput1 = info->DxgiOutput1;

    FirstDesktopFrame = true;
    WaitEvent = ::CreateEventW(nullptr, FALSE, FALSE, nullptr);
    Terminated = false;
    Thread = std::make_shared<std::thread>(&D3D11SameAdapterDuplication::Loop, this);
}

void D3D11SameAdapterDuplication::StartShutdown()
{
    Terminated = true;

    ::SetEvent(WaitEvent.Get());
}

void D3D11SameAdapterDuplication::Shutdown()
{
    Logger.Debug("Shutdown");

    StartShutdown();

    // Release just in case it is still dangling
    ReleaseVrRenderTexture();

    JoinThread(Thread);

    WaitEvent.Clear();

    VrDeviceResources = nullptr;

    // Dupe:
    DXGIOutputDuplication.Reset();
    DxgiOutput1.Reset();
    DupeStagingMouse.Reset();
    DupeSharedTexture.Reset();
    DupeKeyedMutex.Reset();

    // VR:
    VrStagingMouse.Reset();
    VrRenderTexture.Reset();
    SharedTexture.Clear();
    VrKeyedMutex.Reset();
}

bool D3D11SameAdapterDuplication::AcquireVrRenderTexture()
{
    if (VrMutexAcquired) {
        Logger.Error("KeyedMutex never released");
        ReleaseVrRenderTexture();
    }

    // Check if a frame is ready
    if (!FrameReady) {
        return false;
    }

    //Logger.Info("GOT: DirtyCount=", DirtyCount, " MoveCount=", MoveCount);

    if (!AcquireSharedTexture()) {
        OnCaptureFailure();
        return false;
    }

    if (!CreateVrRenderTexture()) {
        OnCaptureFailure();
        return false;
    }

    if (!CopyToVrTextureFromShared()) {
        OnCaptureFailure();
        return false;
    }

    return true;
}

bool D3D11SameAdapterDuplication::AcquireSharedTexture()
{
    if (!VrKeyedMutex)
    {
        HRESULT hr = VrDeviceResources->Device->OpenSharedResource1(
            SharedTexture.Get(),
            __uuidof(ID3D11Texture2D),
            reinterpret_cast<void**>(VrSharedTexture.ReleaseAndGetAddressOf()));

        if (FAILED(hr)) {
            Logger.Error("OpenSharedResource ID3D11Texture2D failed: ", HresultString(hr));
            return false;
        }

        hr = VrSharedTexture->QueryInterface(
            __uuidof(IDXGIKeyedMutex),
            reinterpret_cast<void**>(VrKeyedMutex.ReleaseAndGetAddressOf()));

        if (FAILED(hr)) {
            Logger.Error("VrKeyedMutex QueryInterface IDXGIKeyedMutex failed: ", HresultString(hr));
            return false;
        }
    }

    HRESULT hr = VrKeyedMutex->AcquireSync(kKeyedMutex_VR, 0);

    // If the background thread is still working:
    if (hr == (HRESULT)WAIT_TIMEOUT) {
        return false;
    }

    if (FAILED(hr))
    {
        Logger.Error("FrameReady but AcquireSync failed: ", HresultString(hr));
        OnCaptureFailure();
        return false;
    }

    VrMutexAcquired = true;

    return true;
}

void D3D11SameAdapterDuplication::ReleaseVrRenderTexture()
{
    if (!VrMutexAcquired) {
        return;
    }
    VrMutexAcquired = false;

    // Release shared texture access
    HRESULT hr = VrKeyedMutex->ReleaseSync(kKeyedMutex_Dupe);
    if (FAILED(hr)) {
        Logger.Error("ReleaseSync failed: ", HresultString(hr));
    }

    // Regardless of how it goes, make sure we release the background thread
    FrameReady = false;
    ::SetEvent(WaitEvent.Get());
}

bool D3D11SameAdapterDuplication::CreateVrRenderTexture()
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

void D3D11SameAdapterDuplication::OnCaptureFailure()
{
    if (!CaptureFailure) {
        Terminated = true;
        CaptureFailure = true;
        Logger.Error("Unrecoverable desktop capture failure encountered");
    }
}

bool D3D11SameAdapterDuplication::CopyToVrTextureFromShared()
{
    const auto& context = VrDeviceResources->Context;

    // If VR cursor was written previously:
    if (VrCursorWritten)
    {
        VrCursorWritten = false;

#ifdef DD_LOG_RECTS
        Logger.Info("Erase rect size: ",
            VrLastCursorRect.right - VrLastCursorRect.left,
            " x ",
            VrLastCursorRect.bottom - VrLastCursorRect.top,
            " @ (",
            VrLastCursorRect.left,
            ", ",
            VrLastCursorRect.top,
            ")");
#endif

        D3D11_BOX Box;
        Box.left = VrLastCursorRect.left;
        Box.top = VrLastCursorRect.top;
        Box.front = 0;
        Box.right = VrLastCursorRect.right;
        Box.bottom = VrLastCursorRect.bottom;
        Box.back = 1;

        // Clear the previously written cursor
        context->CopySubresourceRegion1(
            VrRenderTexture.Get(), // destination
            0,
            VrLastCursorRect.left, // destination x
            VrLastCursorRect.top, // destination y
            0,
            VrSharedTexture.Get(), // source
            0,
            &Box, // source
            D3D11_COPY_DISCARD);
    }

#ifdef DD_LOG_RECTS
    Logger.Info("MoveCount: ", CopyDesc.MoveCount, " DirtyCount: ", CopyDesc.DirtyCount);
#endif

    const unsigned move_count = CopyDesc.MoveCount;
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
            VrSharedTexture.Get(), // source
            0,
            &Box, // source
            D3D11_COPY_DISCARD);
    }

    const unsigned dirty_count = CopyDesc.DirtyCount;
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
            VrSharedTexture.Get(), // source
            0,
            &Box, // source
            D3D11_COPY_DISCARD);
    }

    // If cursor is available:
    if (!Cursor.Empty())
    {
#ifdef DD_LOG_RECTS
        Logger.Info("Mouse rect size: ",
            Cursor.Width,
            " x ",
            Cursor.Height,
            " @ (",
            Cursor.X,
            ", ",
            Cursor.Y,
            ")");
#endif

        // Prepare a write staging texture
        bool vr_staging = VrStagingMouse.Prepare(
            *VrDeviceResources,
            StagingTexture::RW::WriteOnly,
            Cursor.Width,
            Cursor.Height,
            DesktopDesc.MipLevels,
            DesktopDesc.Format,
            DesktopDesc.SampleDesc.Count);

        if (!vr_staging) {
            OnCaptureFailure();
            return false;
        }

        if (!VrStagingMouse.Map(*VrDeviceResources)) {
            OnCaptureFailure();
            return false;
        }

        const uint8_t* src = Cursor.Rgba.data();

        RECT rect{};
        rect.bottom = Cursor.Height;
        rect.right = Cursor.Width;
        CopyRectBGRA(
            rect,
            VrStagingMouse.GetMappedData(),
            VrStagingMouse.GetMappedPitch(),
            src,
            Cursor.Width * 4);

        VrStagingMouse.Unmap(*VrDeviceResources);

        D3D11_BOX Box;
        Box.left = 0;
        Box.top = 0;
        Box.front = 0;
        Box.right = Cursor.Width;
        Box.bottom = Cursor.Height;
        Box.back = 1;

        context->CopySubresourceRegion1(
            VrRenderTexture.Get(), // destination
            0,
            Cursor.X, // destination x
            Cursor.Y, // destination y
            0,
            VrStagingMouse.GetTexture(), // source
            0,
            &Box, // source
            D3D11_COPY_DISCARD);

        VrCursorWritten = true;
        VrLastCursorRect.left = Cursor.X;
        VrLastCursorRect.top = Cursor.Y;
        VrLastCursorRect.right = Cursor.X + Cursor.Width;
        VrLastCursorRect.bottom = Cursor.Y + Cursor.Height;
    }

    return true;
}


//--------------------------------------------------------------------------
// Desktop Duplication Thread

void D3D11SameAdapterDuplication::Loop()
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

bool D3D11SameAdapterDuplication::ReadFrame()
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

    // If the screen did not update:
    const bool screen_updated = frame_info.LastPresentTime.QuadPart != 0 && resource != nullptr;
    if (!screen_updated)
    {
        // If the pointer updated:
        if (pointer_updated && DupeKeyedMutex != nullptr)
        {
            CopyDesc.MoveCount = 0;
            CopyDesc.DirtyCount = 0;

            // Acquire shared texture access
            hr = DupeKeyedMutex->AcquireSync(kKeyedMutex_Dupe, 0);
            if (FAILED(hr)) {
                Logger.Error("DupeKeyedMutex->AcquireSync failed: ", HresultString(hr));
                return false;
            }

            UpdateMouseCursor(frame_info);

            // Release shared texture access
            hr = DupeKeyedMutex->ReleaseSync(kKeyedMutex_VR);
            if (FAILED(hr)) {
                Logger.Error("DupeKeyedMutex->ReleaseSync failed: ", HresultString(hr));
                return false;
            }

            DeliverFrame();
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

    if (!CreateSharedTexture()) {
        return false;
    }

    CopyDesc.MoveRects = nullptr;
    CopyDesc.MoveCount = 0;
    CopyDesc.DirtyRects = nullptr;
    CopyDesc.DirtyCount = 0;
    const UINT metadata_size = frame_info.TotalMetadataBufferSize;

    if (FirstDesktopFrame) {
        Logger.Info("Copying whole screen on the first capture");
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

    // Acquire shared texture access
    hr = DupeKeyedMutex->AcquireSync(kKeyedMutex_Dupe, 0);
    if (FAILED(hr)) {
        Logger.Error("DupeKeyedMutex->AcquireSync failed: ", HresultString(hr));
        return false;
    }

    CopyToSharedTexture(DesktopTexture);

    // Write mouse cursor even if not updated, in case the screen was redrawn.
    // TBD: Only redraw cursor if needed
    UpdateMouseCursor(frame_info);

    // Release shared texture access
    hr = DupeKeyedMutex->ReleaseSync(kKeyedMutex_VR);
    if (FAILED(hr)) {
        Logger.Error("DupeKeyedMutex->ReleaseSync failed: ", HresultString(hr));
        return false;
    }

    DeliverFrame();

    Cursor.Rgba.clear();

    return true;
}

bool D3D11SameAdapterDuplication::CreateSharedTexture()
{
    // If texture must be recreated:
    if (!DupeSharedTexture ||
        DesktopDesc.Width != DupeSharedTextureDesc.Width ||
        DesktopDesc.Height != DupeSharedTextureDesc.Height ||
        DesktopDesc.Format != DupeSharedTextureDesc.Format)
    {
        Logger.Info("Recreating DupeSharedTexture ( ",
            DesktopDesc.Width, "x", DesktopDesc.Height, " )");

        // Release VR side resources
        VrKeyedMutex.Reset();
        VrSharedTexture.Reset();
        SharedTexture.Clear();

        // Release Dupe side resources
        DupeKeyedMutex.Reset();
        DupeSharedTexture.Reset();

        FirstDesktopFrame = true;

        DupeSharedTextureDesc = DesktopDesc;
        DupeSharedTextureDesc.BindFlags = 0;
        DupeSharedTextureDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX | D3D11_RESOURCE_MISC_SHARED_NTHANDLE;
        DupeSharedTextureDesc.CPUAccessFlags = 0;
        DupeSharedTextureDesc.Usage = D3D11_USAGE_DEFAULT; // Not a staging texture
        DupeSharedTextureDesc.MipLevels = kDupeMipLevels;

        HRESULT hr = DupeDC.Device->CreateTexture2D(
            &DupeSharedTextureDesc,
            nullptr,
            &DupeSharedTexture);
        if (FAILED(hr)) {
            Logger.Error("CreateTexture2D failed: ", HresultString(hr));
            return false;
        }

        hr = DupeSharedTexture->QueryInterface(
            __uuidof(IDXGIKeyedMutex),
            reinterpret_cast<void**>(DupeKeyedMutex.ReleaseAndGetAddressOf()));
        if (FAILED(hr)) {
            Logger.Error("DupeSharedTexture QueryInterface IDXGIKeyedMutex failed: ", HresultString(hr));
            return false;
        }

        ComPtr<IDXGIResource1> resource;
        hr = DupeSharedTexture.As(&resource);
        if (FAILED(hr)) {
            Logger.Error("DupeSharedTexture.As failed: ", HresultString(hr));
            return false;
        }

        SharedTexture.Clear();
        HANDLE shared_handle = 0;
        hr = resource->CreateSharedHandle(nullptr, DXGI_SHARED_RESOURCE_READ, nullptr, &shared_handle);
        SharedTexture = shared_handle;
    }

    return true;
}

void D3D11SameAdapterDuplication::DeliverFrame()
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

bool D3D11SameAdapterDuplication::UpdateMouseCursor(const DXGI_OUTDUPL_FRAME_INFO& frame_info)
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

    // Use the width/height of the cursor image rather than truncated cursor
    // to avoid recreating the cursor texture a lot
    bool create_dupe = DupeStagingMouse.Prepare(
        DupeDC,
        StagingTexture::RW::ReadOnly,
        width,
        height,
        DesktopDesc.MipLevels,
        DesktopDesc.Format,
        DesktopDesc.SampleDesc.Count);

    if (!create_dupe) {
        return false;
    }

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

    D3D11_BOX Box;
    Box.left = cursor_screen_x;
    Box.top = cursor_screen_y;
    Box.front = 0;
    Box.right = cursor_screen_x + width;
    Box.bottom = cursor_screen_y + height;
    Box.back = 1;

    DupeDC.Context->CopySubresourceRegion1(
        DupeStagingMouse.GetTexture(), // destination
        0,
        0, // destination x
        0, // destination y
        0,
        DupeSharedTexture.Get(), // source
        0,
        &Box, // source
        D3D11_COPY_DISCARD);

    if (!DupeStagingMouse.Map(DupeDC)) {
        return false;
    }
    ScopedFunction scoped_map([&]() {
        DupeStagingMouse.Unmap(DupeDC);
    });

    const uint8_t* screen_src = DupeStagingMouse.GetMappedData();
    const unsigned screen_pitch = DupeStagingMouse.GetMappedPitch();

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

void D3D11SameAdapterDuplication::CopyToSharedTexture(ComPtr<ID3D11Texture2D>& DesktopTexture)
{
    const unsigned move_count = CopyDesc.MoveCount;
    for (unsigned i = 0; i < move_count; ++i)
    {
        const RECT& move_rect = CopyDesc.MoveRects[i].DestinationRect;

#if 0
        Logger.Debug("Move ", i, ": (",
            move_rect.left, ", ",
            move_rect.top, ") [w=",
            move_rect.right - move_rect.left, " h=",
            move_rect.bottom - move_rect.top, "]");
#endif

        D3D11_BOX Box;
        Box.left = move_rect.left;
        Box.top = move_rect.top;
        Box.front = 0;
        Box.right = move_rect.right;
        Box.bottom = move_rect.bottom;
        Box.back = 1;

        DupeDC.Context->CopySubresourceRegion1(
            DupeSharedTexture.Get(), // destination
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
        Logger.Debug("Dirty ", i, ": (",
            dirty_rect.left, ", ",
            dirty_rect.top, ") [w=",
            dirty_rect.right - dirty_rect.left, " h=",
            dirty_rect.bottom - dirty_rect.top, "]");
#endif

        D3D11_BOX Box;
        Box.left = dirty_rect.left;
        Box.top = dirty_rect.top;
        Box.front = 0;
        Box.right = dirty_rect.right;
        Box.bottom = dirty_rect.bottom;
        Box.back = 1;

        DupeDC.Context->CopySubresourceRegion1(
            DupeSharedTexture.Get(), // destination
            0,
            dirty_rect.left, // destination x
            dirty_rect.top, // destination y
            0,
            DesktopTexture.Get(), // source
            0,
            &Box, // source
            D3D11_COPY_DISCARD);
    }
}


} // namespace xrm
