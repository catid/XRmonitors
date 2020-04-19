// Copyright 2019 Augmented Perception Corporation

#include "stdafx.h"

#include "CameraRenderer.hpp"

// Enable this to use the lower left keyboard keys to control the calibration
// settings in an interactive manner.
//#define ENABLE_CALIBRATION_KEYS

namespace xrm {

using namespace core;
using namespace DirectX::SimpleMath;

static logger::Channel Logger("CameraRenderer");


//------------------------------------------------------------------------------
// CameraRenderer : Shaders

struct CameraMVPConstantBuffer
{
    XMFLOAT4X4 ModelViewProjection;
};

struct CameraColorConstantBuffer
{
    XMFLOAT4 ColorAdjustment;
};

constexpr char kCameraVertexShaderHlsl[] = R"_(
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

       // Place it behind everything else
       output.Pos.z = 0.9999f * output.Pos.w;

       output.Tex = input.Tex;
       return output;
    }
)_";

constexpr char kCameraPixelShaderHlsl[] = R"_(
    struct PSVertex {
        float4 Pos : SV_POSITION;
        float2 Tex : TEXCOORD0;
    };

    cbuffer ColorConstantBuffer : register(b0) {
        float4 ColorAdjustment;
    };

    Texture2D shaderTexture;
    SamplerState SampleType;

    float4 MainPS(PSVertex input) : SV_TARGET {
        float4 color = shaderTexture.Sample(SampleType, input.Tex);
        return float4(
            color.r * ColorAdjustment.r,
            color.r * ColorAdjustment.g,
            color.r * ColorAdjustment.b,
            1.0);
    }
)_";

//------------------------------------------------------------------------------
// CameraRenderer : API

void CameraRenderer::InitializeD3D(
    CameraImager* imager,
    HeadsetCameraCalibration* camera_calibration,
    D3D11DeviceContext& device_context)
{
    Imager = imager;
    CameraCalibration = camera_calibration;

    const ComPtr<ID3DBlob> vertexShaderBytes = CompileShader(kCameraVertexShaderHlsl, "MainVS", "vs_5_0");
    XR_CHECK(vertexShaderBytes != nullptr);
    XR_CHECK_HRCMD(device_context.Device->CreateVertexShader(
        vertexShaderBytes->GetBufferPointer(),
        vertexShaderBytes->GetBufferSize(),
        nullptr,
        VertexShader.ReleaseAndGetAddressOf()));

    const ComPtr<ID3DBlob> pixelShaderBytes = CompileShader(kCameraPixelShaderHlsl, "MainPS", "ps_5_0");
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
        sizeof(CameraMVPConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
    XR_CHECK_HRCMD(device_context.Device->CreateBuffer(
        &mvpConstantBufferDesc,
        nullptr,
        MvpCBuffer.ReleaseAndGetAddressOf()));

    const CD3D11_BUFFER_DESC colorConstantBufferDesc(
        sizeof(CameraColorConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
    XR_CHECK_HRCMD(device_context.Device->CreateBuffer(
        &colorConstantBufferDesc,
        nullptr,
        ColorCBuffer.ReleaseAndGetAddressOf()));

    // Create a texture sampler state description.
    D3D11_SAMPLER_DESC sampler_desc{};
    sampler_desc.Filter = D3D11_FILTER_ANISOTROPIC;
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.MipLODBias = 0.0f;
    sampler_desc.MaxAnisotropy = D3D11_DEFAULT_MAX_ANISOTROPY;
    sampler_desc.ComparisonFunc = D3D11_COMPARISON_LESS;
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

    GenerateMesh(CameraCalibration->K1, CameraCalibration->K2);

    for (unsigned i = 0; i < kEyeViewCount; ++i)
    {
        D3D11_SUBRESOURCE_DATA vb_data{};
        vb_data.pSysMem = Vertices[i].data();
        vb_data.SysMemPitch = 0;
        vb_data.SysMemSlicePitch = 0;
        const CD3D11_BUFFER_DESC vb_desc(
            sizeof(VertexPositionTexture) * static_cast<unsigned>(Vertices[i].size()),
            D3D11_BIND_VERTEX_BUFFER);

        XR_CHECK_HRCMD(device_context.Device->CreateBuffer(
            &vb_desc,
            &vb_data,
            &VertexBuffer[i]));
    }

    D3D11_SUBRESOURCE_DATA ib_data{};
    ib_data.pSysMem = Indices.data();
    ib_data.SysMemPitch = 0;
    ib_data.SysMemSlicePitch = 0;
    CD3D11_BUFFER_DESC ib_desc(
        sizeof(uint16_t) * static_cast<unsigned>(Indices.size()), D3D11_BIND_INDEX_BUFFER);

    XR_CHECK_HRCMD(device_context.Device->CreateBuffer(
        &ib_desc,
        &ib_data,
        &IndexBuffer));

    IndexCount = static_cast<unsigned>(Indices.size());
}

void CameraRenderer::Render(
    D3D11DeviceContext& device_context,
    const Matrix& view_projection_matrix,
    const CameraRenderParams& params)
{
    if (params.EyeIndex >= kEyeViewCount) {
        CORE_DEBUG_BREAK(); // Should never happen
        return;
    }

    if (!Imager->HasCameraImage)
    {
        // TBD: Render red?
        return;
    }

    const Matrix modelScale = Matrix::CreateScale(CameraCalibration->Scale);

#ifdef ENABLE_CALIBRATION_KEYS
    bool modified = false;
    static float offset_x = CameraCalibration->OffsetX;
    static float offset_y = CameraCalibration->OffsetY;
    static float right_offset_y = CameraCalibration->RightOffsetY;
    // (2) Then adjust the translation offsets so that eyes can fuse the top of real monitors at eye-level
    if ((::GetAsyncKeyState('F') >> 15) != 0) {
        offset_y -= 0.001f;
        modified = true;
    }
    if ((::GetAsyncKeyState('V') >> 15) != 0) {
        offset_y += 0.001f;
        modified = true;
    }
    if ((::GetAsyncKeyState('G') >> 15) != 0) {
        offset_x -= 0.001f;
        modified = true;
    }
    if ((::GetAsyncKeyState('B') >> 15) != 0) {
        offset_x += 0.001f;
        modified = true;
    }
    if ((::GetAsyncKeyState('H') >> 15) != 0) {
        right_offset_y -= 0.0001f;
        modified = true;
    }
    if ((::GetAsyncKeyState('N') >> 15) != 0) {
        right_offset_y += 0.0001f;
        modified = true;
    }
#else
    const float offset_x = CameraCalibration->OffsetX;
    const float offset_y = CameraCalibration->OffsetY;
    const float right_offset_y = CameraCalibration->RightOffsetY;
#endif

    Vector3 translate;
    if (params.EyeIndex == 0) {
        translate.x = -offset_x;
        translate.y = offset_y - right_offset_y;
    }
    else {
        translate.x = offset_x;
        translate.y = offset_y + right_offset_y;
    }
    translate.z = 0.f;

    const Matrix translateMatrix = Matrix::CreateTranslation(translate);

#ifdef ENABLE_CALIBRATION_KEYS
    static float eye_cant_x = CameraCalibration->EyeCantX;
    static float eye_cant_y = CameraCalibration->EyeCantY;
    static float eye_cant_z = CameraCalibration->EyeCantZ;

    // (3) Then adjust the cant angles so that vertical bars in the room fuse at top and bottom
    if ((::GetAsyncKeyState('A') >> 15) != 0) {
        eye_cant_x -= 0.001f;
        modified = true;
    }
    if ((::GetAsyncKeyState('Z') >> 15) != 0) {
        eye_cant_x += 0.001f;
        modified = true;
    }
    if ((::GetAsyncKeyState('S') >> 15) != 0) {
        eye_cant_y -= 0.001f;
        modified = true;
    }
    if ((::GetAsyncKeyState('X') >> 15) != 0) {
        eye_cant_y += 0.001f;
        modified = true;
    }
    if ((::GetAsyncKeyState('D') >> 15) != 0) {
        eye_cant_z -= 0.001f;
        modified = true;
    }
    if ((::GetAsyncKeyState('C') >> 15) != 0) {
        eye_cant_z += 0.001f;
        modified = true;
    }

    if (modified) {
        Logger.Info("Modified camera settings:\n",
            "OffsetX=", offset_x, "\n",
            "OffsetY=", offset_y, "\n",
            "EyeCantX=", eye_cant_x, "\n",
            "EyeCantY=", eye_cant_y, "\n",
            "EyeCantZ=", eye_cant_z, "\n");
    }

#else
    const float eye_cant_x = CameraCalibration->EyeCantX;
    const float eye_cant_y = CameraCalibration->EyeCantY;
    const float eye_cant_z = CameraCalibration->EyeCantZ;
#endif

    Matrix rotateMatrix;
    if (params.EyeIndex == 0) {
        rotateMatrix = Matrix::CreateFromYawPitchRoll(-eye_cant_y, eye_cant_x, -eye_cant_z);
    }
    else {
        rotateMatrix = Matrix::CreateFromYawPitchRoll(eye_cant_y, eye_cant_x, eye_cant_z);
    }

    const Matrix modelOrientation = Matrix::CreateFromQuaternion(params.CameraOrientation);
    const Matrix modelTranslation = Matrix::CreateTranslation(params.CameraPosition);
    const Matrix distTranslation = Matrix::CreateTranslation(Vector3(0.f, 0.f, -1.f));

    const Matrix transform = rotateMatrix * translateMatrix * modelScale * distTranslation * modelOrientation * modelTranslation;

    //const Matrix transform = Matrix::CreateTranslation(Vector3(0.f, 0.f, -1.f));

    // Render:

    ID3D11DeviceContext* context = device_context.Context.Get();

    CameraMVPConstantBuffer mvp;
    XMStoreFloat4x4(&mvp.ModelViewProjection, XMMatrixTranspose(transform * view_projection_matrix));

    context->UpdateSubresource(MvpCBuffer.Get(), 0, nullptr, &mvp, 0, 0);

    CameraColorConstantBuffer color{};
    if (params.Color == CameraColors::AllBusinessBlue)
    {
        color.ColorAdjustment.x = 0.f;
        color.ColorAdjustment.y = 161.f / 255.f;
        color.ColorAdjustment.z = 241.f / 255.f;
    }
    else if (params.Color == CameraColors::Sepia)
    {
        color.ColorAdjustment.x = 112.f / 255.f;
        color.ColorAdjustment.y = 66.f / 255.f;
        color.ColorAdjustment.z = 20.f / 255.f;
    }
    else if (params.Color == CameraColors::Grey)
    {
        color.ColorAdjustment.x = 0.75f;
        color.ColorAdjustment.y = 0.75f;
        color.ColorAdjustment.z = 0.75f;
    }
    else // White is default:
    {
        color.ColorAdjustment.x = 1.f;
        color.ColorAdjustment.y = 1.f;
        color.ColorAdjustment.z = 1.f;
    }
    color.ColorAdjustment.w = 1.f;

    context->UpdateSubresource(ColorCBuffer.Get(), 0, nullptr, &color, 0, 0);

    ID3D11Buffer* const vsConstantBuffers[] = { MvpCBuffer.Get() };

    context->VSSetConstantBuffers(0, (UINT)std::size(vsConstantBuffers), vsConstantBuffers);
    context->VSSetShader(VertexShader.Get(), nullptr, 0);

    ID3D11Buffer* const psConstantBuffers[] = { ColorCBuffer.Get() };

    context->PSSetConstantBuffers(0, (UINT)std::size(psConstantBuffers), psConstantBuffers);
    context->PSSetShader(PixelShader.Get(), nullptr, 0);

    // Set cube primitive data:

    const UINT strides[] = { sizeof(VertexPositionTexture) };
    const UINT offsets[] = { 0 };
    ID3D11Buffer* vertexBuffers[] = { VertexBuffer[params.EyeIndex].Get() };

    context->IASetVertexBuffers(0, (UINT)std::size(vertexBuffers), vertexBuffers, strides, offsets);
    context->IASetIndexBuffer(IndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(InputLayout.Get());

    // Set texture:

    ComPtr<ID3D11ShaderResourceView> srv;
    XR_CHECK_HRCMD(device_context.Device->
        CreateShaderResourceView(
            Imager->RenderTexture.Get(),
            nullptr,
            &srv));

#ifdef ENABLE_DUPE_MIP_LEVELS
    //context->GenerateMips(srv.Get());
#endif

    context->PSSetSamplers(0, 1, SamplerState.GetAddressOf());
    context->PSSetShaderResources(0, 1, srv.GetAddressOf());

    // Render!

    context->DrawIndexed(IndexCount, 0, 0);
}


//------------------------------------------------------------------------------
// CameraRenderer : Private

void CameraRenderer::GenerateMesh(float k1, float k2)
{
    for (unsigned i = 0; i < kEyeViewCount; ++i) {
        Vertices[i].clear();
    }
    Indices.clear();

    // TBD: Support multiple resolutions
    const float aspect = 640.f / 480.f;

    // FIXME: This mesh is way too high rez
    const unsigned width = 20;
    const unsigned pitch = width + 1;
    const unsigned height = 20;
    const float dx = 1.f / width;
    const float dy = 1.f / height;

    unsigned lr_index = 0;

    const float inv_height = 1.f / height;
    const float inv_width = 1.f / width;

    for (unsigned y = 0; y <= height; ++y)
    {
        float yf = y * inv_height;
        float v_y = yf - 0.5f;
        float v = 1.f - yf;

        for (unsigned x = 0; x <= width; ++x)
        {
            float xf = x * inv_width;
            float v_x = xf - 0.5f;
            float u = xf;

            float s, t;
            WarpVertex(
                k1,
                k2,
                v_x * aspect,
                v_y,
                s,
                t);

            VertexPositionTexture vertex;
            vertex.position.x = s;
            vertex.position.y = t;
            vertex.position.z = 0.f;

            const float border = 0.005f;

            vertex.textureCoordinate.x = u * (0.5f - border);
            vertex.textureCoordinate.y = v;

            Vertices[0].push_back(vertex);

            vertex.textureCoordinate.x = (0.5f + border) + u * (0.5f - border);
            vertex.textureCoordinate.y = v;

            Vertices[1].push_back(vertex);

            if (x > 0 && y > 0) {
                Indices.push_back(lr_index - pitch);
                Indices.push_back(lr_index - pitch - 1);
                Indices.push_back(lr_index - 1);

                Indices.push_back(lr_index);
                Indices.push_back(lr_index - pitch);
                Indices.push_back(lr_index - 1);
            }

            ++lr_index;
        }
    }
}

void CameraRenderer::WarpVertex(float k1, float k2, float u, float v, float& s, float& t)
{
    // We assume that 0 is already the image center
    const float r_sqr = u * u + v * v;
    const float r_sqr2 = r_sqr * r_sqr;
    const float k_inv = 1.f / (1.f + k1 * r_sqr + k2 * r_sqr2);

    s = u * k_inv;
    t = v * k_inv;
}


} // namespace xrm
