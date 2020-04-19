// Copyright 2019 Augmented Perception Corporation

#include "xrm_ui_abi.hpp"
#include <windows.h> // Sleep


//------------------------------------------------------------------------------
// XRmonitors UI Shared Memory Layout

bool XrmUiSharedMemoryLayout::Read(uint32_t& epoch, XrmUiData& data) const
{
    for (int retries = 0; retries < 20; ++retries)
    {
        uint32_t read_counter = BeforeWriteCounter;

        data = Data;

        const uint32_t after = AfterWriteCounter;

        if (read_counter == after) {
            epoch = after;
            return true;
        }

        ::Sleep(1);
    }

    return false;
}
