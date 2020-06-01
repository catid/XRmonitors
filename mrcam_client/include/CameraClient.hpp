// Copyright 2019 Augmented Perception Corporation

#pragma once

#include "core.hpp"
#include "core_win32.hpp"

#include "implant_abi.hpp"
#include "xrm_ui_abi.hpp"

#include <vector>
#include <atomic>
#include <thread>
#include <memory>
#include <mutex>
#include <functional>
#include <condition_variable>

namespace core {


//------------------------------------------------------------------------------
// CameraClient


//------------------------------------------------------------------------------
// CameraClient

struct CameraFrame
{
    uint64_t ExposureTimeUsec = 0;
    unsigned FrameNumber = 0;

    uint8_t* Buffer = nullptr;
    unsigned BufferBytes = 0;

    uint8_t* CameraImage = nullptr;
    unsigned Width = 0;
    unsigned Height = 0;
};

/// This class is thread-safe
class CameraClient
{
public:
    bool Start();
    void Stop();

    // Returns true if a new frame is ready.
    // Returns false if prior frame is still active
    bool AcquireNextFrame(CameraFrame& frame);

    // Release frame when done
    void ReleaseFrame();

    // Returns true if new UI state was received.
    // Returns false if no updates have occurred.
    bool ReadUiState(XrmUiData& data);

    // Tell service to kick off USB hub power fix
    void ApplyUsbHubPowerFix()
    {
        SharedMemoryLayout->UsbHubPowerEpoch++;
    }

protected:
    SharedMemoryFile SharedFile;
    ImplantSharedMemoryLayout* SharedMemoryLayout = nullptr;

    // Global shared memory file
    SharedMemoryFile UiSharedFile;
    XrmUiSharedMemoryLayout* UiSharedMemoryLayout = nullptr;

    AutoEvent FrameEvent;

    std::atomic<bool> Terminated = ATOMIC_VAR_INIT(false);
    std::shared_ptr<std::thread> Thread;


    //--------------------------------------------------------------------------
    // Camera

    uint8_t CameraData[ImplantSharedMemoryLayout::kCameraBytes];

    std::mutex Lock;
    std::condition_variable Condition;

    bool FrameInUse = false;
    CameraFrame LatestFrame;
    unsigned LastAcquiredFrameNumber = 0;


    // Background thread
    void Loop();
    void ReadFrame();


    //--------------------------------------------------------------------------
    // UI

    std::mutex UiLock;

    // Update number for the UI state
    unsigned LastUiEpoch = 0;
};


} // namespace core
