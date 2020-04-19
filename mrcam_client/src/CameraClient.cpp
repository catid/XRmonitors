// Copyright 2019 Augmented Perception Corporation

#include "CameraClient.hpp"
#include "core_string.hpp"
#include "core_logger.hpp"

namespace core {

static logger::Channel Logger("CameraClient");


//------------------------------------------------------------------------------
// CameraClient

bool CameraClient::Start()
{
    Terminated = true;

    FrameEvent = ::OpenEventA(SYNCHRONIZE, FALSE, CAMERA_IMPLANT_FRAME_EVENT_NAME);
    if (FrameEvent.Invalid()) {
        Logger.Error("OpenEventA failed: ", WindowsErrorString(::GetLastError()));
        Logger.Error("Camera Service is not running.");
        return false;
    }

    if (!UiSharedFile.Open(kXrmUiSharedMemoryBytes, XRM_UI_SHARED_MEMORY_NAME)) {
        Logger.Error("UiSharedFile.Open failed: ", WindowsErrorString(::GetLastError()));
        return false;
    }
    UiSharedMemoryLayout = reinterpret_cast<XrmUiSharedMemoryLayout*>(UiSharedFile.GetFront());

    LastUiEpoch = 0x7fffff;

    if (!SharedFile.Open(kImplantSharedMemoryBytes, CAMERA_IMPLANT_SHARED_MEMORY_NAME)) {
        Logger.Error("SharedFile.Open failed: ", WindowsErrorString(::GetLastError()));
        return false;
    }
    SharedMemoryLayout = reinterpret_cast<ImplantSharedMemoryLayout*>( SharedFile.GetFront() );

    SharedMemoryLayout->EnableServiceHook = 1;

    Terminated = false;
    Thread = std::make_shared<std::thread>(&CameraClient::Loop, this);

    return true;
}

void CameraClient::Stop()
{
    {
        std::unique_lock<std::mutex> locker(Lock);
        Condition.notify_all();
    }

    Terminated = true;
    JoinThread(Thread);

    if (SharedMemoryLayout) {
        SharedMemoryLayout->EnableServiceHook = 0;
    }
}

bool CameraClient::AcquireNextFrame(CameraFrame& frame)
{
    std::lock_guard<std::mutex> locker(Lock);

    if (LatestFrame.FrameNumber == LastAcquiredFrameNumber) {
        return false;
    }
    LastAcquiredFrameNumber = LatestFrame.FrameNumber;

    FrameInUse = true;
    frame = LatestFrame;
    return true;
}

void CameraClient::ReleaseFrame()
{
    std::unique_lock<std::mutex> locker(Lock);
    FrameInUse = false;
    Condition.notify_all();
}

bool CameraClient::ReadUiState(XrmUiData& data)
{
    std::unique_lock<std::mutex> locker(UiLock);

    if (!UiSharedMemoryLayout) {
        return false;
    }

    uint32_t epoch = 0;
    bool success = UiSharedMemoryLayout->Read(epoch, data);
    if (!success) {
        return false;
    }

    if (epoch == LastUiEpoch) {
        return false;
    }
    LastUiEpoch = epoch;

    return true;
}


//------------------------------------------------------------------------------
// CameraClient: Loop

void CameraClient::Loop()
{
    SetCurrentThreadName("CameraClient");

    while (!Terminated)
    {
        DWORD result = ::WaitForSingleObject(FrameEvent.Get(), 200);
        if (result == WAIT_OBJECT_0) {
            ReadFrame();
        }
        else if (result == WAIT_TIMEOUT) {
            if (SharedMemoryLayout->ImplantInstalled) {
                Logger.Info("Implant: Installed");
            }
            else {
                Logger.Info("Implant: Not installed");
            }
        }
        else {
            Logger.Error("WaitForSingleObject failed: ", WindowsErrorString(::GetLastError()));
        }
    }
}

void CameraClient::ReadFrame()
{
    // unique_lock used since QueueCondition.wait requires it
    std::unique_lock<std::mutex> locker(Lock);

    if (Terminated) {
        return;
    }

    while (FrameInUse)
    {
        Condition.wait(locker);
        if (Terminated) {
            return;
        }
    }

    // FIXME: Check here if we are locked and wait here to be woken up

    uint32_t read_bytes, read_counter;
    uint64_t exposure_usec;
    bool success = SharedMemoryLayout->ReadCamera(CameraData, read_bytes, exposure_usec, read_counter);
    if (success) {
        const unsigned camera_width = 640;
        const unsigned camera_height = 480;
        const unsigned camera_bytes = camera_width * camera_height;
        const unsigned pair_bytes = camera_bytes * 2;

        const unsigned offset = 1312;

        if (offset + pair_bytes > read_bytes) {
            Logger.Error("Camera data is truncated");
            return;
        }

        LatestFrame.FrameNumber = read_counter;
        LatestFrame.Buffer = CameraData;
        LatestFrame.BufferBytes = read_bytes;
        LatestFrame.CameraImage = CameraData + offset;
        LatestFrame.Width = camera_width * 2;
        LatestFrame.Height = camera_height;
        LatestFrame.ExposureTimeUsec = exposure_usec;
    }
    else {
        Logger.Error("Failed to read frame");
    }
}


} // namespace core
