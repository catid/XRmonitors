// Copyright 2019 Augmented Perception Corporation

#include "UsbHubPowerFix.hpp"

#include "core_string.hpp"
#include "core_logger.hpp"

#include <comdef.h>
#include <Dbt.h>
#include <SetupAPI.h>
#include <Cfgmgr32.h>
#include <InitGuid.h>
#include <Devpkey.h>
#include <usbiodef.h> // GUID_DEVINTERFACE_USB_DEVICE
#include <usbioctl.h>
#include <SetupAPI.h>

#pragma comment(lib, "cfgmgr32")
#pragma comment(lib, "setupapi")

namespace core {

static logger::Channel Logger("UsbHub");


//------------------------------------------------------------------------------
// Constants

#ifndef GUID_POWER_DEVICE_ENABLE
DEFINE_GUID(GUID_POWER_DEVICE_ENABLE, 0x827c0a6fL, 0xfeb0, 0x11d0, 0xbd, 0x26, 0x00, 0xaa, 0x00, 0xb7, 0xb3, 0x2a);
#endif // GUID_POWER_DEVICE_ENABLE


//------------------------------------------------------------------------------
// Tools

std::string HresultString(HRESULT hr)
{
    std::ostringstream oss;
    oss << _com_error(hr).ErrorMessage() << " [hr=" << hr << "]";
    return oss.str();
}

std::string WideStringToUtf8String(const std::wstring& wide)
{
    if (wide.empty()) {
        return "";
    }

    // Calculate necessary buffer size
    int len = ::WideCharToMultiByte(
        CP_UTF8,
        0,
        wide.c_str(),
        static_cast<int>(wide.size()),
        NULL,
        0,
        NULL,
        NULL);

    // Perform actual conversion
    if (len > 0)
    {
        std::vector<char> buffer(len);

        len = ::WideCharToMultiByte(
            CP_UTF8,
            0,
            wide.c_str(),
            static_cast<int>(wide.size()),
            &buffer[0],
            static_cast<int>(buffer.size()),
            NULL,
            NULL);

        if (len > 0)
        {
            return std::string(&buffer[0], buffer.size());
        }
    }

    return "<failure>";
}


//------------------------------------------------------------------------------
// WmiFunctions

#ifndef WNODE_HEADER
typedef struct _WNODE_HEADER {
    ULONG BufferSize;
    ULONG ProviderId;
    union {
        ULONG64 HistoricalContext;
        struct {
            ULONG Version;
            ULONG Linkage;
        } Hdr;
    } DUMMYUNIONNAME;
    union {
        ULONG         CountLost;
        HANDLE        KernelHandle;
        LARGE_INTEGER TimeStamp;
    } DUMMYUNIONNAME2;
    GUID  Guid;
    ULONG ClientContext;
    ULONG Flags;
} WNODE_HEADER, * PWNODE_HEADER;

typedef struct tagWNODE_SINGLE_INSTANCE {
    struct _WNODE_HEADER WnodeHeader;
    ULONG                OffsetInstanceName;
    ULONG                InstanceIndex;
    ULONG                DataBlockOffset;
    ULONG                SizeDataBlock;
    UCHAR                VariableData[1];
} WNODE_SINGLE_INSTANCE, * PWNODE_SINGLE_INSTANCE;
#endif // WNODE_HEADER

typedef HRESULT(WINAPI* WOB) (LPGUID lpGUID, DWORD nDesiredAccess, OUT PVOID* DataBlockObject);
typedef HRESULT(WINAPI* WQSI)(PVOID DataBlockObject, WCHAR* wsInstanceName, __inout ULONG* nInOutBufferSize, OUT PVOID OutBuffer);
typedef HRESULT(WINAPI* WDITIN)(WCHAR* wsInstanceName, ULONG nSize, WCHAR* wsInstanceId, ULONG);
typedef HRESULT(WINAPI* WSSI)(PVOID DataBlockObject, WCHAR* wsInstanceName, ULONG nVersion, ULONG nValueBufferSize, PVOID ValueBuffer);

struct WmiFunctions
{
    HINSTANCE hAdvapiLib = nullptr;

    WOB WmiOpenBlock = nullptr;
    WQSI WmiQuerySingleInstance = nullptr;
    WDITIN WmiDevInstToInstanceName = nullptr;
    WSSI WmiSetSingleInstance = nullptr;


    bool Initialize();
    void Shutdown();
};

bool WmiFunctions::Initialize()
{
    hAdvapiLib = ::LoadLibraryW(L"advapi32.dll");
    if (!hAdvapiLib) {
        Logger.Error("LoadLibraryW failed: ", WindowsErrorString(::GetLastError()));
        return false;
    }

    WmiOpenBlock = (WOB)GetProcAddress(hAdvapiLib, "WmiOpenBlock");
    WmiQuerySingleInstance = (WQSI)GetProcAddress(hAdvapiLib, "WmiQuerySingleInstanceW");
    WmiSetSingleInstance = (WSSI)GetProcAddress(hAdvapiLib, "WmiSetSingleInstanceW");
    WmiDevInstToInstanceName = (WDITIN)GetProcAddress(hAdvapiLib, "WmiDevInstToInstanceNameW");

    if (!WmiOpenBlock || !WmiQuerySingleInstance || !WmiSetSingleInstance || !WmiDevInstToInstanceName) {
        Logger.Error("advapi32 Wmi API does not contain an expected export");
        return false;
    }

    return true;
}

void WmiFunctions::Shutdown()
{
    if (hAdvapiLib) {
        ::FreeLibrary(hAdvapiLib);
        hAdvapiLib = nullptr;
    }
}


//------------------------------------------------------------------------------
// UsbHubPowerFix

void ApplyUsbHubPowerFix()
{
    WmiFunctions wmi;
    if (!wmi.Initialize()) {
        Logger.Error("Failed to initialize WMI");
        return;
    }
    ScopedFunction wmi_scope([&]() {
        wmi.Shutdown();
    });

    HDEVINFO hDevInfo = ::SetupDiGetClassDevsW(
        &GUID_DEVINTERFACE_USB_DEVICE,
        nullptr,
        nullptr,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (hDevInfo == INVALID_HANDLE_VALUE) {
        Logger.Error("SetupDiGetClassDevsW failed: ", WindowsErrorString(::GetLastError()));
        return;
    }
    ScopedFunction handle_scope([&]() {
        ::SetupDiDestroyDeviceInfoList(hDevInfo);
    });

    for (DWORD member_index = 0;; ++member_index)
    {
        SP_DEVICE_INTERFACE_DATA iface_data{};
        iface_data.cbSize = sizeof(iface_data);

        BOOL result = ::SetupDiEnumDeviceInterfaces(
            hDevInfo,
            nullptr,
            &GUID_DEVINTERFACE_USB_DEVICE,
            member_index,
            &iface_data);

        if (!result) {
            break;
        }

        if ((iface_data.Flags & SPINT_ACTIVE) == 0) {
            continue;
        }

        char buf[sizeof(DWORD) + (MAX_PATH * sizeof(wchar_t))];

        DWORD bufferSize = sizeof(buf);
        DWORD requiredSize = 0;
 
        PSP_INTERFACE_DEVICE_DETAIL_DATA_W pFnClassDevData = (PSP_INTERFACE_DEVICE_DETAIL_DATA_W)buf;
        pFnClassDevData->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA_W);

        SP_DEVINFO_DATA devinfo_data{};
        devinfo_data.cbSize = sizeof(SP_DEVINFO_DATA);

        result = ::SetupDiGetDeviceInterfaceDetailW(
            hDevInfo,
            &iface_data,
            pFnClassDevData,
            bufferSize,
            &requiredSize,
            &devinfo_data);

        if (!result) {
            Logger.Warning("SetupDiGetDeviceInterfaceDetailW failed: ", WindowsErrorString(::GetLastError()));
            continue;
        }

        std::string str = WideStringToUtf8String(pFnClassDevData->DevicePath);

        if (!StrIStr(str.c_str(), "pid_0659")) {
            continue;
        }

        DEVINST devinst = devinfo_data.DevInst;

        for (int i = 0; i < 16; ++i)
        {
            CONFIGRET cm_result = ::CM_Get_Parent(&devinst, devinst, 0);

            if (cm_result != CR_SUCCESS) {
                Logger.Error("CM_Get_Parent failed: ", cm_result);
                break;
            }

            wchar_t parent_id[256];
            cm_result = ::CM_Get_Device_IDW(devinst, parent_id, sizeof(parent_id), 0);
            parent_id[255] = '\0';
            if (cm_result != CR_SUCCESS) {
                Logger.Error("CM_Get_Device_IDW failed: ", cm_result);
                continue;
            }

            void* hWmiBlock = nullptr;
            HRESULT wmi_result = wmi.WmiOpenBlock((LPGUID)&GUID_POWER_DEVICE_ENABLE, 0, &hWmiBlock);
            if (FAILED(wmi_result)) {
                Logger.Error("WmiOpenBlock failed: ", HresultString(wmi_result));
                continue;
            }

            WCHAR wmiInstance[MAX_PATH];
            wmi_result = wmi.WmiDevInstToInstanceName(wmiInstance, CORE_ARRAY_COUNT(wmiInstance), parent_id, 0);
            if (FAILED(wmi_result)) {
                Logger.Error("WmiDevInstToInstanceName failed: ", HresultString(wmi_result));
                continue;
            }

            ULONG buffer_size = 0;
            wmi_result = wmi.WmiQuerySingleInstance(hWmiBlock, wmiInstance, &buffer_size, nullptr);
            if ((FAILED(wmi_result) && wmi_result != ERROR_INSUFFICIENT_BUFFER) || buffer_size <= 0) {
                Logger.Warning("WmiQuerySingleInstance failed: ", HresultString(wmi_result));
                Logger.Info("Stopped walking USB hub chain where power settings stop being available");
                break;
            }

            std::vector<uint8_t> buffer(buffer_size);
            wmi_result = wmi.WmiQuerySingleInstance(hWmiBlock, wmiInstance, &buffer_size, buffer.data());
            if (FAILED(wmi_result) || buffer_size < sizeof(WNODE_HEADER)) {
                Logger.Error("WmiQuerySingleInstance(2) failed: ", HresultString(wmi_result));
                continue;
            }

            WNODE_SINGLE_INSTANCE* node = (WNODE_SINGLE_INSTANCE*)buffer.data();
            const ULONG nVersion = node->WnodeHeader.Hdr.Version;

            if (node->SizeDataBlock == 1 && node->DataBlockOffset + node->SizeDataBlock <= buffer_size)
            {
                uint8_t* data = buffer.data() + node->DataBlockOffset;
                Logger.Info("Prior GUID_POWER_DEVICE_ENABLE value = ", (int)*data);
            }

            BOOLEAN bNewValue = FALSE;
            wmi_result = wmi.WmiSetSingleInstance(hWmiBlock, wmiInstance, 0, sizeof(bNewValue), &bNewValue);
            if (FAILED(wmi_result) || bNewValue != FALSE) {
                Logger.Error("WmiSetSingleInstance failed: ", HresultString(wmi_result));
                continue;
            }

            std::string hub_path = WideStringToUtf8String(parent_id);
            Logger.Info("Reconfigured USB power settings for hub path: ", hub_path);
        } // for each usb hub chained off device
    } // for each usb device
}


} // namespace core
