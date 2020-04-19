// Copyright 2019 Augmented Perception Corporation

#ifndef XRM_PLUGINS_ABI_HPP
#define XRM_PLUGINS_ABI_HPP

#include <stdint.h>
#include <string.h>

#include <string>
#include <atomic>

#include "core_win32.hpp"


//------------------------------------------------------------------------------
// Constants

#define XRM_PLUGINS_SHARED_MEMORY_NAME "Global\\XRmonitorsPlugins"

// Each time the XrmHostToPluginData is updated, we signal this event.
// This allows the plugin to react faster to gaining focus.
// File names are e.g. "Global\\XRmonitorsPluginS2C_4" for plugin #4.
#define XRM_PLUGINS_S2C_EVENT_PREFIX "Global\\XRmonitorsPluginS2C_"

#define XRM_PLUGIN_COUNT 20


//------------------------------------------------------------------------------
// Host -> Plugin

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4324)  // Padded struct
#endif
#pragma pack(push)
#pragma pack(1)

struct XrmHostToPluginData
{
    // Nonzero: Plugin server is shutdown
    uint32_t Shutdown;

    // Change -> App should terminate
    uint32_t TerminateEpoch;

    // Texture format e.g. DXGI_FORMAT_B8G8R8A8_UNORM
    uint32_t Format;

    // Number of samples for textures
    uint32_t SampleCount;

    // Number of mip levels for textures
    uint32_t MipLevels;

    // Gaze X, Y
    int32_t GazeScreenX;
    int32_t GazeScreenY;

    // Nonzero: Has input focus?  Should capture input
    uint32_t HasFocus;

    // When this changes, it means the texture has been released
    // back to the plugin to own
    uint32_t TextureReleaseEpoch;

    // LUID
    uint32_t LuidLowPart;
    uint64_t LuidHighPart;
};

struct XrmHostToPluginUpdater
{
    std::atomic<uint32_t> BeforeWriteCounter = ATOMIC_VAR_INIT(0);
    uint8_t PaddingBefore[32 - 4];

    std::atomic<uint32_t> AfterWriteCounter = ATOMIC_VAR_INIT(0);
    uint8_t PaddingAfter[32 - 4];

    XrmHostToPluginData Data;


    void Write(const XrmHostToPluginData& data);
    bool Read(uint32_t& epoch, XrmHostToPluginData& data) const;
};


//------------------------------------------------------------------------------
// Plugin -> Host

struct XrmPluginToHostData
{
    // Must increment once every 5 seconds
    uint32_t KeepAliveEpoch;

    // Process Id that owns this slot
    uint32_t ProcessId;

    // When this changes, it means the application should re-open the texture
    uint32_t TextureHandleEpoch;

    // When this changes, it means the texture has been released
    // back to the host to own
    uint32_t TextureReleaseEpoch;

    // Center of plugin texture on cylinder in screen coordinates.
    // Can be placed over or outside of the real monitors.
    int32_t X;
    int32_t Y;

    // Dimensions of the plugin texture in pixels
    int32_t Width;
    int32_t Height;
};

struct XrmPluginToHostUpdater
{
    std::atomic<uint32_t> BeforeWriteCounter = ATOMIC_VAR_INIT(0);
    uint8_t PaddingBefore[32 - 4];

    std::atomic<uint32_t> AfterWriteCounter = ATOMIC_VAR_INIT(0);
    uint8_t PaddingAfter[32 - 4];

    XrmPluginToHostData Data;


    void Write(const XrmPluginToHostData& data);
    bool Read(uint32_t& epoch, XrmPluginToHostData& data) const;
};


//------------------------------------------------------------------------------
// XrmPluginsMemoryLayout

struct XrmPluginsMemoryLayout
{
    XrmHostToPluginUpdater HostToPlugin[XRM_PLUGIN_COUNT];
    XrmPluginToHostUpdater PluginToHost[XRM_PLUGIN_COUNT];
};

#pragma pack(pop)
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

static const uint32_t kXrmPluginsMemoryLayoutBytes = static_cast<uint32_t>(sizeof(XrmPluginsMemoryLayout));


//------------------------------------------------------------------------------
// Host -> Plugin Event

class HostToPluginEvent
{
public:
    // Client-side:

    // Open the event for the given plugin index
    // Returns false on failure to open
    bool Open(int plugin_index);

    // Wait for signal
    // Returns true on signal
    // Returns false on timeout
    bool Wait(int timeout_msec);

    // Server-side:

    // Signal event
    void Signal();

protected:
    core::AutoEvent Event;
};


#endif // XRM_PLUGINS_ABI_HPP
