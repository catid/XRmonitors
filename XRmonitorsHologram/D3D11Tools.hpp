// Copyright 2019 Augmented Perception Corporation

#pragma once

#include <DXGI.h>
#include <d3d11.h>
#include <wrl/client.h>

#include <DirectXHelpers.h> // MapGuard
#include <VertexTypes.h> // VertexPositionTexture
#include <SimpleMath.h> // Quaternion

namespace xrm {

using namespace Microsoft::WRL;


//------------------------------------------------------------------------------
// D3D11DeviceContext

struct D3D11DeviceContext
{
    ComPtr<ID3D11Device1> Device;
    ComPtr<ID3D11DeviceContext1> Context;

    /*
        Given a DXGI adapter, this creates the highest feature level
        D3D11Device and D3D11DeviceContext it can.

        Returns true if adapter could be created.
        Returns false if adapter could not be created.
    */
    bool CreateForAdapter(
        IDXGIAdapter1* adapter,
        const std::vector<D3D_FEATURE_LEVEL>& feature_levels);
};


//------------------------------------------------------------------------------
// Stringizers

// D3D_FEATURE_LEVEL -> string
const char* D3dFeatureLevelToString(D3D_FEATURE_LEVEL feature_level);

// D3D11_RESOURCE_MISC_* -> log
void PrintD3D11MiscFlags(uint32_t flags);


//------------------------------------------------------------------------------
// D3D11 Helpers

// Get adapter for the given LUID.
// Used to get the DXGI adapter that the VR headset is plugged into
ComPtr<IDXGIAdapter1> GetAdapterForLuid(LUID luid);

// Compile a shader to a loadable blob
ComPtr<ID3DBlob> CompileShader(
    const char* hlsl,
    const char* entrypoint,
    const char* shaderTarget);


//------------------------------------------------------------------------------
// StagingTexture

class StagingTexture
{
public:
    ~StagingTexture()
    {
        CORE_DEBUG_ASSERT(!MappedData); // Never unmapped
    }

    enum class RW {
        ReadOnly,
        WriteOnly
    };

    /// Recreate texture if needed
    bool Prepare(
        D3D11DeviceContext& dc,
        RW mode,
        unsigned width,
        unsigned height,
        unsigned mip_levels,
        DXGI_FORMAT format,
        unsigned samples);

    /// Release texture and map
    void Reset();

    /// Returns false on failure
    bool Map(D3D11DeviceContext& dc);

    /// Must be called after successful Map()
    void Unmap(D3D11DeviceContext& dc);

    ID3D11Texture2D* GetTexture() const
    {
        return Texture.Get();
    }
    uint8_t* GetMappedData() const
    {
        return MappedData;
    }
    unsigned GetMappedPitch() const
    {
        return MappedPitch;
    }
    unsigned Width() const
    {
        return Desc.Width;
    }
    unsigned Height() const
    {
        return Desc.Height;
    }

protected:
    D3D11_TEXTURE2D_DESC Desc{};
    ComPtr<ID3D11Texture2D> Texture;

    D3D11_MAP MapType;
    uint8_t* MappedData = nullptr;
    unsigned MappedPitch = 0;
};


} // namespace xrm
