// Copyright 2019 Augmented Perception Corporation

/*
    Cross-Adapter D3D11 Desktop Duplication

    Each display has its own thread, duplication object, device object,
    and context object, blocked on acquiring a new frame.
    Each display has a staging texture created to be the same size as
    the desktop texture, but can be mapped for read access.

    When a new frame arrives on the background thread:
    (1) CopySubresourceRegion() moved and dirty rects to staging texture.
    (2) Map staging texture.
    (3) Flag that a frame is ready.
    (4) Sleep until signalled.
    (5) Unmap staging texture.

    The main thread also has a staging texture and a render texture for
    each monitor.

    From the main thread, when a frame ready flag is set:
    (1) Map main staging texture for this monitor for write.
    (2) memcpy() the dirty and moved rects between staging textures.
    (3) Unmap main staging texture.
    (4) CopySubresourceRegion() moved and dirty rects to render texture.
    (5) Signal background thread to wake up and continue.

    This approach uses ~128 MB texture memory on the GPU
        = 4 textures * 32 MB for each 4K monitor, and ~64 MB on the CPU.

    It does quite a lot of CPU copying, which is required in D3D11 for
    moving texture data between monitors.  Note that DirectX 12 enables
    memory to be copied directly between GPUs, at the expense of a
    tremendous amount of complexity, that is probably warranted longer-
    term for this product.

    It has a fairly good split of work between background and foreground
    threads, and importantly all context operations are performed on the
    respective thread for each context.
*/

/*
    Cursor rendering:

    From read thread:

    (1) Generate mouse-sized staging texture if needed
    (2) Copy desktop texture region for mouse to mouse-sized staging texture
    (3) Read under-cursor image into a vector [needed to erase old cursor]
    (4) Combine desktop with cursor info to generate cursor image vector

    - Ready to copy!

    From main render thread:

    (1) Copy under-cursor image to a staging texture
    (2) Copy under-cursor staging to screen
    (3) Copy all the screen updates, since they may overlap the cursor
    (4) Copy cursor image to a staging texture
    (5) Copy cursor image to the screen

    We can simplify this by reading the old location in addition to the new
    mouse location each frame.  By storing off the old frame contents we
    avoid this extra read.  The main render loop has the same operations
    either way.
*/

#pragma once

#include "D3D11DuplicationCommon.hpp"

namespace xrm {


//------------------------------------------------------------------------------
// D3D11CrossAdapterDuplication

class D3D11CrossAdapterDuplication : public ID3D11DesktopDuplication
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
    void ReleaseVrRenderTexture() override {}

protected:
    // These are provided by Initialize()
    std::shared_ptr<MonitorEnumInfo> Info;
    D3D11DeviceContext* VrDeviceResources = nullptr;

    D3D11DeviceContext DupeDC;
    ComPtr<IDXGIOutput1> DxgiOutput1;

    std::atomic<bool> FrameReady = ATOMIC_VAR_INIT(false);
    AutoEvent WaitEvent;

    std::atomic<bool> Terminated = ATOMIC_VAR_INIT(false);
    std::shared_ptr<std::thread> Thread;

    ComPtr<IDXGIOutputDuplication> DXGIOutputDuplication;

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

    // BGRA rectangle under the cursor from the current and previous frame.
    // Empty if there is nothing from the prior frame.
    // Otherwise before rendering the next desktop update these pixels should be restored
    // to the VR staging texture
    StoredCursor UnderCursor[2];

    // This is the index where the current desktop under cursor is written
    int UnderCurrentIndex = 0;

    // Cursor superimposed on the desktop for the current frame.
    // Empty if no cursor to render
    StoredCursor Cursor;

    // Last valid cursor position
    int LastValidCursorX = 0;
    int LastValidCursorY = 0;

    // Texture that we can map to CPU memory (on dupe device)
    StagingTexture DupeStaging;

    // Texture that receives the mouse cursor
    StagingTexture DupeStagingMouse;

    // Texture that we can map to CPU memory (on VR device)
    StagingTexture VrStaging;

    // Pool of smaller staging textures to greatly speed up Map() time
    StagingTexturePool VrStagingPool;

    // Pool epoch
    uint64_t StagingPoolEpoch = 0;


    // Background thread loop
    void Loop();

    // Returns true if loop is healthy.
    // Returns false if loop cannot continue
    bool ReadFrame();

    // If a staging texture is needed, do the copy
    bool CopyToStagingTexture(ComPtr<ID3D11Texture2D>& DesktopTexture);

    // Create RenderTexture if needed
    bool CreateVrRenderTexture();

    // Reactions to a capture failure
    void OnCaptureFailure();

    // Write mouse cursor directly to the VR staging texture
    bool WriteMouseCursor(const DXGI_OUTDUPL_FRAME_INFO& frame_info);

    // Deliver the latest frame
    void DeliverFrame();

    // Copy between staging textures
    bool CopyToVrStaging();

    // Check if CopyToVrStaging_Small() can be used
    bool CanUseVrStagingPool();

    // Copy between staging textures (small copy)
    bool CopyToVrStagingPool();
    bool CopyToVrStagingPool_SingleRect(const RECT& rect);
    bool CopyToVrStagingPool_Mouse(StoredCursor* cursor);
};


} // namespace xrm
