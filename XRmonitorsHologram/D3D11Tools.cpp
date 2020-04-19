// Copyright 2019 Augmented Perception Corporation

#include "stdafx.h"

#include "D3D11Tools.hpp"

#include <d3dcompiler.h>

#include "MonitorTools.hpp"

#include "core_win32.hpp"
#include "core_string.hpp"
#include "core_logger.hpp"

namespace xrm {

using namespace core;

static logger::Channel Logger("D3D11Tools");


//------------------------------------------------------------------------------
// D3D11DeviceContext

bool D3D11DeviceContext::CreateForAdapter(
    IDXGIAdapter1* adapter,
    const std::vector<D3D_FEATURE_LEVEL>& feature_levels)
{
    CORE_DEBUG_ASSERT(adapter != nullptr && !feature_levels.empty());

    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef CORE_DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    const D3D_DRIVER_TYPE type = (adapter == nullptr) ?
        D3D_DRIVER_TYPE_HARDWARE : D3D_DRIVER_TYPE_UNKNOWN;

    D3D_FEATURE_LEVEL feature_level = feature_levels[0];

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;

    HRESULT hr = ::D3D11CreateDevice(adapter,
        type,
        0,
        flags,
        feature_levels.data(),
        (UINT)feature_levels.size(),
        D3D11_SDK_VERSION,
        device.GetAddressOf(),
        &feature_level,
        context.GetAddressOf());

    if (FAILED(hr) || !device || !context) {
        Logger.Warning("Hardware accelerated D3D11CreateDevice failed: ", HresultString(hr));

        // If the initialization fails, fall back to the WARP device.
        // For more information on WARP, see: http://go.microsoft.com/fwlink/?LinkId=286690
        hr = ::D3D11CreateDevice(nullptr,
            D3D_DRIVER_TYPE_WARP,
            0,
            flags,
            feature_levels.data(),
            (UINT)feature_levels.size(),
            D3D11_SDK_VERSION,
            device.GetAddressOf(),
            &feature_level,
            context.GetAddressOf());

        if (FAILED(hr) || !device || !context) {
            Logger.Warning("Fallback WARP D3D11CreateDevice failed: ", HresultString(hr));
            return false;
        }

        hr = device.As(&Device);
        if (FAILED(hr)) {
            Logger.Error("device.As failed: ", HresultString(hr));
            return false;
        }

        hr = context.As(&Context);
        if (FAILED(hr)) {
            Logger.Error("context.As failed: ", HresultString(hr));
            return false;
        }

        Logger.Info("Created slow WARP D3D11 device. Feature level: ",
            D3dFeatureLevelToString(feature_level));
        return true;
    }

    hr = device.As(&Device);
    if (FAILED(hr)) {
        Logger.Error("device.As failed: ", HresultString(hr));
        return false;
    }

    hr = context.As(&Context);
    if (FAILED(hr)) {
        Logger.Error("context.As failed: ", HresultString(hr));
        return false;
    }

    Logger.Info("Created hardware accelerated D3D11 device. Feature level: ",
        D3dFeatureLevelToString(feature_level));
    return true;
}


//------------------------------------------------------------------------------
// Tools

const char* D3dFeatureLevelToString(D3D_FEATURE_LEVEL feature_level)
{
    switch (feature_level)
    {
    case D3D_FEATURE_LEVEL_1_0_CORE: return "D3D_FEATURE_LEVEL_1_0_CORE";
    case D3D_FEATURE_LEVEL_9_1: return "D3D_FEATURE_LEVEL_9_1";
    case D3D_FEATURE_LEVEL_9_2: return "D3D_FEATURE_LEVEL_9_2";
    case D3D_FEATURE_LEVEL_9_3: return "D3D_FEATURE_LEVEL_9_3";
    case D3D_FEATURE_LEVEL_10_0: return "D3D_FEATURE_LEVEL_10_0";
    case D3D_FEATURE_LEVEL_10_1: return "D3D_FEATURE_LEVEL_10_1";
    case D3D_FEATURE_LEVEL_11_0: return "D3D_FEATURE_LEVEL_11_0";
    case D3D_FEATURE_LEVEL_11_1: return "D3D_FEATURE_LEVEL_11_1";
    case D3D_FEATURE_LEVEL_12_0: return "D3D_FEATURE_LEVEL_12_0";
    case D3D_FEATURE_LEVEL_12_1: return "D3D_FEATURE_LEVEL_12_1";
    default: break;
    }
    return "(Invalid)";
}

void PrintD3D11MiscFlags(uint32_t flags)
{
    if (flags & D3D11_RESOURCE_MISC_GENERATE_MIPS) { Logger.Info("D3D11_RESOURCE_MISC_GENERATE_MIPS"); }
    if (flags & D3D11_RESOURCE_MISC_SHARED) { Logger.Info("D3D11_RESOURCE_MISC_SHARED"); }
    if (flags & D3D11_RESOURCE_MISC_TEXTURECUBE) { Logger.Info("D3D11_RESOURCE_MISC_TEXTURECUBE"); }
    if (flags & D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS) { Logger.Info("D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS"); }
    if (flags & D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS) { Logger.Info("D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS"); }
    if (flags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED) { Logger.Info("D3D11_RESOURCE_MISC_BUFFER_STRUCTURED"); }
    if (flags & D3D11_RESOURCE_MISC_RESOURCE_CLAMP) { Logger.Info("D3D11_RESOURCE_MISC_RESOURCE_CLAMP"); }
    if (flags & D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX) { Logger.Info("D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX"); }
    if (flags & D3D11_RESOURCE_MISC_GDI_COMPATIBLE) { Logger.Info("D3D11_RESOURCE_MISC_GDI_COMPATIBLE"); }
    if (flags & D3D11_RESOURCE_MISC_SHARED_NTHANDLE) { Logger.Info("D3D11_RESOURCE_MISC_SHARED_NTHANDLE"); }
    if (flags & D3D11_RESOURCE_MISC_RESTRICTED_CONTENT) { Logger.Info("D3D11_RESOURCE_MISC_RESTRICTED_CONTENT"); }
    if (flags & D3D11_RESOURCE_MISC_RESTRICT_SHARED_RESOURCE) { Logger.Info("D3D11_RESOURCE_MISC_RESTRICT_SHARED_RESOURCE"); }
    if (flags & D3D11_RESOURCE_MISC_RESTRICT_SHARED_RESOURCE_DRIVER) { Logger.Info("D3D11_RESOURCE_MISC_RESTRICT_SHARED_RESOURCE_DRIVER"); }
    if (flags & D3D11_RESOURCE_MISC_GUARDED) { Logger.Info("D3D11_RESOURCE_MISC_GUARDED"); }
    if (flags & D3D11_RESOURCE_MISC_TILE_POOL) { Logger.Info("D3D11_RESOURCE_MISC_TILE_POOL"); }
    if (flags & D3D11_RESOURCE_MISC_TILED) { Logger.Info("D3D11_RESOURCE_MISC_TILED"); }
    if (flags & D3D11_RESOURCE_MISC_HW_PROTECTED) { Logger.Info("D3D11_RESOURCE_MISC_HW_PROTECTED"); }
}


//------------------------------------------------------------------------------
// D3D11 Helpers

ComPtr<ID3DBlob> CompileShader(const char* hlsl, const char* entrypoint, const char* shaderTarget)
{
    ComPtr<ID3DBlob> compiled;
    ComPtr<ID3DBlob> errMsgs;
    DWORD flags = D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR | D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS;

#ifdef CORE_DEBUG
    flags |= D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_DEBUG;
#else
    flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

    HRESULT hr = ::D3DCompile(hlsl,
        strlen(hlsl),
        nullptr,
        nullptr,
        nullptr,
        entrypoint,
        shaderTarget,
        flags,
        0,
        &compiled,
        &errMsgs);

    if (FAILED(hr) || !compiled) {
        Logger.Error("D3DCompile failed: ", HresultString(hr));

        if (errMsgs) {
            Logger.Error("D3DCompile error messages: \n", (const char*)errMsgs->GetBufferPointer());
        }
        else {
            Logger.Warning("D3DCompile: No error messages provided");
        }

        return nullptr;
    }

    return compiled;
}

ComPtr<IDXGIAdapter1> GetAdapterForLuid(LUID luid)
{
    ComPtr<IDXGIFactory2> factory;
    HRESULT hr = ::CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));

    if (FAILED(hr) || !factory) {
        Logger.Error("CreateDXGIFactory2 failed: ", HresultString(hr));
        return nullptr;
    }

    for (UINT adapter_index = 0;; adapter_index++)
    {
        ComPtr<IDXGIAdapter1> adapter;

        hr = factory->EnumAdapters1(adapter_index, &adapter);

        if (FAILED(hr) || !adapter)
        {
            // EnumAdapters1 will fail with DXGI_ERROR_NOT_FOUND when there are no more adapters to enumerate.
            if (hr == DXGI_ERROR_NOT_FOUND) {
                break;
            }

            Logger.Error("EnumAdapters1 failed: ", HresultString(hr));
            return nullptr;
        }

        DXGI_ADAPTER_DESC1 desc{};
        hr = adapter->GetDesc1(&desc);

        if (!LUIDMatch(desc.AdapterLuid, luid)) {
            continue;
        }

        Logger.Info("Found matching XR graphics adapter: ", WideStringToUtf8String(desc.Description));

        return adapter;
    }

    Logger.Error("Failed to find adapter for LUID: ", LUIDToString(luid));
    return nullptr;
}


//------------------------------------------------------------------------------
// StagingTexture

bool StagingTexture::Prepare(
    D3D11DeviceContext& dc,
    RW mode,
    unsigned width,
    unsigned height,
    unsigned mip_levels,
    DXGI_FORMAT format,
    unsigned samples)
{
    // If texture must be recreated:
    if (!Texture ||
        Desc.Width != width ||
        Desc.Height != height ||
        Desc.Format != format)
    {
        Logger.Info("Recreating staging texture ( ",
            width, "x", height, " )");

        Texture.Reset();

        Desc = D3D11_TEXTURE2D_DESC();
        Desc.Width = width;
        Desc.Height = height;
        Desc.MipLevels = mip_levels;
        Desc.Format = format;
        Desc.SampleDesc.Count = samples;
        Desc.ArraySize = 1;

        Desc.Usage = D3D11_USAGE_STAGING;
        if (mode == RW::ReadOnly) {
            Desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            MapType = D3D11_MAP_READ;
        }
        else {
            Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            MapType = D3D11_MAP_WRITE;
        }

        HRESULT hr = dc.Device->CreateTexture2D(
            &Desc,
            nullptr,
            &Texture);

        if (FAILED(hr)) {
            Logger.Error("CreateTexture2D failed: ", HresultString(hr));
            return false;
        }
    }

    return true;
}

bool StagingTexture::Map(D3D11DeviceContext& dc)
{
    CORE_DEBUG_ASSERT(!MappedData); // Never unmapped

    const UINT subresource_index = D3D11CalcSubresource(0, 0, 0);
    D3D11_MAPPED_SUBRESOURCE subresource{};

#ifdef DD_MAP_TIMING
    const uint64_t t0 = GetTimeUsec();
#endif

    HRESULT hr = dc.Context->Map(
        Texture.Get(),
        subresource_index,
        MapType,
        0, // flags
        &subresource);

    if (FAILED(hr) || !subresource.pData) {
        CORE_DEBUG_BREAK();
        Logger.Error("Map failed: ", HresultString(hr));
        return false;
    }

#ifdef DD_MAP_TIMING
    const uint64_t t1 = GetTimeUsec();
    Logger.Info("Map: ", (t1 - t0), " usec ", Desc.Width, "x", Desc.Height);
#endif

    MappedPitch = subresource.RowPitch;
    MappedData = (uint8_t*)subresource.pData;
    return true;
}

void StagingTexture::Unmap(D3D11DeviceContext& dc)
{
    if (MappedData)
    {
        const UINT subresource_index = D3D11CalcSubresource(0, 0, 0);
        dc.Context->Unmap(Texture.Get(), subresource_index);

        MappedData = nullptr;
    }
}

void StagingTexture::Reset()
{
    CORE_DEBUG_ASSERT(!MappedData);

    Texture.Reset();
}


} // namespace xrm
