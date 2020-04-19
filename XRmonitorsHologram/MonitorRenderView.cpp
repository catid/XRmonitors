// Copyright 2019 Augmented Perception Corporation

#include "stdafx.h"

#include "MonitorRenderView.hpp"

#include <list>

#include <wrl/client.h>

#include <DirectXColors.h>
#include <D3Dcompiler.h>

#include "XrUtility/XrMath.h"

#include "MonitorTools.hpp"
#include "D3D11Tools.hpp"

#include "core_win32.hpp"
#include "core_logger.hpp"

namespace xrm {

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace core;
using namespace DirectX::SimpleMath;

static logger::Channel ModuleLogger("MonitorRenderView");


//------------------------------------------------------------------------------
// PoseHistory

void PoseHistory::WritePose(const Quaternion& q, const Vector3& p)
{
    if (++NextWriteIndex >= kPoseCount) {
        NextWriteIndex = 0;
    }

    Poses[NextWriteIndex].TimeUsec = GetTimeUsec();
    Poses[NextWriteIndex].Orientation = q;
    Poses[NextWriteIndex].Position = p;
}

static inline uint64_t TimeDelta(uint64_t a, uint64_t b)
{
    if (a > b) {
        return a - b;
    }
    else {
        return b - a;
    }
}

bool PoseHistory::FindPose(uint64_t t, OldPose& pose_out)
{
    uint64_t best_dist = TimeDelta(Poses[0].TimeUsec, t);
    int best_i = 0;

    for (int i = 1; i < kPoseCount; ++i)
    {
        uint64_t dist = TimeDelta(Poses[i].TimeUsec, t);
        if (dist < best_dist) {
            best_i = i;
            best_dist = dist;
        }
    }

    //ModuleLogger.Info("best_dist = ", best_dist);

    // If there is not one found:
    if (best_dist >= 70000) {
        return false;
    }

    pose_out = Poses[best_i];
    return true;
}


//------------------------------------------------------------------------------
// Cube

// Vertices for a 1x1x1 meter cube. (Left/Right, Top/Bottom, Front/Back)
constexpr Vector3 LBB{ -0.5f, -0.5f, -0.5f };
constexpr Vector3 LBF{ -0.5f, -0.5f, 0.5f };
constexpr Vector3 LTB{ -0.5f, 0.5f, -0.5f };
constexpr Vector3 LTF{ -0.5f, 0.5f, 0.5f };
constexpr Vector3 RBB{ 0.5f, -0.5f, -0.5f };
constexpr Vector3 RBF{ 0.5f, -0.5f, 0.5f };
constexpr Vector3 RTB{ 0.5f, 0.5f, -0.5f };
constexpr Vector3 RTF{ 0.5f, 0.5f, 0.5f };

#define CUBE_SIDE(V1, V2, V3, V4, V5, V6, COLOR) \
    {V1, COLOR}, {V2, COLOR}, {V3, COLOR}, {V4, COLOR}, {V5, COLOR}, {V6, COLOR},

static const VertexPositionColor c_cubeVertices[] = {
    CUBE_SIDE(LTB, LBF, LBB, LTB, LTF, LBF, DarkRed)   // -X
    CUBE_SIDE(RTB, RBB, RBF, RTB, RBF, RTF, Red)       // +X
    CUBE_SIDE(LBB, LBF, RBF, LBB, RBF, RBB, DarkGreen) // -Y
    CUBE_SIDE(LTB, RTB, RTF, LTB, RTF, LTF, Green)     // +Y
    CUBE_SIDE(LBB, RBB, RTB, LBB, RTB, LTB, DarkBlue)  // -Z
    CUBE_SIDE(LBF, LTF, RTF, LBF, RTF, RBF, Blue)      // +Z
};

// Winding order is clockwise. Each side uses a different color.
constexpr unsigned short c_cubeIndices[] = {
    0,  1,  2,  3,  4,  5,  // -X
    6,  7,  8,  9,  10, 11, // +X
    12, 13, 14, 15, 16, 17, // -Y
    18, 19, 20, 21, 22, 23, // +Y
    24, 25, 26, 27, 28, 29, // -Z
    30, 31, 32, 33, 34, 35, // +Z
};

// Separate entrypoints for the vertex and pixel shader functions.
constexpr char kCubeShaderHlsl[] = R"_(
    struct PSVertex {
        float4 Pos : SV_POSITION;
        float3 Color : COLOR0;
    };
    struct Vertex {
        float3 Pos : POSITION;
        float3 Color : COLOR0;
    };
    cbuffer ModelConstantBuffer : register(b0) {
        float4x4 ModelViewProjection;
    };

    PSVertex MainVS(Vertex input) {
       PSVertex output;
       output.Pos = mul(float4(input.Pos, 1), ModelViewProjection);
       output.Color = input.Color;
       return output;
    }

    float4 MainPS(PSVertex input) : SV_TARGET {
        return float4(input.Color, 1);
    }
)_";


void Cube::SetPosition(XrSpace space)
{
    Space = space;
    Pose = xr::math::Pose::Translation({ 0, 0, 0 });
    Scale = { 0.1f, 0.1f, 0.1f };
}

void Cube::InitializeD3D(D3D11DeviceContext& device_context)
{
    const ComPtr<ID3DBlob> vertexShaderBytes = CompileShader(kCubeShaderHlsl, "MainVS", "vs_5_0");
    XR_CHECK(vertexShaderBytes != nullptr);
    XR_CHECK_HRCMD(device_context.Device->CreateVertexShader(
        vertexShaderBytes->GetBufferPointer(),
        vertexShaderBytes->GetBufferSize(),
        nullptr,
        VertexShader.ReleaseAndGetAddressOf()));

    const ComPtr<ID3DBlob> pixelShaderBytes = CompileShader(kCubeShaderHlsl, "MainPS", "ps_5_0");
    XR_CHECK(pixelShaderBytes != nullptr);
    XR_CHECK_HRCMD(device_context.Device->CreatePixelShader(
        pixelShaderBytes->GetBufferPointer(),
        pixelShaderBytes->GetBufferSize(),
        nullptr,
        PixelShader.ReleaseAndGetAddressOf()));

    const D3D11_INPUT_ELEMENT_DESC vertexDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    XR_CHECK_HRCMD(device_context.Device->CreateInputLayout(
        vertexDesc,
        (UINT)std::size(vertexDesc),
        vertexShaderBytes->GetBufferPointer(),
        vertexShaderBytes->GetBufferSize(),
        &InputLayout));

    const CD3D11_BUFFER_DESC mvpConstantBufferDesc(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
    XR_CHECK_HRCMD(device_context.Device->CreateBuffer(
        &mvpConstantBufferDesc,
        nullptr,
        MvpCBuffer.ReleaseAndGetAddressOf()));

    const D3D11_SUBRESOURCE_DATA vertexBufferData{ c_cubeVertices };
    const CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(c_cubeVertices), D3D11_BIND_VERTEX_BUFFER);
    XR_CHECK_HRCMD(device_context.Device->CreateBuffer(
        &vertexBufferDesc,
        &vertexBufferData,
        VertexBuffer.ReleaseAndGetAddressOf()));

    const D3D11_SUBRESOURCE_DATA indexBufferData{ c_cubeIndices };
    const CD3D11_BUFFER_DESC indexBufferDesc(sizeof(c_cubeIndices), D3D11_BIND_INDEX_BUFFER);
    XR_CHECK_HRCMD(device_context.Device->CreateBuffer(
        &indexBufferDesc,
        &indexBufferData,
        IndexBuffer.ReleaseAndGetAddressOf()));
}

void Cube::Render(
    ID3D11DeviceContext* context,
    const Matrix& view_projection_matrix)
{
    ID3D11Buffer* const constantBuffers[] = { MvpCBuffer.Get() };
    context->VSSetConstantBuffers(0, (UINT)std::size(constantBuffers), constantBuffers);
    context->VSSetShader(VertexShader.Get(), nullptr, 0);
    context->PSSetShader(PixelShader.Get(), nullptr, 0);

    // Set cube primitive data.
    const UINT strides[] = { sizeof(VertexPositionColor) };
    const UINT offsets[] = { 0 };
    ID3D11Buffer* vertexBuffers[] = { VertexBuffer.Get() };

    context->IASetVertexBuffers(0, (UINT)std::size(vertexBuffers), vertexBuffers, strides, offsets);
    context->IASetIndexBuffer(IndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(InputLayout.Get());

    // Compute and update the model transform.
    Matrix scalingMatrix = Matrix::CreateScale(Scale.x, Scale.y, Scale.z);
    Matrix modelMatrix = scalingMatrix * xr::math::LoadXrPose(Pose);
    Matrix mvpMatrix = modelMatrix * view_projection_matrix;

    ModelViewProjectionConstantBuffer mvp;
    mvp.ModelViewProjection = mvpMatrix.Transpose();

    context->UpdateSubresource(MvpCBuffer.Get(), 0, nullptr, &mvp, 0, 0);

    context->DrawIndexed((UINT)std::size(c_cubeIndices), 0, 0);
}


//------------------------------------------------------------------------------
// MonitorQuad

static const VertexPositionTexture kQuadVertices[] = {
    { XMFLOAT3(-0.5f, -0.5f, 0.f), XMFLOAT2(0.f, 1.f) }, // LL
    { XMFLOAT3( 0.5f, -0.5f, 0.f), XMFLOAT2(1.f, 1.f) }, // LR
    { XMFLOAT3(-0.5f,  0.5f, 0.f), XMFLOAT2(0.f, 0.f) }, // UL
    { XMFLOAT3( 0.5f,  0.5f, 0.f), XMFLOAT2(1.f, 0.f) }, // UR
};

// Winding order is clockwise
constexpr unsigned short kQuadIndices[] = {
    0, 2, 3,
    0, 3, 1
};

constexpr char kQuadShaderHlsl[] = R"_(
    struct Vertex {
        float3 Pos : POSITION;
        float2 Tex : TEXCOORD0;
    };

    struct PSVertex {
        float4 Pos : SV_POSITION;
        float2 Tex : TEXCOORD0;
    };

    cbuffer ModelConstantBuffer : register(b0) {
        float4x4 ModelViewProjection;
    };

    PSVertex MainVS(Vertex input) {
       PSVertex output;
       output.Pos = mul(float4(input.Pos, 1), ModelViewProjection);
       output.Tex = input.Tex;
       return output;
    }

    Texture2D shaderTexture;
    SamplerState SampleType;

    float4 MainPS(PSVertex input) : SV_TARGET {
        //return float4(1.0, 0.0, 0.0, 1.0);
        return shaderTexture.Sample(SampleType, input.Tex);
    }
)_";

void MonitorQuadRenderer::InitializeD3D(D3D11DeviceContext& device_context)
{
    const ComPtr<ID3DBlob> vertexShaderBytes = CompileShader(kQuadShaderHlsl, "MainVS", "vs_5_0");
    XR_CHECK(vertexShaderBytes != nullptr);
    XR_CHECK_HRCMD(device_context.Device->CreateVertexShader(
        vertexShaderBytes->GetBufferPointer(),
        vertexShaderBytes->GetBufferSize(),
        nullptr,
        VertexShader.ReleaseAndGetAddressOf()));

    const ComPtr<ID3DBlob> pixelShaderBytes = CompileShader(kQuadShaderHlsl, "MainPS", "ps_5_0");
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
        sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
    XR_CHECK_HRCMD(device_context.Device->CreateBuffer(
        &mvpConstantBufferDesc,
        nullptr,
        MvpCBuffer.ReleaseAndGetAddressOf()));

    const D3D11_SUBRESOURCE_DATA vertexBufferData{ kQuadVertices };
    const CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(kQuadVertices), D3D11_BIND_VERTEX_BUFFER);
    XR_CHECK_HRCMD(device_context.Device->CreateBuffer(
        &vertexBufferDesc,
        &vertexBufferData,
        VertexBuffer.ReleaseAndGetAddressOf()));

    const D3D11_SUBRESOURCE_DATA indexBufferData{ kQuadIndices };
    const CD3D11_BUFFER_DESC indexBufferDesc(sizeof(kQuadIndices), D3D11_BIND_INDEX_BUFFER);
    XR_CHECK_HRCMD(device_context.Device->CreateBuffer(
        &indexBufferDesc,
        &indexBufferData,
        IndexBuffer.ReleaseAndGetAddressOf()));

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
    rtblend.BlendEnable = TRUE; // TBD
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

void MonitorQuadRenderer::Render(
    D3D11DeviceContext& device_context,
    const Matrix& view_projection_matrix,
    const MonitorQuadRenderParams& params)
{
    ID3D11DeviceContext* context = device_context.Context.Get();

    ID3D11Buffer* const constantBuffers[] = { MvpCBuffer.Get() };
    context->VSSetConstantBuffers(0, (UINT)std::size(constantBuffers), constantBuffers);
    context->VSSetShader(VertexShader.Get(), nullptr, 0);
    context->PSSetShader(PixelShader.Get(), nullptr, 0);

    // Set cube primitive data:

    const UINT strides[] = { sizeof(VertexPositionTexture) };
    const UINT offsets[] = { 0 };
    ID3D11Buffer* vertexBuffers[] = { VertexBuffer.Get() };

    context->IASetVertexBuffers(0, (UINT)std::size(vertexBuffers), vertexBuffers, strides, offsets);
    context->IASetIndexBuffer(IndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(InputLayout.Get());

    // Compute and update the model transform:

    Matrix scalingMatrix = Matrix::CreateScale(params.Size.x, params.Size.y, 1.f);

    XrPosef pose;
    pose.orientation.x = params.Orientation.x;
    pose.orientation.y = params.Orientation.y;
    pose.orientation.z = params.Orientation.z;
    pose.orientation.w = params.Orientation.w;
    pose.position.x = params.Position.x;
    pose.position.y = params.Position.y;
    pose.position.z = params.Position.z;

    Matrix modelMatrix = scalingMatrix * xr::math::LoadXrPose(pose);

    Matrix mvpMatrix = modelMatrix * view_projection_matrix;

    ModelViewProjectionConstantBuffer mvp;
    mvp.ModelViewProjection = mvpMatrix.Transpose();

    context->UpdateSubresource(MvpCBuffer.Get(), 0, nullptr, &mvp, 0, 0);

    // Set texture

    if (!params.Texture)
    {
        // TBD: Render red?
        return;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};
    srv_desc.Format = params.TextureFormat;
    srv_desc.ViewDimension = params.MultisamplingEnabled ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MostDetailedMip = 0;
    srv_desc.Texture2D.MipLevels = -1;

    ComPtr<ID3D11ShaderResourceView> srv;
    XR_CHECK_HRCMD(device_context.Device->
        CreateShaderResourceView(
            params.Texture,
            &srv_desc,
            &srv));

#ifdef ENABLE_DUPE_MIP_LEVELS
    context->GenerateMips(srv.Get());
#endif

    context->PSSetSamplers(0, 1, SamplerState.GetAddressOf());
    context->PSSetShaderResources(0, 1, srv.GetAddressOf());

    // Render!

    context->DrawIndexed((UINT)std::size(kQuadIndices), 0, 0);
}


//------------------------------------------------------------------------------
// MonitorCylinderRenderer

struct CylinderColorConstantBuffer
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
        color.r *= ColorAdjust.x;
        color.g *= ColorAdjust.y;
        color.b *= ColorAdjust.z;
        return color;
    }
)_";

void MonitorCylinderRenderer::InitializeD3D(D3D11DeviceContext& device_context)
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
        sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
    XR_CHECK_HRCMD(device_context.Device->CreateBuffer(
        &mvpConstantBufferDesc,
        nullptr,
        MvpCBuffer.ReleaseAndGetAddressOf()));

    const CD3D11_BUFFER_DESC colorConstantBufferDesc(
        sizeof(CylinderColorConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
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

    // Alpha blending for bezels:

    D3D11_BLEND_DESC blend_desc{};
    blend_desc.AlphaToCoverageEnable = FALSE; // Needed to render multiple alphas properly
    blend_desc.IndependentBlendEnable = FALSE;
    auto& rtblend = blend_desc.RenderTarget[0];
    rtblend.BlendEnable = FALSE; // Enables alpha blending
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

void MonitorCylinderRenderer::Render(
    D3D11DeviceContext& device_context,
    const Matrix& view_projection_matrix,
    const MonitorCylinderParams& params)
{
    ID3D11DeviceContext* context = device_context.Context.Get();

    ID3D11Buffer* const vsConstantBuffers[] = { MvpCBuffer.Get() };
    context->VSSetConstantBuffers(0, (UINT)std::size(vsConstantBuffers), vsConstantBuffers);

    ID3D11Buffer* const psConstantBuffers[] = { ColorCBuffer.Get() };
    context->PSSetConstantBuffers(0, (UINT)std::size(psConstantBuffers), psConstantBuffers);

    context->VSSetShader(VertexShader.Get(), nullptr, 0);
    context->PSSetShader(PixelShader.Get(), nullptr, 0);

    // Set cube primitive data:

    const UINT strides[] = { sizeof(VertexPositionTexture) };
    const UINT offsets[] = { 0 };
    ID3D11Buffer* vertexBuffers[] = { params.VertexBuffer };

    context->IASetVertexBuffers(0, (UINT)std::size(vertexBuffers), vertexBuffers, strides, offsets);
    context->IASetIndexBuffer(params.IndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(InputLayout.Get());

    // We do not adjust the model at all at render time

    Matrix mvpMatrix = view_projection_matrix;

    ModelViewProjectionConstantBuffer mvp;
    mvp.ModelViewProjection = mvpMatrix.Transpose();

    context->UpdateSubresource(MvpCBuffer.Get(), 0, nullptr, &mvp, 0, 0);

    CylinderColorConstantBuffer color;
    color.ColorAdjustment.x = params.ColorScale;
    color.ColorAdjustment.y = params.ColorScale;
    color.ColorAdjustment.z = params.ColorScale;
    color.ColorAdjustment.w = params.ColorScale;
    if (params.EnableBlueLightFilter) {
        color.ColorAdjustment.z *= 0.5f;
    }

    context->UpdateSubresource(ColorCBuffer.Get(), 0, nullptr, &color, 0, 0);

    // Set texture

    if (!params.Texture)
    {
        // TBD: Render red?
        return;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};
    srv_desc.Format = params.TextureFormat;
    srv_desc.ViewDimension = params.MultisamplingEnabled ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MostDetailedMip = 0;
    srv_desc.Texture2D.MipLevels = -1;

    ComPtr<ID3D11ShaderResourceView> srv;
    XR_CHECK_HRCMD(device_context.Device->
        CreateShaderResourceView(
            params.Texture,
            &srv_desc,
            &srv));

#ifdef ENABLE_DUPE_MIP_LEVELS
    context->GenerateMips(srv.Get());
#endif

    context->PSSetSamplers(0, 1, SamplerState.GetAddressOf());
    context->PSSetShaderResources(0, 1, srv.GetAddressOf());

    //context->OMSetBlendState(this->BlendState.Get(), nullptr, 0xffffffff);

    // Render!

    context->DrawIndexed(params.IndexCount, 0, 0);

    //context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
}


//------------------------------------------------------------------------------
// MonitorRenderView

void MonitorRenderView::OnHeadsetRenderingInitialized(CameraImager* imager)
{
    Imager = imager;

    // Set up THE CUBE
    m_Cube.InitializeD3D(Rendering->DeviceContext);
    m_Cube.SetPosition(Rendering->SceneSpace.Get());

    // Set up the quad monitor renderer
    MonitorQuad.InitializeD3D(Rendering->DeviceContext);

    // Set up the cylindrical monitor renderer
    MonitorCylinder.InitializeD3D(Rendering->DeviceContext);

    // Set up pass-through camera rendering
    CameraRender.InitializeD3D(
        imager,
        CameraCalibration,
        Rendering->DeviceContext);
}

void MonitorRenderView::FrameRenderStart()
{
    // Update cubes location with latest space relation
    for (auto cube : { m_Cube }) {
        if (cube.Space != XR_NULL_HANDLE) {
            XrSpaceRelation spaceRelation{ XR_TYPE_SPACE_RELATION };
            XR_CHECK_XRCMD(xrLocateSpace(
                cube.Space,
                Rendering->SceneSpace.Get(),
                Rendering->PredictedDisplayTime,
                &spaceRelation));

            if (Rendering->ViewPosesValid) {
                cube.Pose = spaceRelation.pose;
            }
        }
    }
}

void MonitorRenderView::RenderProjectiveView(XrD3D11ProjectionView* view)
{
    ID3D11DeviceContext* context = Rendering->DeviceContext.Context.Get();

    CD3D11_VIEWPORT viewport(0.f, 0.f, (float)view->Swapchain.Width, (float)view->Swapchain.Height);
    context->RSSetViewports(1, &viewport);

    ComPtr<ID3D11RenderTargetView> rtv;

    if (view->MultiSamplingEnabled) {
        rtv = view->MultisampleRTV;
    }
    else
    {
        // Create RenderTargetView with original swapchain format (swapchain is typeless).
        const CD3D11_RENDER_TARGET_VIEW_DESC rtv_desc(
            D3D11_RTV_DIMENSION_TEXTURE2D,
            Rendering->SwapchainFormat);
        XR_CHECK_HRCMD(Rendering->DeviceContext.Device->CreateRenderTargetView(
            view->Swapchain.FrameTexture,
            &rtv_desc,
            &rtv));
    }

    // Clear swapchain and depth buffer.
    // NOTE: This will clear the entire render target view, not just the specified view.
    context->ClearRenderTargetView(
        rtv.Get(),
        Headset->EnvironmentBlendMode == XR_ENVIRONMENT_BLEND_MODE_OPAQUE
            ? DirectX::Colors::Black : DirectX::Colors::Transparent);

    context->ClearDepthStencilView(
        view->DepthStencil,
        D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
        1.0f,
        0);

    ID3D11RenderTargetView* renderTargets[] = { rtv.Get() };
    context->OMSetRenderTargets((UINT)std::size(renderTargets), renderTargets, view->DepthStencil);

    // Eye view jitter to reduce aliasing:

    Vector3 position = xr::math::LoadXrVector3(view->Pose.position);
    Quaternion orientation = xr::math::LoadXrQuaternion(view->Pose.orientation);
    orientation.Normalize();

    const float vq = 0.0002f;

    const uint32_t rv = wellons_triple32(NextJitterSeed += 1337);
    const float xr = vq * (uint32_t)(rv & 0xffff) / 65536.f;
    const float yr = vq * (uint32_t)(rv >> 16) / 65536.f;

    const float xm = fmodf(orientation.x, vq);
    const float ym = fmodf(orientation.y, vq);

    orientation.x = orientation.x - xm + xr;
    orientation.y = orientation.y - ym + yr;

    Quaternion invertOrientation = XMQuaternionConjugate(orientation);
    Vector3 invertPosition = XMVector3Rotate(-position, invertOrientation);

    const Matrix space_to_view = XMMatrixAffineTransformation(
        g_XMOne,           // scale
        g_XMZero,          // rotation origin
        invertOrientation, // rotation
        invertPosition);   // translation

    const Matrix projection_matrix = xr::math::ComposeProjectionMatrix(view->Fov, { 0.05f, 100.0f });
    const Matrix view_projection_matrix = space_to_view * projection_matrix;

    // Render all the objects in the scene:

    XR_CHECK(view->ViewIndex < 2);
    if (RenderModel->UiData.EnablePassthrough && view->ViewIndex < 2)
    {
        const uint32_t view_index = view->ViewIndex;

        Quaternion tOrientation;
        Vector3 tPosition;
        OpenXrToQuaternion(tOrientation, view->Pose.orientation);
        OpenXrToVector(tPosition, view->Pose.position);
        CameraPoseHistory[view_index].WritePose(tOrientation, tPosition);

        if (RenderModel->UpdatedCamerasThisFrame) {
            OldPose old_pose;
            if (CameraPoseHistory[view_index].FindPose(RenderModel->ExposureTimeUsec, old_pose))
            {
                RenderModel->CameraOrientation[view_index] = old_pose.Orientation;
                RenderModel->CameraPosition[view_index] = old_pose.Position;
            }
            else
            {
                Logger.Error("Failed to find camera frame exposure time in pose history!");
            }
            //OpenXrToQuaternion(RenderModel->CameraOrientation[view_index], view->Pose.orientation);
            //OpenXrToVector(RenderModel->CameraPosition[view_index], view->Pose.position);
        }

        CameraRenderParams camera_render_params;
        camera_render_params.CameraOrientation = RenderModel->CameraOrientation[view_index];
        camera_render_params.CameraPosition = RenderModel->CameraPosition[view_index];
        camera_render_params.EyeIndex = view_index;
        camera_render_params.MultisamplingEnabled = view->MultiSamplingEnabled;
        camera_render_params.TextureFormat = Rendering->SwapchainFormat;
        if (RenderModel->UiData.EnableBlueLightFilter != 0) {
            camera_render_params.Color = CameraColors::Sepia;
        }
        else {
            camera_render_params.Color = CameraColors::AllBusinessBlue;
        }

        CameraRender.Render(
            Rendering->DeviceContext,
            view_projection_matrix,
            camera_render_params);
    }

    //m_Cube.Render(context, view_projection_matrix);

#ifdef MONITOR_USE_POSE_FILTER
    XMMATRIX monitor_view_projection_matrix;
    if (view->ViewIndex >= 2) {
        monitor_view_projection_matrix = view_projection_matrix;
    }
    else {
        const XMMATRIX space_to_view = XMMatrixInverse(
            nullptr,
            xr::math::LoadXrPose(Rendering->PoseFilter.FilteredPoses[view->ViewIndex]));
        monitor_view_projection_matrix = space_to_view * projection_matrix;
    }
#endif

    const unsigned monitor_count = RenderModel->Monitors.size();

    for (unsigned i = 0; i < monitor_count; ++i)
    {
        auto& monitor = RenderModel->Monitors[i];

#if 0
        MonitorQuadRenderParams params;
        params.Texture = monitor->Dupe->VrRenderTexture.Get();
        params.Orientation = monitor->Orientation;
        params.Position = monitor->Position;
        params.Size = monitor->Size;
        params.MultisamplingEnabled = view->MultiSamplingEnabled;
        params.TextureFormat = Rendering->SwapchainFormat;

        MonitorQuad.Render(
            Rendering->DeviceContext,
            view_projection_matrix,
            params);
#else
        MonitorCylinderParams params;
        params.Texture = monitor->Dupe->VrRenderTexture.Get();
        params.MultisamplingEnabled = view->MultiSamplingEnabled;
        params.TextureFormat = Rendering->SwapchainFormat;
        params.VertexBuffer = monitor->VertexBuffer.Get();
        params.IndexBuffer = monitor->IndexBuffer.Get();
        params.IndexCount = monitor->IndexCount;
        if (Headset->NeedsReverbColorHack) {
            params.ColorScale = 0.75f;
        }
        if (RenderModel->UiData.EnableBlueLightFilter != 0) {
            params.EnableBlueLightFilter = true;
        }
        else {
            params.EnableBlueLightFilter = false;
        }

        MonitorCylinder.Render(
            Rendering->DeviceContext,
#ifdef MONITOR_USE_POSE_FILTER
            monitor_view_projection_matrix,
#else
            view_projection_matrix,
#endif
            params);
#endif

        // Release the render texture
        monitor->Dupe->ReleaseVrRenderTexture();
    }

    Plugins->Render(view_projection_matrix);
}

void MonitorRenderView::RenderMonitorToQuadLayer(XrD3D11QuadLayerSwapchain* quad, MonitorRenderState* monitor)
{
    Rendering->DeviceContext.Context->CopyResource(
        quad->Swapchain.FrameTexture,
        monitor->Dupe->VrRenderTexture.Get());

#if 0

    // This does not work
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};
    srv_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MostDetailedMip = 0;
    srv_desc.Texture2D.MipLevels = -1;

    ComPtr<ID3D11ShaderResourceView> srv;
    XR_CHECK_HRCMD(Rendering->DeviceContext.Device->
        CreateShaderResourceView(
            quad->Swapchain.FrameTexture,
            &srv_desc,
            &srv));

    bool key_down = (::GetAsyncKeyState(VK_LSHIFT) >> 15) != 0;

    if (key_down)
    {
        Rendering->DeviceContext.Context->GenerateMips(srv.Get());
    }
#endif
}


} // namespace xrm
