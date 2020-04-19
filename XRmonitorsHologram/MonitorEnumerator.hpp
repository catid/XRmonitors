// Copyright 2019 Augmented Perception Corporation

#pragma once

#include "MonitorTools.hpp"
#include "MonitorChangeWatcher.hpp"

namespace xrm {


//------------------------------------------------------------------------------
// MonitorEnumInfo

struct MonitorEnumInfo
{
    // e.g. \\.\DISPLAY1
    std::string DeviceName;

    // Is this the primary monitor?
    bool IsPrimary = false;

    // e.g. \\?\display#gsm5b09#5&c27ae1b&0&uid12551#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7
    std::string DeviceInterfaceName;

    // EDID from registry
    uint8_t Edid[1024];
    unsigned EdidBytes = 0;

    // EDID parsed data
    EdidMonitorInterfaceType InterfaceType = EdidMonitorInterfaceType::Unknown;
    std::string VendorName;
    uint16_t ModelNumber = 0;
    std::string DisplayName;
    std::string Serial;
    uint32_t SerialNumber = 0;

    // Physical size (Inaccurate for HeadlessGhost and DuetDisplay)
    // This is in the natural orientation of the display panel.
    unsigned WidthMm = 0;
    unsigned HeightMm = 0;

    // Position and orientation of the monitor
    RECT Coords;

    // Note the rotation does not affect D3D11 desktop duplication
    DXGI_MODE_ROTATION Rotation;
    float RotationDegrees = 0.f;

    // Is it on its side?  Determines screen versus device space rotation
    bool IsPortraitMode = false;

    // Screen-space dimensions.
    // e.g. If the monitor is rotated its Height > Width.
    unsigned ScreenSpaceWidth = 0;
    unsigned ScreenSpaceHeight = 0;

    // Device-space dimensions.
    // e.g. If the monitor is rotated on the desktop, this does not change.
    unsigned DeviceSpaceWidth = 0;
    unsigned DeviceSpaceHeight = 0;

    // Host adapter LUID
    LUID AdapterLuid;

    // "A physical display has the same HMONITOR as long as it is part of the desktop"
    HMONITOR MonitorHandle = nullptr;

    // Is this a Headless Ghost private mode display?
    bool HeadlessGhost = false;

    // Is this a Duet Display?
    bool DuetDisplay = false;

    // Which monitor is this?
    int MonitorIndex = 0;

    // D3D11 device context that can be used for desktop duplication
    D3D11DeviceContext DC;
    ComPtr<IDXGIOutput1> DxgiOutput1;

    // Counter that increments for each update to check for removed monitors
    unsigned UpdateEpoch = 0;

    // Last time we received a mouse update
    uint64_t LastMouseUpdateTicks = 0;


    void LogInfo();
};

//------------------------------------------------------------------------------
// MonitorEnumerator

class MonitorEnumerator
{
public:
    void Initialize();
    void Shutdown();

    // Performs scheduled enumeration tasks
    // After this call the monitor list may be updated
    // Returns true if enumeration occurred
    bool UpdateEnumeration();

    // Result of enumeration
    std::vector<std::shared_ptr<MonitorEnumInfo>> Monitors;
    unsigned UpdateEpoch = 0;

protected:
    MonitorChangeWatcher MonitorWatcher;

    // Time when update was first requested
    uint64_t UpdateInitiatedTimeMsec = 0;

    void EnumerateMonitors();

    // From an array of device names, fill in DeviceInterfaceNames
    void FillDisplayDeviceInterfaceNames();

    // From hardware id and driver, fill in EDID
    void FillEdidFromRegistry();

    std::shared_ptr<MonitorEnumInfo> FindOrCreateMonitor(
        HMONITOR hmonitor,
        bool& is_new_monitor);
};


} // namespace xrm
