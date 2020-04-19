// Copyright 2019 Augmented Perception Corporation

#include "stdafx.h"

#include "D3D11DuplicationCommon.hpp"

#include <immintrin.h>
#include <cstdint>
#include <comdef.h>

namespace xrm {

static logger::Channel Logger("Duplication");


//------------------------------------------------------------------------------
// Tools

void MemCopy2D(
    const unsigned rows,
    const unsigned cols_bytes,
    uint8_t* __restrict dest,
    const uint8_t* __restrict src,
    const unsigned dest_pitch,
    const unsigned src_pitch)
{
    for (unsigned i = 0; i < rows; ++i)
    {
        memcpy(dest, src, cols_bytes);

        src += src_pitch;
        dest += dest_pitch;
    }
}

void AlignedCopyRectBGRA(
    const RECT& rect,
    uint8_t* __restrict dest,
    unsigned dest_pitch,
    const uint8_t* __restrict src,
    unsigned src_pitch)
{
    src += rect.top * src_pitch + rect.left * 4;
    dest += rect.top * dest_pitch + rect.left * 4;

    const unsigned rows = rect.bottom - rect.top;
    const unsigned cols_bytes = (rect.right - rect.left) * 4;

    //Logger.Info("rows: ", rows, " cols_bytes: ", cols_bytes);

    // If there may not be even one vector word to stream:
    if (cols_bytes < 64) {
        MemCopy2D(rows, cols_bytes, dest, src, dest_pitch, src_pitch);
        return;
    }

    const unsigned leading = 32 - (unsigned)((uintptr_t)dest & 31);

    uintptr_t vector_start = (uintptr_t)dest + leading;
    uintptr_t bytes_end = (uintptr_t)dest + cols_bytes;

    const unsigned nVects = (unsigned)(bytes_end - vector_start) / 32;
    const unsigned vector_count_bytes = nVects * 32;

    CORE_DEBUG_ASSERT(cols_bytes >= leading + vector_count_bytes);
    const unsigned trailing = cols_bytes - leading - vector_count_bytes;

//#pragma omp parallel for num_threads(2) default(none)
    for (unsigned i = 0; i < rows; ++i)
    {
        uint8_t* __restrict row_dest = dest + i * dest_pitch;
        const uint8_t* __restrict row_src = src + i * src_pitch;

        memcpy(row_dest, row_src, leading);

        const __m256i* pSrc = reinterpret_cast<const __m256i* __restrict>(row_src + leading);
        __m256i* pDest = reinterpret_cast<__m256i* __restrict>(row_dest + leading);

#if 0
        while (nVects >= 8) {
            _mm256_stream_si256(pDest + 0, _mm256_stream_load_si256(pSrc + 0));
            _mm256_stream_si256(pDest + 1, _mm256_stream_load_si256(pSrc + 1));
            _mm256_stream_si256(pDest + 2, _mm256_stream_load_si256(pSrc + 2));
            _mm256_stream_si256(pDest + 3, _mm256_stream_load_si256(pSrc + 3));
            _mm256_stream_si256(pDest + 4, _mm256_stream_load_si256(pSrc + 4));
            _mm256_stream_si256(pDest + 5, _mm256_stream_load_si256(pSrc + 5));
            _mm256_stream_si256(pDest + 6, _mm256_stream_load_si256(pSrc + 6));
            _mm256_stream_si256(pDest + 7, _mm256_stream_load_si256(pSrc + 7));

            pDest += 8;
            pSrc += 8;
            nVects -= 8;
        }
#endif

        for (unsigned j = 0; j < nVects; ++j) {
            _mm256_stream_si256(pDest + j, _mm256_stream_load_si256(pSrc + j));
        }

        memcpy(row_dest + leading + vector_count_bytes, row_src + leading + vector_count_bytes, trailing);
    }

    _mm_sfence();
}

void CopyRectBGRA(
    const RECT& rect,
    uint8_t* __restrict dest,
    unsigned dest_pitch,
    const uint8_t* __restrict src,
    unsigned src_pitch)
{
#if 1
    if ((uintptr_t)dest % 32 == 0 &&
        (uintptr_t)src % 32 == 0 &&
        dest_pitch % 32 == 0 &&
        src_pitch % 32 == 0)
    {
        AlignedCopyRectBGRA(rect, dest, dest_pitch, src, src_pitch);
        return;
    }
#endif

    src += rect.top * src_pitch + rect.left * 4;
    dest += rect.top * dest_pitch + rect.left * 4;

    const unsigned rows = rect.bottom - rect.top;
    const unsigned cols_bytes = (rect.right - rect.left) * 4;

    MemCopy2D(rows, cols_bytes, dest, src, dest_pitch, src_pitch);
}


//------------------------------------------------------------------------------
// StagingTexturePool

static unsigned RoundUpToNextMultipleOf64(unsigned x)
{
    x += 63;
    x /= 64;
    x *= 64;
    return x;
}

void StagingTexturePool::Shutdown()
{
    SmallArena.clear();
    LargeArena.clear();
}

EpochStagingTexture* StagingTexturePool::Acquire(
    const D3D11_TEXTURE2D_DESC& example,
    D3D11DeviceContext& dc,
    StagingTexture::RW mode,
    unsigned width,
    unsigned height,
    uint64_t epoch)
{
    arena_t* arena = nullptr;

    if (IsSmall(width, height)) {
        arena = &SmallArena;
    }
    else {
        arena = &LargeArena;
    }

    uint64_t largest_epoch_delta = 0;
    int largest_epoch_i = 0;

    unsigned smallest_size = 0;
    int smallest_i = 0;

    const int count = (int)arena->size();
    for (int i = 0; i < count; ++i)
    {
        EpochStagingTexture* texture = arena->at(i).get();

        if (texture->Epoch == epoch) {
            continue;
        }

        const unsigned tw = texture->Texture.Width();
        const unsigned th = texture->Texture.Height();

        if (tw >= width && th >= height)
        {
            texture->Epoch = epoch;
            return texture;
        }

        uint64_t epoch_delta = epoch - texture->Epoch;
        if (largest_epoch_delta < epoch_delta) {
            largest_epoch_delta = epoch_delta;
            largest_epoch_i = i;
        }
    }

    std::shared_ptr<EpochStagingTexture> texture = std::make_shared<EpochStagingTexture>();

    texture->Epoch = epoch;
    bool prep = texture->Texture.Prepare(
        dc,
        mode,
        RoundUpToNextMultipleOf64(width),
        RoundUpToNextMultipleOf64(height),
        example.MipLevels,
        example.Format,
        example.SampleDesc.Count);
    if (!prep) {
        return nullptr;
    }

    if (count < kMaxAllocated) {
        texture->Index = (int)arena->size();
        arena->push_back(texture);
    }
    else {
        texture->Index = largest_epoch_i;
        arena->at(largest_epoch_i).reset();
        arena->at(largest_epoch_i) = texture;
    }

    return texture.get();
}


//------------------------------------------------------------------------------
// StoredCursor

void StoredCursor::PrepareWrite(
    unsigned width,
    unsigned height,
    unsigned x,
    unsigned y)
{
    const unsigned bytes = width * height * 4;
    if (bytes <= 0) {
        return;
    }
    if (Rgba.size() < bytes) {
        Rgba.resize(bytes);
    }
    Width = width;
    Height = height;
    X = x;
    Y = y;
}


} // namespace xrm
