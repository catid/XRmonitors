// Copyright 2019 Augmented Perception Corporation

#ifndef IMPLANT_ABI_HPP
#define IMPLANT_ABI_HPP

#include <stdint.h>
#include <string.h>

#include <string>
#include <atomic>


//------------------------------------------------------------------------------
// Constants

#define CAMERA_IMPLANT_SHARED_MEMORY_NAME "Global\\mrcam_implant"
#define CAMERA_IMPLANT_FRAME_EVENT_NAME   "Global\\mrcam_frame"


//------------------------------------------------------------------------------
// Implant Shared Memory Layout

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4324)  // Padded struct
#endif
#pragma pack(push)
#pragma pack(4)

struct ImplantSharedMemoryLayout
{
    // Increment this to trigger another USB HUB power tweak
    __declspec(align(256)) std::atomic<uint32_t> UsbHubPowerEpoch = ATOMIC_VAR_INIT(0);

    __declspec(align(256)) std::atomic<uint32_t> EnableServiceHook = ATOMIC_VAR_INIT(0);
    __declspec(align(256)) std::atomic<uint32_t> RemoveServiceHook = ATOMIC_VAR_INIT(0);

    __declspec(align(256)) std::atomic<uint32_t> ImplantStage = ATOMIC_VAR_INIT(0);
    __declspec(align(256)) std::atomic<uint32_t> ImplantInstalled = ATOMIC_VAR_INIT(0);
    __declspec(align(256)) std::atomic<uint32_t> ImplantRemoveGood = ATOMIC_VAR_INIT(0);
    __declspec(align(256)) std::atomic<uint32_t> ImplantRemoveFail = ATOMIC_VAR_INIT(0);

    static const unsigned kCameraBytes = 640 * 480 * 2 + 4096;

    // Writer will increment BeforeWriteCounter then write data and then
    // increment AfterWriteCounter.  Reader will read them in the same order
    // and read again if the counters do not match.
    __declspec(align(256)) std::atomic<uint32_t> BeforeWriteCounter = ATOMIC_VAR_INIT(0);
    __declspec(align(256)) volatile uint32_t CameraBytes;
    __declspec(align(256)) volatile uint64_t ExposureTimeUsec;
    __declspec(align(256)) volatile uint8_t CameraData[kCameraBytes];
    __declspec(align(256)) std::atomic<uint32_t> AfterWriteCounter = ATOMIC_VAR_INIT(0);

    void WriteCamera(
        const uint8_t* data,
        uint32_t read_bytes,
        uint64_t exposure_usec);
    bool ReadCamera(
        uint8_t* data,
        uint32_t& read_bytes,
        uint64_t& exposure_usec,
        uint32_t& read_counter);
};

#pragma pack(pop)
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

static const uint32_t kImplantSharedMemoryBytes = static_cast<uint32_t>(sizeof(ImplantSharedMemoryLayout));

#endif // IMPLANT_ABI_HPP
