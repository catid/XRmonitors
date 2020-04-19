// Copyright 2019 Augmented Perception Corporation

#include "implant_abi.hpp"

#include "core.hpp"
#include "core_win32.hpp" // Sleep


//------------------------------------------------------------------------------
// Implant Shared Memory Layout

void ImplantSharedMemoryLayout::WriteCamera(
    const uint8_t* data,
    uint32_t read_bytes,
    uint64_t exposure_usec)
{
    uint32_t counter = ++BeforeWriteCounter;
    ExposureTimeUsec = exposure_usec;
    CameraBytes = read_bytes;
    memcpy((void*)CameraData, data, read_bytes);
    AfterWriteCounter = counter;
}

bool ImplantSharedMemoryLayout::ReadCamera(
    uint8_t* data,
    uint32_t& read_bytes,
    uint64_t& exposure_usec,
    uint32_t& read_counter)
{
    for (int retries = 0; retries < 20; ++retries)
    {
        exposure_usec = ExposureTimeUsec;
        read_counter = BeforeWriteCounter;
        read_bytes = CameraBytes;

        memcpy(data, (void*)&CameraData[0], read_bytes);

        const uint32_t after = AfterWriteCounter;

        if (read_counter == after) {
            return true;
        }

        ::Sleep(1);
    }

    return false;
}
