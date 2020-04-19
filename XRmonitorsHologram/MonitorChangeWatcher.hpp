// Copyright 2019 Augmented Perception Corporation

#pragma once

#include "MonitorTools.hpp"

namespace xrm {


//------------------------------------------------------------------------------
// MonitorChangeWatcher

// Check and clear monitor updated flag
bool CheckMonitorsUpdated();

class MonitorChangeWatcher
{
public:
    void Initialize();
    void Shutdown();

protected:
    std::atomic<bool> Terminated = ATOMIC_VAR_INIT(false);
    std::shared_ptr<std::thread> Thread;

    HWND Window = nullptr;
    HDEVNOTIFY DevNotify = nullptr;


    void Loop();

    bool WindowStart();
    void WindowStop();
};

// Allow external code to trigger a monitor enumeration on e.g. error codes
void TriggerMonitorEnumeration();


} // namespace xrm
