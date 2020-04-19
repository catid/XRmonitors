// Copyright 2019 Augmented Perception Corporation

#include "stdafx.h"

#include "Plugins.hpp"
#include "MonitorTools.hpp"

namespace xrm {

using Microsoft::WRL::ComPtr;
using namespace DirectX;

static logger::Channel Logger("Plugins");


//------------------------------------------------------------------------------
// Tools

static void ApplyTransform(
    VertexPositionTexture& vertex,
    const Matrix& transform)
{
    Vector3 p(vertex.position.x, vertex.position.y, vertex.position.z);
    Vector3 q = Vector3::Transform(p, transform);
    vertex.position.x = q.x;
    vertex.position.y = q.y;
    vertex.position.z = q.z;
}


//------------------------------------------------------------------------------
// PluginServer

void PluginServer::Initialize(
    int plugin_index,
    XrmPluginsMemoryLayout* shared_memory,
    XrHeadsetProperties* headset,
    XrRenderProperties* rendering,
    MonitorRenderModel* model,
    uint32_t mip_levels,
    uint32_t sample_count)
{
    std::ostringstream oss;
    oss << "[ " << plugin_index << " ] ";
    Logger.SetPrefix(oss.str());

    Headset = headset;
    Rendering = rendering;
    RenderModel = model;
    PluginIndex = plugin_index;

    memset(&HostData, 0, sizeof(HostData));
    memset(&PluginData, 0, sizeof(PluginData));

    HostToPlugin = &shared_memory->HostToPlugin[PluginIndex];
    PluginToHost = &shared_memory->PluginToHost[PluginIndex];

    HostData.Shutdown = 0;
    HostData.TerminateEpoch = 0;
    HostData.Format = rendering->SwapchainFormat;
    HostData.MipLevels = mip_levels;
    HostData.SampleCount = sample_count;
    HostData.LuidHighPart = Headset->AdapterLuid.HighPart;
    HostData.LuidLowPart = Headset->AdapterLuid.LowPart;

    HostToPlugin->Write(HostData);
    UpdateEvent.Signal();

    Timeout.SetTimeout(kTimeoutMsec);

    if (!UpdateEvent.Open(plugin_index)) {
        Logger.Error("Failed to open event for plugin ", plugin_index);
    }
}

void PluginServer::Shutdown()
{
    HostData.Shutdown = 1;
    HostData.TerminateEpoch++;
    HostToPlugin->Write(HostData);
    UpdateEvent.Signal();
}

void PluginServer::SetFocusAndGaze(int screen_x, int screen_y, bool focus)
{
    const bool focus_changed = ((HostData.HasFocus != 0) != focus);

    if (focus_changed) {
        Logger.Info("Gaze focus: ", focus);
    }

    HostData.HasFocus = focus;
    HostData.GazeScreenX = screen_x;
    HostData.GazeScreenY = screen_y;
    HostToPlugin->Write(HostData);

    if (focus_changed) {
        UpdateEvent.Signal();
    }
}

void PluginServer::SignalTerminate()
{
    HostData.TerminateEpoch++;
    HostToPlugin->Write(HostData);
    UpdateEvent.Signal();
}

bool PluginServer::HitTest(int screen_x, int screen_y)
{
    if (std::abs(PluginData.X - screen_x) > PluginData.Width / 2) {
        return false;
    }
    if (std::abs(PluginData.Y - screen_y) > PluginData.Height / 2) {
        return false;
    }
    return true;
}

bool PluginServer::UpdateRenderModel()
{
    PluginRenderInfo* info = &RenderModel->Plugins[PluginIndex];

    // By default will disable render with Texture = null
    info->Texture = nullptr;

    if (!PluginToHost) {
        return false;
    }

    const uint32_t old_keep_alive_epoch = PluginData.KeepAliveEpoch;
    const uint32_t old_texture_handle_epoch = PluginData.TextureHandleEpoch;
    const uint32_t old_texture_release_epoch = PluginData.TextureReleaseEpoch;
    const int32_t old_x = PluginData.X;
    const int32_t old_y = PluginData.Y;
    const int32_t old_width = PluginData.Width;
    const int32_t old_height = PluginData.Height;

    const bool success = PluginToHost->Read(PluginEpoch, PluginData);

    // If keepalive is received:
    if (old_keep_alive_epoch != PluginData.KeepAliveEpoch) {
        Timeout.Reset();
    }

    // If no shared texture for this plugin:
    if (PluginData.TextureHandleEpoch == 0) {
        return false; // No shared texture update
    }

    // If shared texture changed:
    const bool tex_changed = (PluginData.TextureHandleEpoch != old_texture_handle_epoch);
    if (tex_changed) {
        if (!UpdateSharedTexture()) {
            Logger.Error("UpdateSharedTexture failed");
            return false;
        }
        if (!UpdateVrTexture()) {
            Logger.Error("UpdateVrTexture failed");
            return false;
        }
        Logger.Info("Successfully accessed plugin texture");
        Timeout.Reset();
    }

    // Ignore texture updates if we failed during setup
    if (!VrRenderTexture || !SharedTexture) {
        return false;
    }

    info->Texture = VrRenderTexture.Get();
    info->TextureFormat = (DXGI_FORMAT)HostData.Format;
    info->MultisamplingEnabled = HostData.SampleCount > 1;

    //Logger.Info("Texture ready: X=", PluginData.X, " Y=", PluginData.Y);

    info->X = PluginData.X;
    info->Y = PluginData.Y;
    info->Width = PluginData.Width;
    info->Height = PluginData.Height;

    // To be filled in by caller
    info->EnableBlueLightFilter = false;

    const bool rect_changed = success && (
        PluginData.X != old_x ||
        PluginData.Y != old_y ||
        PluginData.Width != old_width ||
        PluginData.Height != old_height);

    // If mesh must be updated:
    if (rect_changed || tex_changed) {
        UpdateMesh();
    }

    info->IndexBuffer = IndexBuffer.Get();
    info->VertexBuffer = VertexBuffer.Get();
    info->IndexCount = IndexCount;

    // If texture release epoch changed implying texture mutex released to host:
    if (old_texture_release_epoch != PluginData.TextureReleaseEpoch)
    {
        if (!CopyTexture()) {
            Logger.Error("CopyTexture failed");
            return false;
        }
    }

    return true;
}

void PluginServer::CheckTimeout()
{
    // If already inactive:
    if (!VrRenderTexture) {
        return;
    }

    if (Timeout.Timeout())
    {
        Logger.Warning("Plugin application timed out");
        ReleaseRemoteTexture();
        SignalTerminate();
        return;
    }
}

void PluginServer::UpdateMesh()
{
    // If inactive:
    if (!VrRenderTexture) {
        return;
    }

    const float radius_m = RenderModel->HmdFocalDistanceMeters - 0.001f;

    Logger.Info("Updating mesh");

    Vertices.clear();

    int left = PluginData.X - PluginData.Width / 2;
    int right = PluginData.X + (PluginData.Width + 1) / 2;
    int top = PluginData.Y - PluginData.Height / 2;
    int bottom = PluginData.Y + (PluginData.Height + 1) / 2;

    float x0 = RenderModel->MetersPerPixel * (left - RenderModel->ScreenFocusCenterX);
    float x1 = RenderModel->MetersPerPixel * (right - RenderModel->ScreenFocusCenterX);
    float y0 = RenderModel->MetersPerPixel * (top - RenderModel->ScreenFocusCenterY);
    float y1 = RenderModel->MetersPerPixel * (bottom - RenderModel->ScreenFocusCenterY);

    float len_m = x1 - x0;
    int target_quad_count = static_cast<int>(len_m / 0.01f);
    if (target_quad_count > 50) {
        target_quad_count = 50;
    }
    else if (target_quad_count < 4) {
        target_quad_count = 4;
    }
    float dx = len_m / target_quad_count;
    float du = 1.f / target_quad_count;

    // Make sure we do not leave very slim geometry on the right cap
    const float x1_end = x1 - dx * 1.5f;
    const float inv_radius_m = 1.f / radius_m;

    int quad_count = 0;
    for (float x = x0, u = 0.f; x < x1_end; x += dx, u += du)
    {
        VertexPositionTexture top, bottom;

        // (s, t) = (x, z)
        // theta = Pi/2 -> (r, 0)
        // theta = 0 -> (0, -r)
        // theta = -Pi/2 -> (-r, 0)
        // theta = Pi -> (0, r)
        const float theta = x * inv_radius_m;
        const float s = radius_m * cosf(theta);
        const float t = radius_m * sinf(theta);

        top.position.x = s;
        top.position.y = y0;
        top.position.z = t;
        top.textureCoordinate.x = u;
        top.textureCoordinate.y = 1.f;

        ApplyTransform(top, RenderModel->PoseTransform);

        bottom.position.x = s;
        bottom.position.y = y1;
        bottom.position.z = t;
        bottom.textureCoordinate.x = u;
        bottom.textureCoordinate.y = 0.f;

        ApplyTransform(bottom, RenderModel->PoseTransform);

        Vertices.push_back(top);
        Vertices.push_back(bottom);
        ++quad_count;
    }

    // Final column:
    {
        VertexPositionTexture top, bottom;

        const float theta = x1 * inv_radius_m;
        const float s = radius_m * cosf(theta);
        const float t = radius_m * sinf(theta);

        top.position.x = s;
        top.position.y = y0;
        top.position.z = t;
        top.textureCoordinate.x = 1.f;
        top.textureCoordinate.y = 1.f;

        ApplyTransform(top, RenderModel->PoseTransform);

        bottom.position.x = s;
        bottom.position.y = y1;
        bottom.position.z = t;
        bottom.textureCoordinate.x = 1.f;
        bottom.textureCoordinate.y = 0.f;

        ApplyTransform(bottom, RenderModel->PoseTransform);

        Vertices.push_back(top);
        Vertices.push_back(bottom);
    }

    Indices.clear();

    // Counter-clockwise winding facing the center
    for (int i = 0; i < quad_count; ++i)
    {
        Indices.push_back(i * 2);
        Indices.push_back(i * 2 + 1);
        Indices.push_back(i * 2 + 2);

        Indices.push_back(i * 2 + 2);
        Indices.push_back(i * 2 + 1);
        Indices.push_back(i * 2 + 3);
    }

    const D3D11_SUBRESOURCE_DATA vertex_data{ Vertices.data() };
    const CD3D11_BUFFER_DESC vertex_desc(
        Vertices.size() * sizeof(VertexPositionTexture),
        D3D11_BIND_VERTEX_BUFFER);
    XR_CHECK_HRCMD(Rendering->DeviceContext.Device->CreateBuffer(
        &vertex_desc,
        &vertex_data,
        VertexBuffer.ReleaseAndGetAddressOf()));

    const D3D11_SUBRESOURCE_DATA index_data{ Indices.data() };
    const CD3D11_BUFFER_DESC index_desc(
        Indices.size() * sizeof(uint16_t),
        D3D11_BIND_INDEX_BUFFER);
    XR_CHECK_HRCMD(Rendering->DeviceContext.Device->CreateBuffer(
        &index_desc,
        &index_data,
        IndexBuffer.ReleaseAndGetAddressOf()));

    IndexCount = (int)Indices.size();
}

static std::wstring GetSharedTextureName(int plugin)
{
    std::wostringstream woss;
    woss << L"XRmonitors_" << plugin;
    return woss.str();
}

bool PluginServer::UpdateSharedTexture()
{
    Logger.Info("UpdateSharedTexture");

    ReleaseRemoteTexture();

    const std::wstring name = GetSharedTextureName(PluginIndex);

    HRESULT hr = Rendering->DeviceContext.Device->OpenSharedResourceByName(
        name.data(),
        DXGI_SHARED_RESOURCE_READ,
        __uuidof(ID3D11Texture2D),
        reinterpret_cast<void**>(SharedTexture.ReleaseAndGetAddressOf()));

    if (FAILED(hr)) {
        Logger.Error("OpenSharedResource ID3D11Texture2D failed: ", HresultString(hr));
        return false;
    }

    hr = SharedTexture->QueryInterface(
        __uuidof(IDXGIKeyedMutex),
        reinterpret_cast<void**>(KeyedMutex.ReleaseAndGetAddressOf()));

    if (FAILED(hr)) {
        Logger.Error("KeyedMutex QueryInterface IDXGIKeyedMutex failed: ", HresultString(hr));
        return false;
    }

    return true;
}

void PluginServer::ReleaseRemoteTexture()
{
    Logger.Info("Releasing remote texture");

    // Release shared textures
    KeyedMutex.Reset();
    SharedTexture.Reset();
    MutexAcquired = false;
    VrRenderTexture.Reset();
    RenderModel->Plugins[PluginIndex].Texture = nullptr;
}

bool PluginServer::UpdateVrTexture()
{
    // If texture must be recreated:
    if (!VrRenderTexture ||
        PluginData.Width != VrRenderTextureDesc.Width ||
        PluginData.Height != VrRenderTextureDesc.Height)
    {
        Logger.Info("Recreating VrRenderTexture ( ",
            PluginData.Width, "x", PluginData.Height, " )");

        VrRenderTexture.Reset();

        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = PluginData.Width;
        desc.Height = PluginData.Height;
        desc.MipLevels = HostData.MipLevels;
        desc.ArraySize = 1;
        desc.Format = (DXGI_FORMAT)HostData.Format;
        desc.SampleDesc.Count = HostData.SampleCount;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

        VrRenderTextureDesc = desc;

        HRESULT hr = Rendering->DeviceContext.Device->CreateTexture2D(
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

bool PluginServer::CopyTexture()
{
    //Logger.Info("CopyTexture");

    HRESULT hr = KeyedMutex->AcquireSync(kKeyedMutex_Host, 0);
    if (FAILED(hr))
    {
        Logger.Error("AcquireSync failed: ", HresultString(hr));
        return false;
    }

    D3D11_BOX box;
    box.left = 0;
    box.right = PluginData.Width;
    box.top = 0;
    box.bottom = PluginData.Height;
    box.front = 0;
    box.back = 1;

    Rendering->DeviceContext.Context->CopySubresourceRegion1(
        VrRenderTexture.Get(),
        0, // subresource 0
        0, // X
        0, // Y
        0, // Z
        SharedTexture.Get(),
        0, // subresource 0,
        &box,
        D3D11_COPY_DISCARD);

    // Release shared texture access
    hr = KeyedMutex->ReleaseSync(kKeyedMutex_Plugin);
    if (FAILED(hr)) {
        Logger.Error("ReleaseSync failed: ", HresultString(hr));
    }

    // Write host data
    HostData.TextureReleaseEpoch++;
    HostToPlugin->Write(HostData);
    UpdateEvent.Signal();

    return true;
}


//------------------------------------------------------------------------------
// PluginCylinderRenderer

struct PluginModelViewProjectionConstantBuffer
{
    XMFLOAT4X4 ModelViewProjection;
};

struct PluginCylinderColorConstantBuffer
{
    XMFLOAT4 ColorAdjustment;
};

constexpr char kCylinderVertexShaderHlsl[] = R"_(
    cbuffer ModelConstantBuffer : register(b0) {
        float4x4 ModelViewProjection;
    };

    struct Vertex {
        float3 Pos : POSITION;
        float2 Tex : TEXCOORD0;
    };

    // "sample" enables SSAA
    struct PSVertex {
        sample float4 Pos : SV_POSITION;
        sample float2 Tex : TEXCOORD0;
    };

    PSVertex MainVS(Vertex input) {
       PSVertex output;
       output.Pos = mul(float4(input.Pos, 1), ModelViewProjection);
       output.Tex = input.Tex;
       return output;
    }
)_";

constexpr char kCylinderPixelShaderHlsl[] = R"_(
    cbuffer ColorConstantBuffer : register(b0) {
        float4 ColorAdjust;
    };

    // "sample" enables SSAA
    struct PSVertex {
        sample float4 Pos : SV_POSITION;
        sample float2 Tex : TEXCOORD0;
    };

    Texture2D shaderTexture;
    SamplerState SampleType;

    float4 MainPS(PSVertex input) : SV_TARGET {
        float4 color = shaderTexture.Sample(SampleType, input.Tex);
        color.b *= ColorAdjust.y;
        return color;
    }
)_";

void PluginCylinderRenderer::InitializeD3D(D3D11DeviceContext& device_context)
{
    const ComPtr<ID3DBlob> vertexShaderBytes = CompileShader(kCylinderVertexShaderHlsl, "MainVS", "vs_5_0");
    XR_CHECK(vertexShaderBytes != nullptr);
    XR_CHECK_HRCMD(device_context.Device->CreateVertexShader(
        vertexShaderBytes->GetBufferPointer(),
        vertexShaderBytes->GetBufferSize(),
        nullptr,
        VertexShader.ReleaseAndGetAddressOf()));

    const ComPtr<ID3DBlob> pixelShaderBytes = CompileShader(kCylinderPixelShaderHlsl, "MainPS", "ps_5_0");
    XR_CHECK(pixelShaderBytes != nullptr);
    XR_CHECK_HRCMD(device_context.Device->CreatePixelShader(
        pixelShaderBytes->GetBufferPointer(),
        pixelShaderBytes->GetBufferSize(),
        nullptr,
        PixelShader.ReleaseAndGetAddressOf()));

    const D3D11_INPUT_ELEMENT_DESC vertex_desc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    XR_CHECK_HRCMD(device_context.Device->CreateInputLayout(
        vertex_desc,
        (UINT)std::size(vertex_desc),
        vertexShaderBytes->GetBufferPointer(),
        vertexShaderBytes->GetBufferSize(),
        &InputLayout));

    const CD3D11_BUFFER_DESC mvpConstantBufferDesc(
        sizeof(PluginModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
    XR_CHECK_HRCMD(device_context.Device->CreateBuffer(
        &mvpConstantBufferDesc,
        nullptr,
        MvpCBuffer.ReleaseAndGetAddressOf()));

    const CD3D11_BUFFER_DESC colorConstantBufferDesc(
        sizeof(PluginCylinderColorConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
    XR_CHECK_HRCMD(device_context.Device->CreateBuffer(
        &colorConstantBufferDesc,
        nullptr,
        ColorCBuffer.ReleaseAndGetAddressOf()));

    // Best so far: D3D11_FILTER_ANISOTROPIC
    // Feels like both my eyes see the same thing

    // D3D11_FILTER_MIN_MAG_MIP_POINT
    // D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR
    // Head-locked: Left/right eye disparity with text.
    // Not head-locked: Text is jumbled up with any rotation.

    // D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT
    // Loses bits of text

    // D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR
    // Loses the - in the E

    // D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR
    // Head-locked: Little wobble in the _s
    // Free: Little wobble

    // D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT
    // Very small wobble

    // D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT
    // Very very small wobble
    // Very similar to D3D11_FILTER_MIN_MAG_MIP_LINEAR

    // Create a texture sampler state description.
    D3D11_SAMPLER_DESC sampler_desc{};
    sampler_desc.Filter = D3D11_FILTER_ANISOTROPIC;
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.MipLODBias = 0.0f;
    sampler_desc.MaxAnisotropy = D3D11_DEFAULT_MAX_ANISOTROPY;
    sampler_desc.ComparisonFunc = D3D11_COMPARISON_LESS; // FIXME: Depth buffer...
    sampler_desc.BorderColor[0] = 0;
    sampler_desc.BorderColor[1] = 0;
    sampler_desc.BorderColor[2] = 0;
    sampler_desc.BorderColor[3] = 0;
    sampler_desc.MinLOD = 0;
    sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;

    // Create the texture sampler state.
    XR_CHECK_HRCMD(device_context.Device->CreateSamplerState(
        &sampler_desc,
        &SamplerState));
    XR_CHECK(SamplerState != nullptr);

    // TBD: Check if alpha blend improves border quality

    D3D11_BLEND_DESC blend_desc{};
    blend_desc.AlphaToCoverageEnable = FALSE;
    blend_desc.IndependentBlendEnable = FALSE;
    auto& rtblend = blend_desc.RenderTarget[0];
    rtblend.BlendEnable = FALSE; // TBD
    rtblend.BlendOp = D3D11_BLEND_OP_ADD;
    rtblend.SrcBlend = D3D11_BLEND_SRC_ALPHA;
    rtblend.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    rtblend.SrcBlendAlpha = D3D11_BLEND_ONE;
    rtblend.DestBlendAlpha = D3D11_BLEND_ZERO;
    rtblend.BlendOpAlpha = D3D11_BLEND_OP_ADD;
    rtblend.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    // Create the texture sampler state.
    XR_CHECK_HRCMD(device_context.Device->CreateBlendState(
        &blend_desc,
        &BlendState));
}

void PluginCylinderRenderer::Render(
    D3D11DeviceContext& device_context,
    const DirectX::SimpleMath::Matrix& view_projection_matrix,
    const PluginRenderInfo* info)
{
    CORE_DEBUG_ASSERT(info->Texture != nullptr);
    CORE_DEBUG_ASSERT(info->VertexBuffer != nullptr);
    CORE_DEBUG_ASSERT(info->IndexBuffer != nullptr);
    CORE_DEBUG_ASSERT(VertexShader.Get() != nullptr);
    CORE_DEBUG_ASSERT(PixelShader.Get() != nullptr);
    CORE_DEBUG_ASSERT(MvpCBuffer.Get() != nullptr);
    CORE_DEBUG_ASSERT(ColorCBuffer.Get() != nullptr);
    CORE_DEBUG_ASSERT(device_context.Context.Get() != nullptr);

    //Logger.Info("PluginCylinderRenderer: Active");

    ID3D11DeviceContext* context = device_context.Context.Get();

    ID3D11Buffer* const vsConstantBuffers[] = { MvpCBuffer.Get() };
    context->VSSetConstantBuffers(0, (UINT)CORE_ARRAY_COUNT(vsConstantBuffers), vsConstantBuffers);

    ID3D11Buffer* const psConstantBuffers[] = { ColorCBuffer.Get() };
    context->PSSetConstantBuffers(0, (UINT)CORE_ARRAY_COUNT(psConstantBuffers), psConstantBuffers);

    context->VSSetShader(VertexShader.Get(), nullptr, 0);
    context->PSSetShader(PixelShader.Get(), nullptr, 0);

    // Set cube primitive data:

    const UINT strides[] = { sizeof(VertexPositionTexture) };
    const UINT offsets[] = { 0 };
    ID3D11Buffer* vertexBuffers[] = { info->VertexBuffer };

    context->IASetVertexBuffers(0, (UINT)CORE_ARRAY_COUNT(vertexBuffers), vertexBuffers, strides, offsets);
    context->IASetIndexBuffer(info->IndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(InputLayout.Get());

    // We do not adjust the model at all at render time

    Matrix mvpMatrix = view_projection_matrix;

    PluginModelViewProjectionConstantBuffer mvp;
    mvp.ModelViewProjection = mvpMatrix.Transpose();

    context->UpdateSubresource(MvpCBuffer.Get(), 0, nullptr, &mvp, 0, 0);

    PluginCylinderColorConstantBuffer color{};
    if (info->EnableBlueLightFilter) {
        color.ColorAdjustment.y = 0.5f;
    }
    else {
        color.ColorAdjustment.y = 1.f;
    }

    context->UpdateSubresource(ColorCBuffer.Get(), 0, nullptr, &color, 0, 0);

    // Set texture

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};
    srv_desc.Format = info->TextureFormat;
    srv_desc.ViewDimension = info->MultisamplingEnabled ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MostDetailedMip = 0;
    srv_desc.Texture2D.MipLevels = -1;

    ComPtr<ID3D11ShaderResourceView> srv;
    XR_CHECK_HRCMD(device_context.Device->
        CreateShaderResourceView(
            info->Texture,
            &srv_desc,
            &srv));

#ifdef ENABLE_DUPE_MIP_LEVELS
    context->GenerateMips(srv.Get());
#endif

    context->PSSetSamplers(0, 1, SamplerState.GetAddressOf());
    context->PSSetShaderResources(0, 1, srv.GetAddressOf());

    // Render!

    context->DrawIndexed(info->IndexCount, 0, 0);
}


//------------------------------------------------------------------------------
// PluginManager

void PluginManager::Initialize(
    XrHeadsetProperties* headset,
    XrRenderProperties* rendering,
    MonitorRenderModel* model)
{
    Logger.Info("PluginManager: Initialize");

    Headset = headset;
    Rendering = rendering;
    RenderModel = model;

    CylinderRenderer = std::make_unique<PluginCylinderRenderer>();
    CylinderRenderer->InitializeD3D(rendering->DeviceContext);


    //--------------------------------------------------------------------------
    // Global shared memory file: Plugins

    if (!PluginsSharedFile.Open(kXrmPluginsMemoryLayoutBytes, XRM_PLUGINS_SHARED_MEMORY_NAME)) {
        Logger.Error("PluginsSharedFile.Open failed: ", WindowsErrorString(::GetLastError()));
        PluginsSharedMemory = nullptr;
        return;
    }

    // TBD: Enable MSAA?
    const int sample_count = 1;

    PluginsSharedMemory = reinterpret_cast<XrmPluginsMemoryLayout*>(PluginsSharedFile.GetFront());
    for (int i = 0; i < XRM_PLUGIN_COUNT; ++i) {
        PluginServers[i] = std::make_unique<PluginServer>();
        PluginServers[i]->Initialize(
            i,
            PluginsSharedMemory,
            headset,
            rendering,
            model,
            kDupeMipLevels,
            sample_count);
    }
}

void PluginManager::Shutdown()
{
    Logger.Info("PluginManager: Shutting down");

    for (int i = 0; i < XRM_PLUGIN_COUNT; ++i) {
        if (PluginServers[i]) {
            PluginServers[i]->Shutdown();
            PluginServers[i].reset();
        }
    }

    PluginsSharedFile.Close();

    CylinderRenderer.reset();
}

void PluginManager::Update()
{
    bool selected_one = false;

    for (int i = 0; i < XRM_PLUGIN_COUNT; ++i)
    {
        PluginServer* server = PluginServers[i].get();
        if (!server) {
            continue;
        }

        // If no updated occurred:
        server->UpdateRenderModel();

        // Check for timeouts
        server->CheckTimeout();

        // Update gaze
        bool gaze_active = false;
        if (RenderModel->GazeValid && !selected_one && server->VrRenderTexture)
        {
            if (server->HitTest(RenderModel->GazeX, RenderModel->GazeY))
            {
                selected_one = true;
                gaze_active = true;
            }
        }
        server->SetFocusAndGaze(RenderModel->GazeX, RenderModel->GazeY, gaze_active);
    }
}

void PluginManager::Render(const DirectX::SimpleMath::Matrix& view_projection_matrix)
{
    for (int i = 0; i < XRM_PLUGIN_COUNT; ++i)
    {
        PluginRenderInfo* info = &RenderModel->Plugins[i];

        if (!info->Texture) {
            continue;
        }

        info->EnableBlueLightFilter = (RenderModel->UiData.EnableBlueLightFilter != 0);

        CylinderRenderer->Render(Rendering->DeviceContext, view_projection_matrix, info);
    }
}

void PluginManager::SolveDesktopPositions()
{
    for (int i = 0; i < XRM_PLUGIN_COUNT; ++i)
    {
        PluginServer* server = PluginServers[i].get();
        if (!server) {
            continue;
        }

        server->UpdateMesh();
    }
}


} // namespace xrm
