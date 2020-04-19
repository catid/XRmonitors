// Copyright 2019 Augmented Perception Corporation

#include "stdafx.h"

#include "MonitorEnumerator.hpp"

#include <sstream>

#include <SetupAPI.h>
#include <cfgmgr32.h> // for MAX_DEVICE_ID_LEN
#pragma comment(lib, "setupapi.lib")

namespace xrm {

static logger::Channel Logger("Enumerator");


//------------------------------------------------------------------------------
// Constants

// D3DCreate parameter for duplication D3DDevice
static const D3D_FEATURE_LEVEL kFeatureLevels[] =
{
    D3D_FEATURE_LEVEL_12_1,
    D3D_FEATURE_LEVEL_12_0,
    D3D_FEATURE_LEVEL_11_1,
    D3D_FEATURE_LEVEL_11_0
};
static const UINT kNumFeatureLevels = CORE_ARRAY_COUNT(kFeatureLevels);


//------------------------------------------------------------------------------
// MonitorEnumInfo

void MonitorEnumInfo::LogInfo()
{
    const char* special_type = "(Normal)";
    if (HeadlessGhost) {
        special_type = "(Headless)";
    }
    else if (DuetDisplay) {
        special_type = "(DuetDisplay)";
    }

    Logger.Info(
        IsPrimary ? "Primary " : "Secondary ",
        MonitorInterfaceTypeString(InterfaceType),
        " monitor: ", DeviceName, " \"", DisplayName, "\" ",
        special_type);

    Logger.Info(
        "*** Vendor: ", VendorName,
        " Model # ", ModelNumber,
        " Serial # ", SerialNumber,
        " (", Serial, ")");
    Logger.Info("*** Physical size: ", WidthMm, " x ", HeightMm, " mm");
    Logger.Info("*** Resolution: ", DeviceSpaceWidth, " x ", DeviceSpaceHeight, " pixels");
    Logger.Info("*** Desktop Position: (", Coords.left, ", ", Coords.top, ")");
    Logger.Info("*** Desktop Rotation: ", DxgiRotationString(Rotation));
    Logger.Info("*** Adapter LUID: ", LUIDToString(AdapterLuid));
}


//------------------------------------------------------------------------------
// MonitorEnumerator

void MonitorEnumerator::Initialize()
{
    MonitorWatcher.Initialize();

    TriggerMonitorEnumeration();
}

void MonitorEnumerator::Shutdown()
{
    MonitorWatcher.Shutdown();
    Monitors.clear();
}

bool MonitorEnumerator::UpdateEnumeration()
{
    const uint64_t now_msec = ::GetTickCount64();

    if (CheckMonitorsUpdated()) {
        UpdateInitiatedTimeMsec = now_msec;
        return false;
    }

    if (UpdateInitiatedTimeMsec != 0 && now_msec - UpdateInitiatedTimeMsec > 500) {
        EnumerateMonitors();
        UpdateInitiatedTimeMsec = 0;
        return true;
    }

    return false;
}

std::shared_ptr<MonitorEnumInfo> MonitorEnumerator::FindOrCreateMonitor(
    HMONITOR hmonitor,
    bool& is_new_monitor)
{
    std::shared_ptr<MonitorEnumInfo> matched_monitor;
    for (auto& monitor : Monitors) {
        if (monitor->MonitorHandle == hmonitor) {
            matched_monitor = monitor;
            break;
        }
    }
    is_new_monitor = false;
    if (!matched_monitor) {
        matched_monitor = std::make_shared<MonitorEnumInfo>();
        is_new_monitor = true;
    }
    return matched_monitor;
}

void MonitorEnumerator::FillDisplayDeviceInterfaceNames()
{
    for (unsigned i = 0; ; i++)
    {
        DISPLAY_DEVICEA dd{};
        dd.cb = sizeof(dd);

        if (!::EnumDisplayDevicesA(NULL, i, &dd, 0)) {
            break;
        }
        if ((dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) == 0) {
            continue;
        }
        if ((dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) != 0) {
            continue;
        }

        dd.DeviceName[sizeof(dd.DeviceName) - 1] = '\0';

        std::shared_ptr<MonitorEnumInfo> monitor_info;
        for (auto& monitor : Monitors)
        {
            if (0 == StrCaseCompare(monitor->DeviceName.c_str(), dd.DeviceName)) {
                monitor_info = monitor;
                break;
            }
        }
        if (!monitor_info) {
            continue;
        }

        for (unsigned j = 0; ; j++)
        {
            DISPLAY_DEVICEA mi{};
            mi.cb = sizeof(mi);

            if (!::EnumDisplayDevicesA(dd.DeviceName, j, &mi, EDD_GET_DEVICE_INTERFACE_NAME)) {
                break;
            }

            mi.DeviceID[sizeof(mi.DeviceID) - 1] = '\0';
            monitor_info->DeviceInterfaceName = mi.DeviceID;
            break;
        }
    }
}

static const GUID GUID_DEVINTERFACE_MONITOR = {
    0xe6f07b5f, 0xee97, 0x4a90, 0xb0, 0x76, 0x33, 0xf5, 0x7b, 0xf4, 0xea, 0xa7
};

static const uint8_t EDID_SIGNATURE[] = {
    0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00
};

void MonitorEnumerator::FillEdidFromRegistry()
{
    HDEVINFO dev_info_handle = ::SetupDiGetClassDevsA(
        &GUID_DEVINTERFACE_MONITOR,
        nullptr,
        nullptr,
        DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    if (dev_info_handle == INVALID_HANDLE_VALUE) {
        Logger.Error("SetupDiGetClassDevsA failed: err=", WindowsErrorString(::GetLastError()));
        return;
    }
    ScopedFunction dev_info_handle_scope([&]() {
        ::SetupDiDestroyDeviceInfoList(dev_info_handle);
    });

    const DWORD detail_data_len = offsetof(SP_DEVICE_INTERFACE_DETAIL_DATA_A, DevicePath) + MAX_PATH;
    std::vector<uint8_t> detail_data(detail_data_len);
    PSP_DEVICE_INTERFACE_DETAIL_DATA_A detail_data_ptr = (PSP_DEVICE_INTERFACE_DETAIL_DATA_A)&detail_data[0];
    detail_data_ptr->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);

    bool bRes = false;

    for (ULONG i = 0; ; ++i)
    {
        SP_DEVICE_INTERFACE_DATA interface_data{};
        interface_data.cbSize = sizeof(interface_data);

        BOOL enum_success = ::SetupDiEnumDeviceInterfaces(
            dev_info_handle,
            nullptr,
            &GUID_DEVINTERFACE_MONITOR,
            i,
            &interface_data);

        if (!enum_success) {
            const DWORD err = ::GetLastError();
            if (err != ERROR_NO_MORE_ITEMS) {
                Logger.Error("SetupDiEnumDeviceInterfaces failed: ", WindowsErrorString(err));
            }
            break;
        }

        SP_DEVINFO_DATA devinfo_data{};
        devinfo_data.cbSize = sizeof(devinfo_data);

        BOOL detail_success = ::SetupDiGetDeviceInterfaceDetailA(
            dev_info_handle,
            &interface_data,
            detail_data_ptr,
            detail_data_len,
            nullptr,
            &devinfo_data);

        if (!detail_success) {
            Logger.Error("SetupDiGetDeviceInterfaceDetailA failed: ", WindowsErrorString(::GetLastError()));
            continue;
        }

        // Find the associated monitor
        std::shared_ptr<MonitorEnumInfo> monitor_info;
        for (auto& monitor : Monitors) {
            if (0 == StrCaseCompare(detail_data_ptr->DevicePath, monitor->DeviceInterfaceName.c_str())) {
                monitor_info = monitor;
                break;
            }
        }
        if (!monitor_info) {
            Logger.Warning("Unmatched monitor device path: ", detail_data_ptr->DevicePath);
            continue;
        }

        HKEY key = ::SetupDiOpenDevRegKey(
            dev_info_handle,
            &devinfo_data,
            DICS_FLAG_GLOBAL,
            0,
            DIREG_DEV,
            KEY_READ);
        if (key == INVALID_HANDLE_VALUE) {
            Logger.Error("SetupDiOpenDevRegKey failed: ", WindowsErrorString(::GetLastError()));
            continue;
        }
        ScopedFunction key_scope([&]() {
            ::RegCloseKey(key);
        });

        for (unsigned j = 0; ; ++j)
        {
            char name[256];
            DWORD name_len = 256;
            DWORD type = 0;
            uint8_t* data = monitor_info->Edid;
            DWORD data_len = sizeof(monitor_info->Edid);
            LSTATUS result = ::RegEnumValueA(
                key,
                j,
                name,
                &name_len,
                nullptr,
                &type,
                data,
                &data_len);

            if (result != ERROR_SUCCESS) {
                if (result != ERROR_NO_MORE_ITEMS) {
                    Logger.Error("RegEnumValueA failed: ", WindowsErrorString(result));
                }
                break;
            }

            // Check that it is named EDID
            if (name_len > 255) {
                name_len = 255;
            }
            name[name_len] = '\0';
            if (0 != StrCaseCompare(name, "edid")) {
                continue;
            }

            // EDID format:
            // https://en.wikipedia.org/wiki/Extended_Display_Identification_Data

            if (data_len < 23) {
                Logger.Warning("EDID truncated at ", data_len, " bytes");
                continue;
            }
            if (0 != memcmp(EDID_SIGNATURE, data, sizeof(EDID_SIGNATURE))) {
                Logger.Warning("EDID had invalid signature");
                continue;
            }

            monitor_info->EdidBytes = data_len;

            const bool is_digital = (data[20] & 128) != 0;
            if (!is_digital) {
                monitor_info->InterfaceType = EdidMonitorInterfaceType::Analog;
            }
            else {
                unsigned type = data[20] & 15;
                if (type == 1 || type == 2) {
                    monitor_info->InterfaceType = EdidMonitorInterfaceType::HDMI;
                }
                else if (type == 5) {
                    monitor_info->InterfaceType = EdidMonitorInterfaceType::DisplayPort;
                }
                else {
                    monitor_info->InterfaceType = EdidMonitorInterfaceType::Unknown;
                }
            }

            monitor_info->WidthMm = data[21] * 10;
            monitor_info->HeightMm = data[22] * 10;

            // Extract the 5-bit chars for the Vendor ID (PNP code)
            char vendor_name[4] = {
                'A' + ((data[8] >> 2) & 31) - 1,
                'A' + (((data[8] & 2) << 3) | (data[9] >> 5)) - 1,
                'A' + (data[9] & 31),
                '\0'
            };
            monitor_info->VendorName = vendor_name;
            monitor_info->ModelNumber = core::ReadU16_LE(data + 10);
            monitor_info->SerialNumber = core::ReadU32_LE(data + 12);

            unsigned desc_offset = 54;
            for (unsigned k = 0; k < 4; ++k) {
                if (desc_offset + 18 > data_len) {
                    break;
                }
                uint8_t* desc = data + desc_offset;
                desc_offset += 18;

                if (core::ReadU16_LE(desc) == 0) {
                    if (desc[3] == 0xFC) {
                        char str[13 + 1];
                        memcpy(str, desc + 5, 13);
                        str[13] = '\0';
                        StripNonprintables(str);
                        monitor_info->DisplayName = str;
                        monitor_info->HeadlessGhost = IsHeadlessGhost(str);
                        monitor_info->DuetDisplay = IsDuetDisplay(str);
                    }
                    if (desc[3] == 0xFF) {
                        char str[13 + 1];
                        memcpy(str, desc + 5, 13);
                        str[13] = '\0';
                        StripNonprintables(str);
                        monitor_info->Serial = str;
                    }
                }
            }
        }
    }
}

void MonitorEnumerator::EnumerateMonitors()
{
    HDESK hDesktop = ::OpenInputDesktop(0, FALSE, GENERIC_ALL);
    if (!hDesktop) {
        Logger.Warning("OpenInputDesktop failed: ", WindowsErrorString(::GetLastError()));
    }
    else {
        bool success = ::SetThreadDesktop(hDesktop) != 0;
        ::CloseDesktop(hDesktop);
        if (!success) {
            Logger.Warning("SetThreadDesktop failed: ", WindowsErrorString(::GetLastError()));
        }
    }

    const unsigned update_epoch = ++UpdateEpoch;

    Monitors.clear();

    Logger.Info("Enumerating monitors epoch=", update_epoch);

    // Must recreate the factory because all the adapters are cached on creation
    ComPtr<IDXGIFactory2> factory;
    HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));
    if (FAILED(hr) || !factory) {
        Logger.Error("CreateDXGIFactory2 failed: ", HresultString(hr));
        return;
    }

    const HMONITOR primary_monitor = GetPrimaryMonitorHandle();

    unsigned adapter_count = 0;

    for (unsigned i = 0; i < 16; ++i)
    {
        ComPtr<IDXGIAdapter1> adapter;
        hr = factory->EnumAdapters1(i, &adapter);
        if (hr == DXGI_ERROR_NOT_FOUND) {
            break;
        }
        if (FAILED(hr) || !adapter) {
            Logger.Error("EnumAdapters failed: ", HresultString(hr));
            continue;
        }

        DXGI_ADAPTER_DESC adapter_desc{};
        hr = adapter->GetDesc(&adapter_desc);
        if (FAILED(hr)) {
            Logger.Warning("GetDesc failed: ", HresultString(hr));
        }
        const std::string luid_str = LUIDToString(adapter_desc.AdapterLuid);

        adapter_desc.Description[CORE_ARRAY_COUNT(adapter_desc.Description) - 1] = 0;
        std::wstring desc_wstr = adapter_desc.Description;
        std::string desc_str = WideStringToUtf8String(desc_wstr);

        Logger.Info("Adapter ", i, " LUID=", luid_str, " Description: ", desc_str);

        // Count this as an adapter if we got this far
        ++adapter_count;

        for (unsigned j = 0; j < 16; ++j)
        {
            ComPtr<IDXGIOutput> DxgiOutput;
            hr = adapter->EnumOutputs(j, &DxgiOutput);
            if (hr == DXGI_ERROR_NOT_FOUND) {
                break;
            }
            if (FAILED(hr)) {
                Logger.Warning("EnumOutputs failed: ", HresultString(hr));
                continue;
            }

            ComPtr<IDXGIOutput1> DxgiOutput1;
            hr = DxgiOutput.As(&DxgiOutput1);
            if (FAILED(hr)) {
                Logger.Warning("DxgiOutput.As failed: ", HresultString(hr));
                continue;
            }

            DXGI_OUTPUT_DESC output_desc{};
            hr = DxgiOutput->GetDesc(&output_desc);
            if (FAILED(hr)) {
                Logger.Warning("GetDesc failed: ", HresultString(hr));
                continue;
            }

            output_desc.DeviceName[CORE_ARRAY_COUNT(output_desc.DeviceName) - 1] = 0;
            std::wstring device_name_wstr = output_desc.DeviceName;
            std::string device_name_str = WideStringToUtf8String(device_name_wstr);

            Logger.Info("*** Output ", j, " ", device_name_str,
                " AttachedToDesktop=", output_desc.AttachedToDesktop);

            if (!output_desc.AttachedToDesktop) {
                Logger.Warning("AttachedToDesktop = false");
                continue;
            }

            std::vector<D3D_FEATURE_LEVEL> feature_levels = {
                D3D_FEATURE_LEVEL_12_1,
                D3D_FEATURE_LEVEL_12_0,
                D3D_FEATURE_LEVEL_11_1,
                D3D_FEATURE_LEVEL_11_0,
                D3D_FEATURE_LEVEL_10_1,
                D3D_FEATURE_LEVEL_10_0
            };

            bool is_new_monitor = false;
            std::shared_ptr<MonitorEnumInfo> monitor
                = FindOrCreateMonitor(output_desc.Monitor, is_new_monitor);
            if (!monitor) {
                Logger.Error("OOM");
                continue;
            }

            bool success = monitor->DC.CreateForAdapter(adapter.Get(), feature_levels);
            if (!success) {
                Logger.Error("Failed to create D3D11 device context for monitor");
                continue;
            }

            monitor->UpdateEpoch = update_epoch;
            monitor->DxgiOutput1 = DxgiOutput1;
            monitor->AdapterLuid = adapter_desc.AdapterLuid;

            // Read monitor name
            output_desc.DeviceName[31] = 0;
            char monitor_name_cstr[64];
            size_t monitor_name_len = 0;
            errno_t err = ::wcstombs_s(&monitor_name_len, monitor_name_cstr, output_desc.DeviceName, _TRUNCATE);
            if (err == 0 || err == STRUNCATE) {
                monitor_name_cstr[63] = '\0';
                monitor->DeviceName = monitor_name_cstr;
            }

            // Check if monitor is primary
            monitor->IsPrimary = (primary_monitor == output_desc.Monitor);

            monitor->Rotation = output_desc.Rotation;
            monitor->Coords = output_desc.DesktopCoordinates;
            monitor->MonitorHandle = output_desc.Monitor;

            monitor->ScreenSpaceWidth = AbsWidth(monitor->Coords.right, monitor->Coords.left);
            monitor->ScreenSpaceHeight = AbsWidth(monitor->Coords.bottom, monitor->Coords.top);

            bool portrait_mode = false;
            switch (output_desc.Rotation)
            {
            case DXGI_MODE_ROTATION_UNSPECIFIED:
            case DXGI_MODE_ROTATION_IDENTITY:
                monitor->RotationDegrees = 0.f;
                portrait_mode = false;
                break;
            case DXGI_MODE_ROTATION_ROTATE90:
                monitor->RotationDegrees = 90.f;
                portrait_mode = true;
                break;
            case DXGI_MODE_ROTATION_ROTATE180:
                monitor->RotationDegrees = 180.f;
                portrait_mode = false;
                break;
            case DXGI_MODE_ROTATION_ROTATE270:
                monitor->RotationDegrees = 270.f;
                portrait_mode = true;
                break;
            }
            monitor->IsPortraitMode = portrait_mode;

            if (portrait_mode) {
                monitor->DeviceSpaceWidth = monitor->ScreenSpaceHeight;
                monitor->DeviceSpaceHeight = monitor->ScreenSpaceWidth;
            }
            else {
                monitor->DeviceSpaceWidth = monitor->ScreenSpaceWidth;
                monitor->DeviceSpaceHeight = monitor->ScreenSpaceHeight;
            }

            Monitors.push_back(monitor);
        }
    }

    FillDisplayDeviceInterfaceNames();
    FillEdidFromRegistry();

    int monitor_count = (int)Monitors.size();
    for (int i = 0; i < monitor_count; ++i) {
        if (Monitors[i]->UpdateEpoch != update_epoch) {
            // Remove and delete object and continue iterating
            --monitor_count;
            if (i != monitor_count) {
                Monitors[i].swap(Monitors[monitor_count]);
            }
            Monitors[monitor_count].reset();
            Monitors.resize(monitor_count);
        }
        else {
            // Assign updated indices
            Monitors[i]->MonitorIndex = i;
        }
    }
}


} // namespace xrm
