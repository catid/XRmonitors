// Copyright 2019 Augmented Perception Corporation

#include "xrm_plugins_abi.hpp"


//------------------------------------------------------------------------------
// Host -> Plugin

void XrmHostToPluginUpdater::Write(const XrmHostToPluginData& data)
{
    const uint32_t epoch = BeforeWriteCounter + 1;

    BeforeWriteCounter = epoch;

    Data = data;

    AfterWriteCounter = epoch;
}

bool XrmHostToPluginUpdater::Read(uint32_t& epoch, XrmHostToPluginData& data) const
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


//------------------------------------------------------------------------------
// Plugin -> Host

void XrmPluginToHostUpdater::Write(const XrmPluginToHostData& data)
{
    const uint32_t epoch = BeforeWriteCounter + 1;

    BeforeWriteCounter = epoch;

    Data = data;

    AfterWriteCounter = epoch;
}

bool XrmPluginToHostUpdater::Read(uint32_t& epoch, XrmPluginToHostData& data) const
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


//------------------------------------------------------------------------------
// Host -> Plugin Event

static std::string EventNameForPluginIndex(int plugin_index)
{
    std::string event_name = XRM_PLUGINS_S2C_EVENT_PREFIX;
    event_name += std::to_string(plugin_index);
    return event_name;
}

bool HostToPluginEvent::Open(int plugin_index)
{
    const std::string event_name = EventNameForPluginIndex(plugin_index);
    Event = ::OpenEventA(SYNCHRONIZE | EVENT_MODIFY_STATE, FALSE, event_name.c_str());
    return Event.Valid();
}

bool HostToPluginEvent::Wait(int timeout_msec)
{
    const DWORD result = ::WaitForSingleObject(Event.Get(), timeout_msec);
    if (result == WAIT_OBJECT_0) {
        return true;
    }
    if (result != WAIT_TIMEOUT) {
        // Sleep on errors that are not timeouts
        ::Sleep(timeout_msec);
    }
    return false;
}

void HostToPluginEvent::Signal()
{
    ::SetEvent(Event.Get());
}
