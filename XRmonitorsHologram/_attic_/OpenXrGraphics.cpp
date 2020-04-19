// Copyright 2019 Augmented Perception Corporation

#include "stdafx.h"
#include "OpenXrD3D11Graphics.hpp"

#include <list>

#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

#include <DirectXColors.h>
#include <D3Dcompiler.h>
using namespace DirectX;

#include "XrUtility/XrMath.h"

#include "MonitorTools.hpp"
#include "D3D11Tools.hpp"

#include "core_win32.hpp"
#include "core_logger.hpp"
using namespace core;

namespace xrm {

static logger::Channel Logger("OpenXrD3D11Graphics");


//------------------------------------------------------------------------------
// Cube

struct Vertex
{
    XrVector3f Position;
    XrVector3f Color;
};

// RGB
constexpr XrVector3f Red{ 1, 0, 0 };
constexpr XrVector3f DarkRed{ 0.25f, 0, 0 };
constexpr XrVector3f Green{ 0, 1, 0 };
constexpr XrVector3f DarkGreen{ 0, 0.25f, 0 };
constexpr XrVector3f Blue{ 0, 0, 1 };
constexpr XrVector3f DarkBlue{ 0, 0, 0.25f };

// Vertices for a 1x1x1 meter cube. (Left/Right, Top/Bottom, Front/Back)
constexpr XrVector3f LBB{ -0.5f, -0.5f, -0.5f };
constexpr XrVector3f LBF{ -0.5f, -0.5f, 0.5f };
constexpr XrVector3f LTB{ -0.5f, 0.5f, -0.5f };
constexpr XrVector3f LTF{ -0.5f, 0.5f, 0.5f };
constexpr XrVector3f RBB{ 0.5f, -0.5f, -0.5f };
constexpr XrVector3f RBF{ 0.5f, -0.5f, 0.5f };
constexpr XrVector3f RTB{ 0.5f, 0.5f, -0.5f };
constexpr XrVector3f RTF{ 0.5f, 0.5f, 0.5f };

#define CUBE_SIDE(V1, V2, V3, V4, V5, V6, COLOR) {V1, COLOR}, {V2, COLOR}, {V3, COLOR}, {V4, COLOR}, {V5, COLOR}, {V6, COLOR},

constexpr Vertex c_cubeVertices[] = {
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

struct ModelViewProjectionConstantBuffer
{
    XMFLOAT4X4 ModelViewProjection;
};

// Separate entrypoints for the vertex and pixel shader functions.
constexpr char ShaderHlsl[] = R"_(
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
    Pose = xr::math::Pose::Translation({ 0, 0, -1 });
    Scale = { 0.1f, 0.1f, 0.1f };
}

void Cube::InitializeD3D(D3D11DeviceContext& device_context)
{
    const ComPtr<ID3DBlob> vertexShaderBytes = CompileShader(ShaderHlsl, "MainVS", "vs_5_0");
    CHECK_HRCMD(device_context.Device->CreateVertexShader(
        vertexShaderBytes->GetBufferPointer(),
        vertexShaderBytes->GetBufferSize(),
        nullptr,
        VertexShader.ReleaseAndGetAddressOf()));

    const ComPtr<ID3DBlob> pixelShaderBytes = CompileShader(ShaderHlsl, "MainPS", "ps_5_0");
    CHECK_HRCMD(device_context.Device->CreatePixelShader(
        pixelShaderBytes->GetBufferPointer(),
        pixelShaderBytes->GetBufferSize(),
        nullptr,
        PixelShader.ReleaseAndGetAddressOf()));

    const D3D11_INPUT_ELEMENT_DESC vertexDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    CHECK_HRCMD(device_context.Device->CreateInputLayout(
        vertexDesc,
        (UINT)std::size(vertexDesc),
        vertexShaderBytes->GetBufferPointer(),
        vertexShaderBytes->GetBufferSize(),
        &InputLayout));

    const CD3D11_BUFFER_DESC mvpConstantBufferDesc(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
    CHECK_HRCMD(device_context.Device->CreateBuffer(
        &mvpConstantBufferDesc,
        nullptr,
        MvpCBuffer.ReleaseAndGetAddressOf()));

    const D3D11_SUBRESOURCE_DATA vertexBufferData{ c_cubeVertices };
    const CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(c_cubeVertices), D3D11_BIND_VERTEX_BUFFER);
    CHECK_HRCMD(device_context.Device->CreateBuffer(
        &vertexBufferDesc,
        &vertexBufferData,
        VertexBuffer.ReleaseAndGetAddressOf()));

    const D3D11_SUBRESOURCE_DATA indexBufferData{ c_cubeIndices };
    const CD3D11_BUFFER_DESC indexBufferDesc(sizeof(c_cubeIndices), D3D11_BIND_INDEX_BUFFER);
    CHECK_HRCMD(device_context.Device->CreateBuffer(
        &indexBufferDesc,
        &indexBufferData,
        IndexBuffer.ReleaseAndGetAddressOf()));
}


//------------------------------------------------------------------------------
// OpenXrD3D11Graphics

void OpenXrD3D11Graphics::InitializeDevice(XrInstance instance, XrSystemId systemId)
{
    CHECK(instance != XR_NULL_HANDLE);
    CHECK(systemId != XR_NULL_SYSTEM_ID);

    // Create the D3D11 device for the adapter associated with the system.
    XrGraphicsRequirementsD3D11KHR graphicsRequirements{XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR};
    CHECK_XRCMD(xrGetD3D11GraphicsRequirementsKHR(instance, systemId, &graphicsRequirements));

    const ComPtr<IDXGIAdapter1> adapter = GetAdapter(graphicsRequirements.adapterLuid);
    const std::vector<D3D_FEATURE_LEVEL> featureLevels = SelectFeatureLevels(graphicsRequirements);

    InitializeD3D11DeviceForAdapter(
        adapter.Get(),
        featureLevels,
        DeviceContext);

    m_xrGraphicsBindingD3D11.device = DeviceContext.Device.Get();
}

void OpenXrD3D11Graphics::OnSessionInit(XrRenderState* state)
{
    State = state;

    // Set up THE CUBE
    m_Cube.InitializeD3D(DeviceContext);
    m_Cube.SetPosition(State->m_sceneSpace.Get());
}

const XrGraphicsBindingD3D11KHR* OpenXrD3D11Graphics::GetGraphicsBinding() const
{
    return &m_xrGraphicsBindingD3D11;
}

int64_t OpenXrD3D11Graphics::SelectColorSwapchainFormat(const std::vector<int64_t>& runtimeFormats) const
{
    // List of supported color swapchain formats, in priority order.
    constexpr DXGI_FORMAT SupportedColorSwapchainFormats[] = {
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_FORMAT_B8G8R8A8_UNORM,
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
        DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
    };

    auto swapchainFormatIt = std::find_first_of(std::begin(SupportedColorSwapchainFormats),
                                                std::end(SupportedColorSwapchainFormats),
                                                runtimeFormats.begin(),
                                                runtimeFormats.end());
    if (swapchainFormatIt == std::end(SupportedColorSwapchainFormats)) {
        THROW("No runtime swapchain format supported for color swapchain");
    }

    return *swapchainFormatIt;
}

std::vector<XrSwapchainImageBaseHeader*>
OpenXrD3D11Graphics::AllocateSwapchainImageStructs(uint32_t capacity, const XrSwapchainCreateInfo& /*swapchainCreateInfo*/)
{
    // Allocate and initialize the buffer of image structs (must be sequential in memory for xrEnumerateSwapchainImages).
    // Return back an array of pointers to each swapchain image struct so the consumer doesn't need to know the type/size.
    std::vector<XrSwapchainImageD3D11KHR> swapchainImageBuffer(capacity);
    std::vector<XrSwapchainImageBaseHeader*> swapchainImageBase;
    for (XrSwapchainImageD3D11KHR& image : swapchainImageBuffer) {
        image.type = XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR;
        swapchainImageBase.push_back(reinterpret_cast<XrSwapchainImageBaseHeader*>(&image));
    }

    // Keep the buffer alive by moving it into the list of buffers.
    m_swapchainImageBuffers.push_back(std::move(swapchainImageBuffer));

    return swapchainImageBase;
}

ComPtr<ID3D11DepthStencilView> OpenXrD3D11Graphics::GetDepthStencilView(ID3D11Texture2D* colorTexture)
{
    // If a depth-stencil view has already been created for this back-buffer, use it.
    auto depthBufferIt = m_colorToDepthMap.find(colorTexture);
    if (depthBufferIt != m_colorToDepthMap.end()) {
        return depthBufferIt->second;
    }

    // This back-buffer has no cooresponding depth-stencil texture, so create one with matching dimensions.
    D3D11_TEXTURE2D_DESC colorDesc;
    colorTexture->GetDesc(&colorDesc);

    D3D11_TEXTURE2D_DESC depthDesc{};
    depthDesc.Width = colorDesc.Width;
    depthDesc.Height = colorDesc.Height;
    depthDesc.ArraySize = colorDesc.ArraySize;
    depthDesc.MipLevels = 1;
    depthDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    depthDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
    depthDesc.SampleDesc.Count = State->ActiveMsaa;
    ComPtr<ID3D11Texture2D> depthTexture;
    CHECK_HRCMD(DeviceContext.Device->CreateTexture2D(&depthDesc, nullptr, depthTexture.ReleaseAndGetAddressOf()));

    // Create and cache the depth stencil view.
    ComPtr<ID3D11DepthStencilView> depthStencilView;
    CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(
        State->ActiveMsaa == 1 ? D3D11_DSV_DIMENSION_TEXTURE2D : D3D11_DSV_DIMENSION_TEXTURE2DMS,
        DXGI_FORMAT_D32_FLOAT);
    CHECK_HRCMD(DeviceContext.Device->CreateDepthStencilView(depthTexture.Get(), &depthStencilViewDesc, depthStencilView.GetAddressOf()));
    depthBufferIt = m_colorToDepthMap.insert(std::make_pair(colorTexture, depthStencilView)).first;

    return depthStencilView;
}

void OpenXrD3D11Graphics::RenderView(
    const XrCompositionLayerProjectionView& layerView,
    const XrSwapchainImageBaseHeader* swapchainImage,
    const XrEnvironmentBlendMode environmentBlendMode,
    int64_t colorSwapchainFormat)
{
    CHECK(layerView.subImage.imageArrayIndex == 0); // Texture arrays not supported.

    ID3D11Texture2D* const colorTexture = reinterpret_cast<const XrSwapchainImageD3D11KHR*>(swapchainImage)->texture;

    CD3D11_VIEWPORT viewport((float)layerView.subImage.imageRect.offset.x,
                                (float)layerView.subImage.imageRect.offset.y,
                                (float)layerView.subImage.imageRect.extent.width,
                                (float)layerView.subImage.imageRect.extent.height);
    DeviceContext.Context->RSSetViewports(1, &viewport);

    // Create RenderTargetView with original swapchain format (swapchain is typeless).
    ComPtr<ID3D11RenderTargetView> renderTargetView;
    const CD3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc(
        State->ActiveMsaa == 1 ? D3D11_RTV_DIMENSION_TEXTURE2D : D3D11_RTV_DIMENSION_TEXTURE2DMS,
        (DXGI_FORMAT)colorSwapchainFormat);
    CHECK_HRCMD(DeviceContext.Device->CreateRenderTargetView(
        colorTexture,
        &renderTargetViewDesc,
        renderTargetView.ReleaseAndGetAddressOf()));

    const ComPtr<ID3D11DepthStencilView> depthStencilView = GetDepthStencilView(colorTexture);

    // Clear swapchain and depth buffer. NOTE: This will clear the entire render target view, not just the specified view.
    DeviceContext.Context->ClearRenderTargetView(
        renderTargetView.Get(),
        environmentBlendMode == XR_ENVIRONMENT_BLEND_MODE_OPAQUE
            ? DirectX::Colors::Black : DirectX::Colors::Transparent);
    DeviceContext.Context->ClearDepthStencilView(
        depthStencilView.Get(),
        D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
        1.0f,
        0);

    ID3D11RenderTargetView* renderTargets[] = {renderTargetView.Get()};
    DeviceContext.Context->OMSetRenderTargets((UINT)std::size(renderTargets), renderTargets, depthStencilView.Get());

    const XMMATRIX spaceToView = XMMatrixInverse(nullptr, xr::math::LoadXrPose(layerView.pose));
    XMMATRIX projectionMatrix = xr::math::ComposeProjectionMatrix(layerView.fov, {0.05f, 100.0f});
    XMMATRIX viewProjectionMatrix = spaceToView * projectionMatrix;

    ID3D11Buffer* const constantBuffers[] = {m_Cube.MvpCBuffer.Get()};
    DeviceContext.Context->VSSetConstantBuffers(0, (UINT)std::size(constantBuffers), constantBuffers);
    DeviceContext.Context->VSSetShader(m_Cube.VertexShader.Get(), nullptr, 0);
    DeviceContext.Context->PSSetShader(m_Cube.PixelShader.Get(), nullptr, 0);

    // Set cube primitive data.
    const UINT strides[] = {sizeof(Vertex)};
    const UINT offsets[] = {0};
    ID3D11Buffer* vertexBuffers[] = { m_Cube.VertexBuffer.Get()};
    DeviceContext.Context->IASetVertexBuffers(0, (UINT)std::size(vertexBuffers), vertexBuffers, strides, offsets);
    DeviceContext.Context->IASetIndexBuffer(m_Cube.IndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
    DeviceContext.Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    DeviceContext.Context->IASetInputLayout(m_Cube.InputLayout.Get());

    // Render each cube
    for (const Cube& cube : {m_Cube}) {
        // Compute and update the model transform.
        XMMATRIX scalingMatrix = XMMatrixScaling(cube.Scale.x, cube.Scale.y, cube.Scale.z);
        XMMATRIX modelMatrix = scalingMatrix * xr::math::LoadXrPose(cube.Pose);
        XMMATRIX mvpMatrix = modelMatrix * viewProjectionMatrix;

        ModelViewProjectionConstantBuffer mvp;
        XMStoreFloat4x4(&mvp.ModelViewProjection, XMMatrixTranspose(mvpMatrix));

        DeviceContext.Context->UpdateSubresource(cube.MvpCBuffer.Get(), 0, nullptr, &mvp, 0, 0);

        // Draw the cube.
        DeviceContext.Context->DrawIndexed((UINT)std::size(c_cubeIndices), 0, 0);
    }
}

void OpenXrD3D11Graphics::RenderLayers(
    XrTime predictedDisplayTime,
    std::vector<XrCompositionLayerBaseHeader*>& layers)
{
    memset(&BaseLayer, 0, sizeof(BaseLayer));
    BaseLayer.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION;
    if (RenderLayer(predictedDisplayTime, BaseLayer)) {
        layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&BaseLayer));
    }
}

bool OpenXrD3D11Graphics::RenderLayer(
    XrTime predictedDisplayTime,
    XrCompositionLayerProjection& layer)
{
    XrViewState viewState{XR_TYPE_VIEW_STATE};
    uint32_t viewCapacityInput = (uint32_t)State->m_views.size();
    uint32_t viewCountOutput;

    XrViewLocateInfo viewLocateInfo{XR_TYPE_VIEW_LOCATE_INFO};
    viewLocateInfo.displayTime = predictedDisplayTime;
    viewLocateInfo.space = State->m_sceneSpace.Get();
    CHECK_XRCMD(xrLocateViews(
        State->m_session.Get(),
        &viewLocateInfo,
        &viewState,
        viewCapacityInput,
        &viewCountOutput,
        State->m_views.data()));
    CHECK(viewCountOutput > 0);
    CHECK(viewCountOutput == viewCapacityInput);
    CHECK(viewCountOutput == State->m_configViews.size());
    CHECK(viewCountOutput == State->m_swapchains.size());

    constexpr XrViewStateFlags viewPoseValidFlags = XR_VIEW_STATE_POSITION_VALID_BIT | XR_VIEW_STATE_ORIENTATION_VALID_BIT;
    if ((viewState.viewStateFlags & viewPoseValidFlags) != viewPoseValidFlags) {
        Logger.Warning("xrLocateViews returned an invalid pose.");
        return false;
    }

    // Update cubes location with latest space relation
    for (auto cube : {m_Cube}) {
        if (cube.Space != XR_NULL_HANDLE) {
            XrSpaceRelation spaceRelation{XR_TYPE_SPACE_RELATION};
            CHECK_XRCMD(xrLocateSpace(
                cube.Space,
                State->m_sceneSpace.Get(),
                predictedDisplayTime,
                &spaceRelation));

            constexpr XrViewStateFlags poseValidFlags =
                XR_SPACE_RELATION_POSITION_VALID_BIT | XR_SPACE_RELATION_ORIENTATION_VALID_BIT;
            if ((spaceRelation.relationFlags & poseValidFlags) == poseValidFlags) {
                cube.Pose = spaceRelation.pose;
            }
        }
    }

    ProjectionLayerViews.resize(viewCountOutput);

    // Render view to the appropriate part of the swapchain image.
    for (uint32_t i = 0; i < viewCountOutput; i++) {
        // Each view has a separate swapchain which is acquired, rendered to, and released.
        const Swapchain viewSwapchain = State->m_swapchains[i];

        uint32_t swapchainImageIndex;
        XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
        CHECK_XRCMD(xrAcquireSwapchainImage(viewSwapchain.handle, &acquireInfo, &swapchainImageIndex));
        ScopedFunction swapchain_release([&]() {
            XrSwapchainImageReleaseInfo releaseInfo{ XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
            CHECK_XRCMD(xrReleaseSwapchainImage(viewSwapchain.handle, &releaseInfo));
        });

        XrSwapchainImageWaitInfo waitInfo{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
        waitInfo.timeout = XR_INFINITE_DURATION;
        CHECK_XRCMD(xrWaitSwapchainImage(viewSwapchain.handle, &waitInfo));

        ProjectionLayerViews[i] = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
        ProjectionLayerViews[i].pose = State->m_views[i].pose;
        ProjectionLayerViews[i].fov = State->m_views[i].fov;
        ProjectionLayerViews[i].subImage.swapchain = viewSwapchain.handle;
        ProjectionLayerViews[i].subImage.imageRect.offset = {0, 0};
        ProjectionLayerViews[i].subImage.imageRect.extent = {viewSwapchain.width, viewSwapchain.height};

        const XrSwapchainImageBaseHeader* const swapchainImage = State->m_swapchainImages[viewSwapchain.handle][swapchainImageIndex];
        RenderView(
            ProjectionLayerViews[i],
            swapchainImage,
            State->m_environmentBlendMode,
            State->m_colorSwapchainFormat);
    }

    layer.space = State->m_sceneSpace.Get();
    layer.viewCount = (uint32_t)ProjectionLayerViews.size();
    layer.views = ProjectionLayerViews.data();
    return true;
}


} // namespace xrm
