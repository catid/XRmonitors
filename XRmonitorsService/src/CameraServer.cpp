#include "CameraServer.hpp"
#include "RemoteProcess.hpp"
#include "UsbHubPowerFix.hpp"

#include "core_string.hpp"
#include "core_logger.hpp"

#include <TlHelp32.h> // CreateToolhelp32Snapshot

namespace core {

static logger::Channel Logger("CameraServer");


//-----------------------------------------------------------------------------
// Debugging Tools

static BOOL SetPrivilege(
    HANDLE hToken,          // token handle
    LPCTSTR Privilege,      // Privilege to enable/disable
    BOOL bEnablePrivilege   // TRUE to enable.  FALSE to disable
)
{
    TOKEN_PRIVILEGES tp;
    LUID luid;
    TOKEN_PRIVILEGES tpPrevious;
    DWORD cbPrevious = sizeof(TOKEN_PRIVILEGES);

    if (!LookupPrivilegeValue(NULL, Privilege, &luid)) {
        Logger.Error("LookupPrivilegeValue failed: ", WindowsErrorString(::GetLastError()));
        return FALSE;
    }

    // 
    // first pass.  get current privilege setting
    // 
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = 0;

    AdjustTokenPrivileges(
        hToken,
        FALSE,
        &tp,
        sizeof(TOKEN_PRIVILEGES),
        &tpPrevious,
        &cbPrevious
    );

    if (GetLastError() != ERROR_SUCCESS) {
        Logger.Error("AdjustTokenPrivileges failed: ", WindowsErrorString(::GetLastError()));
        return FALSE;
    }

    // 
    // second pass.  set privilege based on previous setting
    // 
    tpPrevious.PrivilegeCount = 1;
    tpPrevious.Privileges[0].Luid = luid;

    if (bEnablePrivilege) {
        tpPrevious.Privileges[0].Attributes |= (SE_PRIVILEGE_ENABLED);
    }
    else {
        tpPrevious.Privileges[0].Attributes ^= (SE_PRIVILEGE_ENABLED &
            tpPrevious.Privileges[0].Attributes);
    }

    AdjustTokenPrivileges(
        hToken,
        FALSE,
        &tpPrevious,
        cbPrevious,
        NULL,
        NULL
    );

    if (GetLastError() != ERROR_SUCCESS) {
        Logger.Error("AdjustTokenPrivileges2 failed: ", WindowsErrorString(::GetLastError()));
        return FALSE;
    }

    return TRUE;
}

bool EnableDebugToken()
{
    HANDLE hToken = INVALID_HANDLE_VALUE;
    if (!::OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &hToken))
    {
        if (::GetLastError() != ERROR_NO_TOKEN) {
            Logger.Error("OpenThreadToken failed: ", WindowsErrorString(::GetLastError()));
            return false;
        }

        if (!::ImpersonateSelf(SecurityImpersonation)) {
            Logger.Error("ImpersonateSelf failed: ", WindowsErrorString(::GetLastError()));
            return false;
        }

        if (!::OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &hToken)) {
            Logger.Error("OpenThreadToken2 failed: ", WindowsErrorString(::GetLastError()));
            return false;
        }
    }

    if (!SetPrivilege(hToken, SE_DEBUG_NAME, TRUE)) {
        ::CloseHandle(hToken);
        return false;
    }

    // Leak handle intentionally
    return true;
}


//------------------------------------------------------------------------------
// Tools

static bool FileExists(const std::string& name)
{
    struct stat buffer {};
    return (stat(name.c_str(), &buffer) == 0);
}

static HANDLE CreateGlobalFrameEvent(const char* event_name)
{
    std::vector<uint8_t> desc(SECURITY_DESCRIPTOR_MIN_LENGTH);
    SECURITY_ATTRIBUTES sa;
    sa.lpSecurityDescriptor = (PSECURITY_DESCRIPTOR)desc.data();
    InitializeSecurityDescriptor(sa.lpSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
    // ACL is set as NULL in order to allow all access to the object.
    SetSecurityDescriptorDacl(sa.lpSecurityDescriptor, TRUE, NULL, FALSE);
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = FALSE;

    return ::CreateEventA(&sa, FALSE, FALSE, event_name);
}


//------------------------------------------------------------------------------
// Find Process ID

static bool ProcessHasMrUsbHost(uint32_t pid)
{
    AutoHandle snapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
    if (snapshot.Invalid()) {
        Logger.Error("CreateToolhelp32Snapshot TH32CS_SNAPMODULE failed: ", WindowsErrorString(::GetLastError()));
        return false;
    }

    MODULEENTRY32 pme{};
    pme.dwSize = sizeof(pme);

    if (!::Module32First(snapshot.Get(), &pme)) {
        Logger.Error("Module32First failed: ", WindowsErrorString(::GetLastError()));
        return false;
    }

    const char* mrusbhost = "MRUSBHost";

    do {
        //Logger.Debug("Module: ", pme.szModule);

        if (StrIStr(pme.szModule, mrusbhost)) {
            return true;
        }

    } while (::Module32Next(snapshot.Get(), &pme));

    return false;
}

static bool FindMrUsbHostProcesses(std::vector<uint32_t>& pids)
{
    pids.clear();

    AutoHandle snapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot.Invalid()) {
        Logger.Error("CreateToolhelp32Snapshot TH32CS_SNAPPROCESS failed: ", WindowsErrorString(::GetLastError()));
        return false;
    }

    PROCESSENTRY32 ppe{};
    ppe.dwSize = sizeof(ppe);

    if (!::Process32First(snapshot.Get(), &ppe)) {
        Logger.Error("Process32First failed: ", WindowsErrorString(::GetLastError()));
        return false;
    }

    const char* wudfhost = "WUDFHost";

    do {
        if (StrIStr(ppe.szExeFile, wudfhost)) {
            const uint32_t pid = ppe.th32ProcessID;

            //Logger.Debug("Process pid=", pid, ": ", ppe.szExeFile);

            if (ProcessHasMrUsbHost(pid)) {
                Logger.Debug("Process found: pid=", pid, ": ", ppe.szExeFile);

                pids.push_back(pid);
            }
        }

    } while (::Process32Next(snapshot.Get(), &ppe));

    return true;
}


//------------------------------------------------------------------------------
// CameraServer

bool CameraServer::Start()
{
    FrameEvent = CreateGlobalFrameEvent(CAMERA_IMPLANT_FRAME_EVENT_NAME);
    if (FrameEvent.Invalid()) {
        Logger.Error("CreateGlobalFrameEvent failed: ", WindowsErrorString(::GetLastError()));
        return false;
    }

    //--------------------------------------------------------------------------
    // Global shared memory file: Implant

    if (!ImplantSharedFile.Create(kImplantSharedMemoryBytes, CAMERA_IMPLANT_SHARED_MEMORY_NAME)) {
        Logger.Error("ImplantSharedFile.Create failed: ", WindowsErrorString(::GetLastError()));
        return false;
    }
    ImplantSharedMemory = reinterpret_cast<ImplantSharedMemoryLayout*>(ImplantSharedFile.GetFront());
    memset(ImplantSharedMemory, 0, kImplantSharedMemoryBytes);

    //--------------------------------------------------------------------------
    // Global shared memory file: UI

    if (!UiSharedFile.Create(kXrmUiSharedMemoryBytes, XRM_UI_SHARED_MEMORY_NAME)) {
        Logger.Error("UiSharedFile.Create failed: ", WindowsErrorString(::GetLastError()));
        return false;
    }
    UiSharedMemory = reinterpret_cast<XrmUiSharedMemoryLayout*>(UiSharedFile.GetFront());
    memset(UiSharedMemory, 0, kXrmUiSharedMemoryBytes);

    UiSharedMemory->Data.DisableWinY = false;
    UiSharedMemory->Data.EnablePassthrough = true;

    //--------------------------------------------------------------------------
    // Global shared memory file: Plugins

    if (!PluginsSharedFile.Create(kXrmPluginsMemoryLayoutBytes, XRM_PLUGINS_SHARED_MEMORY_NAME)) {
        Logger.Error("PluginsSharedFile.Create failed: ", WindowsErrorString(::GetLastError()));
        return false;
    }
    PluginsSharedMemory = reinterpret_cast<XrmPluginsMemoryLayout*>(PluginsSharedFile.GetFront());
    memset(PluginsSharedMemory, 0, kXrmPluginsMemoryLayoutBytes);

    //--------------------------------------------------------------------------
    // Global events triggered by host indicating the plugin has data to read

    for (int i = 0; i < XRM_PLUGIN_COUNT; ++i)
    {
        std::string event_name = XRM_PLUGINS_S2C_EVENT_PREFIX;
        event_name += std::to_string(i);

        HostToPluginEvents[i] = CreateGlobalFrameEvent(event_name.c_str());

        if (HostToPluginEvents[i].Invalid())
        {
            Logger.Error("HostToPluginEvents CreateEventA failed: ", WindowsErrorString(::GetLastError()), " for plugin ", i);
            return false;
        }
    }

    Terminated = false;
    Thread = std::make_shared<std::thread>(&CameraServer::Loop, this);

    return true;
}

void CameraServer::Stop()
{
    Terminated = true;
    JoinThread(Thread);

    if (ImplantSharedMemory) {
        ImplantSharedMemory->RemoveServiceHook = 1;
    }
    ImplantSharedFile.Close();
    UiSharedFile.Close();
    PluginsSharedFile.Close();
}

void CameraServer::CheckApplyUsbHubPowerFix()
{
    if (EpochUsbHubPower != ImplantSharedMemory->UsbHubPowerEpoch)
    {
        EpochUsbHubPower = ImplantSharedMemory->UsbHubPowerEpoch;

        Logger.Info("Applying USB hub power fix");

        ApplyUsbHubPowerFix();
    }
}

void CameraServer::Loop()
{
    SetCurrentThreadName("Injector");

    Logger.Debug("Injector thread started");

    if (!EnableDebugToken()) {
        Logger.Error("Failed to enable debug token");
    }

    while (!Terminated)
    {
        if (ImplantSharedMemory->EnableServiceHook) {
            std::vector<uint32_t> pids;
            if (FindMrUsbHostProcesses(pids)) {
                if (!pids.empty()) {
                    InjectPid(pids[0]);
                }
            }
        }

        for (int i = 0; i < 50; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (Terminated || ImplantSharedMemory->EnableServiceHook) {
                break;
            }

            // Apply the fix when cameras are not enabled
            CheckApplyUsbHubPowerFix();
        }
    }

    Logger.Debug("Injector thread terminated");
}

void CameraServer::InjectPid(uint32_t pid)
{
    Logger.Info("Opening target process id: ", pid);

    // Open server process for duplication and synchronization on termination
    AutoEvent RemoteProcess = ::OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION | \
        PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_DUP_HANDLE | SYNCHRONIZE,
        FALSE, pid);
    if (RemoteProcess.Invalid()) {
        Logger.Error("OpenProcess failed: ", WindowsErrorString(::GetLastError()));
    }

    // Detect if target is 64-bit or 32-bit
    BOOL isWOW64 = FALSE;
    // "WOW64 is the x86 emulator that allows 32-bit Windows-based applications to run seamlessly on 64-bit Windows"
    if (!::IsWow64Process(RemoteProcess.Get(), &isWOW64)) {
        Logger.Error("IsWow64Process failed: ", WindowsErrorString(::GetLastError()));
    }

    bool TargetIs32 = (isWOW64 != FALSE);

    const char* file_name = nullptr;
    if (TargetIs32) {
        file_name = "XRmonitorsCamera.dll";
        Logger.Debug("Target is 32-bit");
    }
    else {
        file_name = "XRmonitorsCamera.dll";
        Logger.Debug("Target is 64-bit");
    }

    std::string copy_src_path = GetFullFilePathFromRelative(file_name);

    if (!FileExists(copy_src_path)) {
        Logger.Error("Required module not found: ", copy_src_path);
        return;
    }

    std::string execute_path = copy_src_path + ".implanted";

    Logger.Info("Copying: ", copy_src_path);

    BOOL copy_result = ::CopyFileA(
        copy_src_path.c_str(),
        execute_path.c_str(),
        FALSE);

    if (!copy_result) {
        Logger.Warning("CopyFileA failed: ", WindowsErrorString(::GetLastError()));
        execute_path = copy_src_path;
    }

    Logger.Info("Implanting: ", execute_path);

    ImplantSharedMemory->ImplantStage = 0;
    ImplantSharedMemory->ImplantInstalled = 0;
    ImplantSharedMemory->ImplantRemoveFail = 0;
    ImplantSharedMemory->ImplantRemoveGood = 0;
    ImplantSharedMemory->RemoveServiceHook = 0;
    ::ResetEvent(FrameEvent.Get());

    int remote_load_result = RemoteLoadLibrary(
        RemoteProcess.Get(),
        execute_path);

    if (remote_load_result != 0) {
        Logger.Error("RemoteLoadLibrary failed: ", remote_load_result);
        return;
    }

    // Wait for implant to end:

    if (!ImplantSharedMemory->ImplantInstalled) {
        Logger.Error("Implant reports installation failure at stage = ", ImplantSharedMemory->ImplantStage);
    }
    else {
        Logger.Info("Implant success");

        while (!Terminated && ImplantSharedMemory->EnableServiceHook)
        {
            DWORD result = ::WaitForSingleObject(RemoteProcess.Get(), 100);
            if (result == WAIT_TIMEOUT) {

                // Apply the fix when cameras are enabled
                CheckApplyUsbHubPowerFix();

                continue;
            }
            if (result == WAIT_OBJECT_0) {
                Logger.Info("Watched process terminated");
                break;
            }
            Logger.Warning("Unexpected wait result: ", result, " error=", WindowsErrorString(::GetLastError()));
            continue;
        }
    }

    // Remove hook:

    ImplantSharedMemory->RemoveServiceHook = 1;

    // Wait up to 1 second
    for (int i = 0; i < 10; ++i)
    {
        if (Terminated) {
            break;
        }

        if (ImplantSharedMemory->ImplantRemoveFail) {
            Logger.Error("Implant reports removal failure");
            break;
        }
        if (ImplantSharedMemory->ImplantRemoveGood) {
            Logger.Info("Implant reports removal Success");
            break;
        }

        DWORD result = ::WaitForSingleObject(RemoteProcess.Get(), 100);
        if (result == WAIT_TIMEOUT) {
            continue;
        }
        if (result == WAIT_OBJECT_0) {
            Logger.Info("Watched process terminated");
            break;
        }
        Logger.Warning("Unexpected wait result: ", result, " error=", WindowsErrorString(::GetLastError()));
        continue;
    }
}


} // namespace core
