// Copyright 2019 Augmented Perception Corporation

#pragma once

#include "MonitorTools.hpp"
#include "MonitorEnumerator.hpp"
#include "D3D11Tools.hpp"

namespace xrm {


//------------------------------------------------------------------------------
// Constants

// Should we log rects?
//#define DD_LOG_RECTS

// Disabled: When monitors are about the same size as the render target,
// e.g. Full HD, mips add a ton of blur
#define ENABLE_DUPE_MIP_LEVELS

#if defined(ENABLE_DUPE_MIP_LEVELS)
static const unsigned kDupeMipLevels = 2;
#else
static const unsigned kDupeMipLevels = 1;
#endif


//------------------------------------------------------------------------------
// Tools

void MemCopy2D(
    const unsigned rows,
    const unsigned cols_bytes,
    uint8_t* __restrict dest,
    const uint8_t* __restrict src,
    const unsigned dest_pitch,
    const unsigned src_pitch);

void AlignedCopyRectBGRA(
    const RECT& rect,
    uint8_t* __restrict dest,
    unsigned dest_pitch,
    const uint8_t* __restrict src,
    unsigned src_pitch);

void CopyRectBGRA(
    const RECT& rect,
    uint8_t* __restrict dest,
    unsigned dest_pitch,
    const uint8_t* __restrict src,
    unsigned src_pitch);


//------------------------------------------------------------------------------
// StoredCursor

struct StoredCursor
{
    // Image data
    std::vector<uint8_t> Rgba;

    // Position and dimensions
    int X = 0, Y = 0;
    int Width = 0, Height = 0;


    bool Empty() const
    {
        return Rgba.empty();
    }

    void PrepareWrite(unsigned width, unsigned height, unsigned x, unsigned y);
};


//------------------------------------------------------------------------------
// DesktopCopyDesc

// Helps organize the rects that need to be copied between staging textures
struct DesktopCopyDesc
{
    // Mouse cursor to erase before normal screen updates
    StoredCursor* CursorToErase = nullptr;

    // Desktop duplication moved rects.  Have not seen these yet but they need
    // to be done before the dirty rects
    DXGI_OUTDUPL_MOVE_RECT* MoveRects = nullptr;
    unsigned MoveCount = 0;

    // Desktop duplication dirty rects
    RECT* DirtyRects = nullptr;
    unsigned DirtyCount = 0;

    // Mouse cursor that should be written for this frame
    StoredCursor* CursorToWrite = nullptr;
};


//------------------------------------------------------------------------------
// StagingTexturePool

struct EpochStagingTexture
{
    StagingTexture Texture;

    uint64_t Epoch = 0;

    int Index = -1;
};

class StagingTexturePool
{
public:
    static const int kMaxAllocated = 8;

    void Shutdown();

    // Allocate a texture from the pool to use for copying
    EpochStagingTexture* Acquire(
        const D3D11_TEXTURE2D_DESC& example,
        D3D11DeviceContext& dc,
        StagingTexture::RW mode,
        unsigned width,
        unsigned height,
        uint64_t epoch);

protected:
    using arena_t = std::vector<std::shared_ptr<EpochStagingTexture>>;

    arena_t SmallArena;
    arena_t LargeArena;

    static bool IsSmall(unsigned width, unsigned height)
    {
        return width <= 128 && height <= 128;
    }
};


//------------------------------------------------------------------------------
// ID3D11DesktopDuplication

class ID3D11DesktopDuplication
{
protected:
    logger::Channel Logger;

public:
    ID3D11DesktopDuplication()
        : Logger("D3D11DesktopDuplication")
    {
    }

    // Initialize after setting above variables
    virtual void Initialize(
        std::shared_ptr<MonitorEnumInfo> info,
        D3D11DeviceContext* vr_device_resources) = 0;

    // Stop background thread and release all objects
    virtual void StartShutdown() = 0;
    virtual void Shutdown() = 0;

    virtual bool IsTerminated() const = 0;

    // Blocks until frame texture is updated with latest frame.
    // Frame may be null if no frame was available.
    // Returns true if a frame was retrieved successfully
    virtual bool AcquireVrRenderTexture() = 0;

    // Release VR render texture after use
    virtual void ReleaseVrRenderTexture() = 0;


    // Indicates an unrecoverable failure
    std::atomic<bool> CaptureFailure = ATOMIC_VAR_INIT(false);

    // Is protected content currently masked out?
    bool ProtectedContentMasked = false;

    // Is rendering currently falling behind the actual desktop?
    bool RenderingFallingBehind = false;

    // Frame texture produced by UpdateFrameTexture()
    // that can be bound for rendering on the VR context
    ComPtr<ID3D11Texture2D> VrRenderTexture;
    D3D11_TEXTURE2D_DESC VrRenderTextureDesc{};
};


} // namespace xrm
