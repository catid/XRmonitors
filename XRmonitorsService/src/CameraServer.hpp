#pragma once

#include "core.hpp"
#include "core_win32.hpp"

#include "implant_abi.hpp"
#include "xrm_ui_abi.hpp"
#include "xrm_plugins_abi.hpp"

#include <vector>
#include <atomic>
#include <thread>
#include <memory>

namespace core {


//------------------------------------------------------------------------------
// CameraServer

// Periodically scans for the Windows Mixed Reality runtime and injects into it.
// Creates the shared memory region used to coordinate client and implant.
class CameraServer
{
public:
    bool Start();
    void Stop();

protected:
    // Global shared memory file: Implant
    SharedMemoryFile ImplantSharedFile;
    ImplantSharedMemoryLayout* ImplantSharedMemory = nullptr;

    // Global shared memory file: UI
    SharedMemoryFile UiSharedFile;
    XrmUiSharedMemoryLayout* UiSharedMemory = nullptr;

    // Global shared memory file: Plugins
    SharedMemoryFile PluginsSharedFile;
    XrmPluginsMemoryLayout* PluginsSharedMemory = nullptr;

    // Global events triggered by host indicating the plugin has data to read
    AutoEvent HostToPluginEvents[XRM_PLUGIN_COUNT];

    // Global event set from implant to wake up listeners for frames
    AutoEvent FrameEvent;

    // Last epoch for USB hub power fix
    uint32_t EpochUsbHubPower = 0;

    /*
        Background thread
        (1) Scan for MR runtime periodically
        (2) Inject into it
        (3) Wait for termination
    */
    std::atomic<bool> Terminated = ATOMIC_VAR_INIT(false);
    std::shared_ptr<std::thread> Thread;


    void Loop();

    void InjectPid(uint32_t pid);

    void CheckApplyUsbHubPowerFix();
};


} // namespace core
