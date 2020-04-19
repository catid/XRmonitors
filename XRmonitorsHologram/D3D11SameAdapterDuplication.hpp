// Copyright 2019 Augmented Perception Corporation

/*
    Same-Adapter D3D11 Desktop Duplication

    Background thread setup:
        Set up desktop duplication and run the background thread.

    Main thread setup:
        Nothing.


    Background thread online:

        KeyedMutex->AcquireSync(0, timeout)

        (1) Use Desktop Duplication API to get a frame
        (2) Re-create a desktop-sized shared texture if needed
        (3) Re-create a mouse-sized staging read-back texture if needed

        If there is a frame + mouse update:

            (4a) Copy the screen under the mouse to the read-back texture
            (4b) Copy the screen changed regions to the shared texture

        If there is only a mouse update:

            (4) Copy the shared texture under mouse to the read-back texture

        (5) Map read-back texture and render mouse on CPU
        (6) FrameReady = true

        KeyedMutex->Release(1)

        <Wait here for main thread to wake us back up>

    Main thread online:

        If FrameReady = true:

            (1) Open shared texture if needed and get the KeyedMutex

            KeyedMutex->AcquireSync(1, timeout)

            (2) Copy any changed regions from the shared texture to VR texture
            (3) Copy old mouse region from shared texture to VR texture
            (4) Map mouse write-texture and copy mouse raster to it on CPU
            (5) Copy mouse texture to VR texture

            KeyedMutex->Release(0)
*/

#pragma once

#include "D3D11DuplicationCommon.hpp"

namespace xrm {


//------------------------------------------------------------------------------
// D3D11SameAdapterDuplication

class D3D11SameAdapterDuplication : public ID3D11DesktopDuplication
{
public:
    // Initialize after setting above variables
    void Initialize(
        std::shared_ptr<MonitorEnumInfo> info,
        D3D11DeviceContext* vr_device_resources) override;

    // Stop background thread and release all objects
    void StartShutdown() override;
    void Shutdown() override;

    bool IsTerminated() const override
    {
        return Terminated;
    }

    // Blocks until frame texture is updated with latest frame.
    // Frame may be null if no frame was available.
    // Returns true if a frame was retrieved successfully
    bool AcquireVrRenderTexture() override;

    // Release VR render texture after use
    void ReleaseVrRenderTexture() override;

protected:
    // These are provided by Initialize()
    std::shared_ptr<MonitorEnumInfo> Info;
    D3D11DeviceContext* VrDeviceResources = nullptr;

    std::atomic<bool> FrameReady = ATOMIC_VAR_INIT(false);
    AutoEvent WaitEvent;

    std::atomic<bool> Terminated = ATOMIC_VAR_INIT(false);
    std::shared_ptr<std::thread> Thread;

    // Current frame desktop texture description
    D3D11_TEXTURE2D_DESC DesktopDesc;
    bool FirstDesktopFrame = true;
    RECT FullDesktopRect;

    // Desktop capture metadata
    std::vector<uint8_t> MetadataBuffer;

    // Description of what to copy from dupe texture to VR texture
    DesktopCopyDesc CopyDesc;

    // Pointer shape buffer
    DXGI_OUTDUPL_POINTER_SHAPE_INFO PointerShapeInfo;
    std::vector<uint8_t> PointerShapeBuffer;
    UINT PointerShapeBufferBytes = 0;

    // Cursor superimposed on the desktop for the current frame.
    // Empty if no cursor to render
    StoredCursor Cursor;

    // Last valid cursor position
    int LastValidCursorX = 0;
    int LastValidCursorY = 0;


    //--------------------------------------------------------------------------
    // Dupe Device Objects

    static const uint64_t kKeyedMutex_Dupe = 0;

    D3D11DeviceContext DupeDC;
    ComPtr<IDXGIOutput1> DxgiOutput1;
    ComPtr<IDXGIOutputDuplication> DXGIOutputDuplication;

    // Texture that receives the mouse cursor
    StagingTexture DupeStagingMouse;

    // VR texture on the D3D11Device side
    ComPtr<ID3D11Texture2D> DupeSharedTexture;
    D3D11_TEXTURE2D_DESC DupeSharedTextureDesc{};

    // Keyed mutex to synchronize cross-D3D11Device operations
    ComPtr<IDXGIKeyedMutex> DupeKeyedMutex;


    // Background thread loop
    void Loop();

    // Returns true if loop is healthy.
    // Returns false if loop cannot continue
    bool ReadFrame();

    // Create the shared texture
    bool CreateSharedTexture();

    // If a staging texture is needed, do the copy
    void CopyToSharedTexture(ComPtr<ID3D11Texture2D>& DesktopTexture);

    // Write mouse cursor directly to the VR staging texture
    bool UpdateMouseCursor(const DXGI_OUTDUPL_FRAME_INFO& frame_info);

    // Deliver the latest frame
    void DeliverFrame();


    //--------------------------------------------------------------------------
    // VR Device Objects

    static const uint64_t kKeyedMutex_VR = 1;

    // Texture that we can map to CPU memory (on VR device)
    StagingTexture VrStagingMouse;

    // Shared handle for VR texture to be written to from duplicated desktop
    AutoHandle SharedTexture;

    ComPtr<ID3D11Texture2D> VrSharedTexture;

    // Keyed mutex to synchronize cross-D3D11Device operations
    ComPtr<IDXGIKeyedMutex> VrKeyedMutex;

    // KeyedMutex acquired by VR code?
    bool VrMutexAcquired = false;

    // Was a VR cursor written to the VR texture
    bool VrCursorWritten = false;

    // Last cursor rectangle
    RECT VrLastCursorRect{};


    // Acquire shared texture access
    bool AcquireSharedTexture();

    // Create RenderTexture if needed
    bool CreateVrRenderTexture();

    // Reactions to a capture failure
    void OnCaptureFailure();

    // Copy between staging textures
    bool CopyToVrTextureFromShared();
};


} // namespace xrm
