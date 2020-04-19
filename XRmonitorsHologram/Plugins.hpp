// Copyright 2019 Augmented Perception Corporation

/*
    This represents a texture shared with an external process.
    The external process is a "plugin" that provides a texture to render
    in VR.  The plugin does not update the texture every frame, so we must
    keep a copy of it locally as well.
*/

#pragma once

#include "D3D11Tools.hpp"
#include "MonitorRenderModel.hpp"
#include "OpenXrD3D11.hpp"
#include "xrm_plugins_abi.hpp"

#include <VertexTypes.h>

namespace xrm {


//------------------------------------------------------------------------------
// PluginServer

class PluginServer
{
    logger::Channel Logger;

public:
    PluginServer()
        : Logger("Plugin")
    {
    }

    void Initialize(
        int index,
        XrmPluginsMemoryLayout* shared_memory,
        XrHeadsetProperties* headset,
        XrRenderProperties* rendering,
        MonitorRenderModel* model,
        uint32_t mip_levels,
        uint32_t sample_count);
    void Shutdown();

    // Returns true if the texture should be rendered.
    // Returns false if the texture is hidden
    bool UpdateRenderModel();

    void CheckTimeout();

    void UpdateMesh();

    bool HitTest(int screen_x, int screen_y);

    void SetFocusAndGaze(int screen_x, int screen_y, bool focus);


    // VR Texture
    ComPtr<ID3D11Texture2D> VrRenderTexture;
    D3D11_TEXTURE2D_DESC VrRenderTextureDesc{};

protected:
    static const uint64_t kKeyedMutex_Plugin = 0;
    static const uint64_t kKeyedMutex_Host = 1;
    static const uint64_t kTimeoutMsec = 5 * 1000; //< 5 seconds

    XrHeadsetProperties* Headset = nullptr;
    XrRenderProperties* Rendering = nullptr;
    MonitorRenderModel* RenderModel = nullptr;

    int PluginIndex = -1;

    // Host -> Plugin
    XrmHostToPluginUpdater* HostToPlugin = nullptr;
    XrmHostToPluginData HostData{};

    // Plugin -> Host
    XrmPluginToHostUpdater* PluginToHost = nullptr;
    XrmPluginToHostData PluginData{};
    uint32_t PluginEpoch = 0;

    // Texture from external process
    ComPtr<ID3D11Texture2D> SharedTexture;
    D3D11_TEXTURE2D_DESC SharedTextureDesc{};

    // Keyed mutex to synchronize cross-D3D11Device operations
    ComPtr<IDXGIKeyedMutex> KeyedMutex;

    // KeyedMutex acquired by VR code?
    bool MutexAcquired = false;

    // Cylinder geometry for each monitor
    std::vector<DirectX::VertexPositionTexture> Vertices;
    ComPtr<ID3D11Buffer> VertexBuffer;
    std::vector<uint16_t> Indices;
    ComPtr<ID3D11Buffer> IndexBuffer;
    int IndexCount = 0;

    // Timeout
    TimeoutTimer Timeout;

    // Update event
    HostToPluginEvent UpdateEvent;


    bool CopyTexture();

    bool UpdateSharedTexture();

    bool UpdateVrTexture();

    void ReleaseRemoteTexture();

    void SignalTerminate();
};


//------------------------------------------------------------------------------
// PluginCylinderRenderer

struct PluginCylinderRenderer
{
    ComPtr<ID3D11VertexShader> VertexShader;
    ComPtr<ID3D11PixelShader> PixelShader;
    ComPtr<ID3D11InputLayout> InputLayout;
    ComPtr<ID3D11Buffer> MvpCBuffer;
    ComPtr<ID3D11Buffer> ColorCBuffer;
    ComPtr<ID3D11SamplerState> SamplerState;
    ComPtr<ID3D11BlendState> BlendState;


    void InitializeD3D(D3D11DeviceContext& device_context);

    void Render(
        D3D11DeviceContext& device_context,
        const DirectX::SimpleMath::Matrix& view_projection_matrix,
        const PluginRenderInfo* info);
};


//------------------------------------------------------------------------------
// PluginManager

/*
    The lifetime of this object is bounded by the OpenXR context.
*/
class PluginManager
{
public:
    void Initialize(
        XrHeadsetProperties* headset,
        XrRenderProperties* rendering,
        MonitorRenderModel* model);
    void Shutdown();

    // Update all plugins once per frame
    void Update();

    // Render multiple times for each eye
    void Render(const DirectX::SimpleMath::Matrix& view_projection_matrix);

    // Recreate mesh setup based on new desktop positions
    void SolveDesktopPositions();

protected:
    XrHeadsetProperties* Headset = nullptr;
    XrRenderProperties* Rendering = nullptr;
    MonitorRenderModel* RenderModel = nullptr;

    // Global shared memory file: Plugins
    SharedMemoryFile PluginsSharedFile;
    XrmPluginsMemoryLayout* PluginsSharedMemory = nullptr;

    // Texture and mesh for each plugin
    std::unique_ptr<PluginServer> PluginServers[XRM_PLUGIN_COUNT];

    // Renderer for all the plugins
    std::unique_ptr<PluginCylinderRenderer> CylinderRenderer;
};


} // namespace xrm
