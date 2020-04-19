#include "CameraServer.hpp"
#include "core_logger.hpp"
#include "core_string.hpp"
#include "core_win32.hpp"

#include <cwchar>

#include <io.h>
#include <fcntl.h>
#include <ios>

#include <fstream>
#include <iomanip>
#include <ctime>
#include <time.h>

#include <shlwapi.h>
#include <shlobj.h>
#pragma comment(lib,"shlwapi.lib")

using namespace core;
using namespace std;

static logger::Channel Logger("MrCamServer");


//-----------------------------------------------------------------------------
// Constants

#define EVENT_SOURCE_NAME "XRmonitorsService"

#define MRCAM_SERVICE_NAME "XRmonitorsService"

#define MRCAM_SERVICE_DISPLAY_NAME "XRmonitors Service"
#define MRCAM_SERVICE_DESC "XRmonitors: Virtual Multi-monitors for the Workplace.  This service is required for proper functioning of the software.  To uninstall the software, use Add/Remove programs instead."


//-----------------------------------------------------------------------------
// Camera Server

static CameraServer* m_CameraServer = nullptr;

static bool StartCameraServer()
{
    if (m_CameraServer) {
        return true;
    }

    m_CameraServer = new CameraServer;
    if (!m_CameraServer->Start()) {
        Logger.Error("m_CameraServer->Start() failed");
        m_CameraServer->Stop();
        delete m_CameraServer;
        m_CameraServer = nullptr;
        return false;
    }

    return true;
}

static void StopCameraServer()
{
    if (m_CameraServer)
    {
        m_CameraServer->Stop();
        delete m_CameraServer;
        m_CameraServer = nullptr;
    }
}


//-----------------------------------------------------------------------------
// Logger

static std::string GetLogFilePath()
{
    char szPath[MAX_PATH];

    HRESULT hr = ::SHGetFolderPathA(NULL, CSIDL_COMMON_APPDATA, NULL, 0, szPath);

    if (FAILED(hr)) {
        Logger.Error("SHGetFolderPathA failed: ", hr);
        return "";
    }

    BOOL result = ::PathAppendA(szPath, "\\XRmonitors");
    if (!result) {
        Logger.Error("PathAppendA failed: ", WindowsErrorString(::GetLastError()));
        return "";
    }

    return szPath;
}

static std::ofstream OutputLogFile;
static unsigned OutputLogFileIndex = 0;
static std::string LogFilePath;
static logger::Level MessageBoxMinLevel = logger::Level::Silent;
static bool EnableEventLog = false;

static void OpenNextLogFile()
{
    std::string log_file_path = LogFilePath;
    log_file_path += "\\XRmonitorsService.";
    log_file_path += std::to_string(OutputLogFileIndex);
    log_file_path += ".log";

    OutputLogFile.open(log_file_path);
    if (!OutputLogFile) {
        Logger.Error("Failed to open log file: ", log_file_path);
    }
    else {
        Logger.Error("Log file: ", log_file_path);

        std::time_t t = std::time(nullptr);
        std::tm tm_result{};
        localtime_s(&tm_result, &t);
        auto ts = std::put_time(&tm_result, "%d-%m-%Y %H:%M:%S (d-m-Y local time)");
        OutputLogFile << "Log file opened: " << ts << std::endl;
    }

    ++OutputLogFileIndex;
    if (OutputLogFileIndex >= 100) {
        OutputLogFileIndex = 0;
    }
}

static void LogCallback(logger::QueuedMessage& message, std::string& formatted)
{
    CORE_UNUSED(message);

    ::OutputDebugStringA(formatted.c_str());

    if (message.LogLevel >= MessageBoxMinLevel) {
        UINT type = MB_OK;
        if (message.LogLevel == logger::Level::Error) {
            type |= MB_ICONERROR;
        }
        else if (message.LogLevel == logger::Level::Warning) {
            type |= MB_ICONWARNING;
        }
        else {
            type |= MB_ICONINFORMATION;
        }
        ::MessageBoxA(nullptr, message.Message.c_str(), MRCAM_SERVICE_DISPLAY_NAME, type);
    }

#if 0
    if (!EnableEventLog) {
        return;
    }
#endif

    if (OutputLogFile) {
        OutputLogFile.write(formatted.c_str(), formatted.size());
        OutputLogFile.flush();

        auto pos = OutputLogFile.tellp();

        if (pos >= 100 * 1000)
        {
            OutputLogFile << "Cycling log file." << std::endl;
            OutputLogFile.close();

            OpenNextLogFile();
        }
    }
}

static void SetupLogging()
{
    OutputLogFileIndex = (GetTimeUsec() / 333) % 100;

    LogFilePath = GetLogFilePath();

    ::CreateDirectoryA(LogFilePath.c_str(), nullptr);

    OpenNextLogFile();

    logger::SetLogCallback(&LogCallback);
}

#if 0
static void LogCallback(logger::QueuedMessage& message, std::string& formatted)
{
    const char* cstr = formatted.c_str();

    ::OutputDebugStringA(cstr);

    if (message.LogLevel >= MessageBoxMinLevel) {
        UINT type = MB_OK;
        if (message.LogLevel == logger::Level::Error) {
            type |= MB_ICONERROR;
        }
        else if (message.LogLevel == logger::Level::Warning) {
            type |= MB_ICONWARNING;
        }
        else {
            type |= MB_ICONINFORMATION;
        }
        ::MessageBoxA(nullptr, message.Message.c_str(), MRCAM_SERVICE_DISPLAY_NAME, type);
    }

    if (!EnableEventLog) {
        return;
    }

    HANDLE hEventSource = ::RegisterEventSourceA(nullptr, EVENT_SOURCE_NAME);
    if (!hEventSource) {
        ::OutputDebugStringA("RegisterEventSourceA failed");
        return;
    }

    WORD level = EVENTLOG_INFORMATION_TYPE;
    if (message.LogLevel == logger::Level::Error) {
        level = EVENTLOG_ERROR_TYPE;
    }
    else if (message.LogLevel == logger::Level::Warning) {
        level = EVENTLOG_WARNING_TYPE;
    }

    BOOL success = ::ReportEventA(
        hEventSource,
        level,
        0,
        0,
        nullptr,
        1,
        0,
        &cstr,
        nullptr);

    if (!success) {
        ::OutputDebugStringA("ReportEventA failed");
    }

    ::DeregisterEventSource(hEventSource);
}
#endif

//-----------------------------------------------------------------------------
// Tools

static bool GlobalIsAlreadyRunning()
{
    HANDLE globalMutex = ::CreateMutexW(nullptr, FALSE, L"Global\\MrCamServiceInstance");
    if (!globalMutex || ::GetLastError() == ERROR_ALREADY_EXISTS) {
        return true;
    }
    return false;
}

static const char* GetServiceStateString(DWORD state)
{
    switch (state) {
    case SERVICE_STOPPED: return "SERVICE_STOPPED";
    case SERVICE_START_PENDING: return "SERVICE_START_PENDING";
    case SERVICE_STOP_PENDING: return "SERVICE_STOP_PENDING";
    case SERVICE_RUNNING: return "SERVICE_RUNNING";
    case SERVICE_CONTINUE_PENDING: return "SERVICE_CONTINUE_PENDING";
    case SERVICE_PAUSE_PENDING: return "SERVICE_PAUSE_PENDING";
    case SERVICE_PAUSED: return "SERVICE_PAUSED";
    default: break;
    }
    return "Unknown";
}


//-----------------------------------------------------------------------------
// Application Mode

static void MrRunApplicationMode()
{
    if (GlobalIsAlreadyRunning()) {
        Logger.Error("Already running in the background");
        return;
    }

    if (!StartCameraServer()) {
        return;
    }

    MessageBoxMinLevel = logger::Level::Error;

    ::MessageBoxA(nullptr, "Press OK to stop the demo mode", MRCAM_SERVICE_DISPLAY_NAME, MB_OK);

    StopCameraServer();
}

static bool MrInstallService()
{
    std::string filepath = GetFullFilePathFromRelative("XRmonitorsService.exe");
    std::stringstream ssquoted;
    ssquoted << "\"" << filepath << "\"";
    std::string quoted_filepath = ssquoted.str();

    SC_HANDLE hManager = ::OpenSCManagerA(
        nullptr,
        nullptr,
        SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE);
    if (!hManager) {
        Logger.Error("OpenSCManagerA failed: ", WindowsErrorString(::GetLastError()));
        return false;
    }
    ScopedFunction hManager_scope([&]() {
        ::CloseServiceHandle(hManager);
    });

    SC_HANDLE hService = ::CreateServiceA(
        hManager,
        MRCAM_SERVICE_NAME,
        MRCAM_SERVICE_DISPLAY_NAME,
        SERVICE_START | SERVICE_QUERY_STATUS | SERVICE_CHANGE_CONFIG,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START,
        SERVICE_ERROR_NORMAL,
        quoted_filepath.c_str(),
        nullptr,
        nullptr,
        "",
        nullptr,
        nullptr);
    if (!hService) {
        Logger.Error("CreateServiceA failed: ", WindowsErrorString(::GetLastError()));
        return false;
    }
    ScopedFunction hService_scope([&]() {
        ::CloseServiceHandle(hService);
    });

    SERVICE_DESCRIPTIONA desc{};
    desc.lpDescription = MRCAM_SERVICE_DESC;
    BOOL success = ::ChangeServiceConfig2A(hService, SERVICE_CONFIG_DESCRIPTION, &desc);
    if (!success) {
        Logger.Warning("ChangeServiceConfig2A failed: ", WindowsErrorString(::GetLastError()));
    }

    Logger.Debug("Successfully installed the service");
    return true;
}

static bool MrStartService()
{
    SC_HANDLE hManager = ::OpenSCManagerA(
        nullptr,
        nullptr,
        SC_MANAGER_CONNECT);
    if (!hManager) {
        Logger.Error("OpenSCManagerA failed: ", WindowsErrorString(::GetLastError()));
        return false;
    }
    ScopedFunction hManager_scope([&]() {
        ::CloseServiceHandle(hManager);
    });

    SC_HANDLE hService = ::OpenServiceA(
        hManager,
        MRCAM_SERVICE_NAME,
        SERVICE_START | SERVICE_QUERY_STATUS);
    if (!hService) {
        Logger.Error("OpenServiceA failed: ", WindowsErrorString(::GetLastError()));
        return false;
    }
    ScopedFunction hService_scope([&]() {
        ::CloseServiceHandle(hService);
    });

    BOOL start_success = ::StartServiceA(hService, 0, nullptr);
    if (!start_success) {
        Logger.Error("StartServiceA failed: ", WindowsErrorString(::GetLastError()));
        return false;
    }

    bool success = false;
    for (int i = 0; i < 100; ++i)
    {
        ::Sleep(200);

        SERVICE_STATUS status{};
        if (!::QueryServiceStatus(hService, &status)) {
            Logger.Error("QueryServiceStatus failed: ", WindowsErrorString(::GetLastError()));
            return false;
        }

        Logger.Debug("Service state: ", GetServiceStateString(status.dwCurrentState));

        if (status.dwCurrentState == SERVICE_START_PENDING) {
            continue;
        }

        if (status.dwCurrentState == SERVICE_RUNNING) {
            Logger.Debug("Service running");
            success = true;
        }

        break;
    }

    return success;
}

static bool MrStopService()
{
    SC_HANDLE hManager = ::OpenSCManagerA(
        nullptr,
        nullptr,
        SC_MANAGER_CONNECT);
    if (!hManager) {
        Logger.Error("OpenSCManagerA failed: ", WindowsErrorString(::GetLastError()));
        return false;
    }
    ScopedFunction hManager_scope([&]() {
        ::CloseServiceHandle(hManager);
    });

    SC_HANDLE hService = ::OpenServiceA(
        hManager,
        MRCAM_SERVICE_NAME,
        SERVICE_STOP | SERVICE_QUERY_STATUS);
    if (!hService) {
        Logger.Error("OpenServiceA failed: ", WindowsErrorString(::GetLastError()));
        return false;
    }
    ScopedFunction hService_scope([&]() {
        ::CloseServiceHandle(hService);
    });

    SERVICE_STATUS stop_status{};
    BOOL stop_success = ::ControlService(hService, SERVICE_CONTROL_STOP, &stop_status);
    if (!stop_success) {
        Logger.Error("ControlService failed: ", WindowsErrorString(::GetLastError()));
        return false;
    }

    bool success = false;
    for (int i = 0; i < 100; ++i)
    {
        ::Sleep(200);

        SERVICE_STATUS status{};
        if (!::QueryServiceStatus(hService, &status)) {
            Logger.Error("QueryServiceStatus failed: ", WindowsErrorString(::GetLastError()));
            return false;
        }

        Logger.Debug("Service state: ", GetServiceStateString(status.dwCurrentState));

        if (status.dwCurrentState == SERVICE_STOP_PENDING) {
            continue;
        }

        if (status.dwCurrentState == SERVICE_STOPPED) {
            Logger.Debug("Service stopped");
            success = true;
        }

        break;
    }

    return success;
}

static bool MrUninstallService()
{
    SC_HANDLE hManager = ::OpenSCManagerA(
        nullptr,
        nullptr,
        SC_MANAGER_CONNECT);
    if (!hManager) {
        Logger.Error("OpenSCManagerA failed: ", WindowsErrorString(::GetLastError()));
        return false;
    }
    ScopedFunction hManager_scope([&]() {
        ::CloseServiceHandle(hManager);
    });

    SC_HANDLE hService = ::OpenServiceA(
        hManager,
        MRCAM_SERVICE_NAME,
        DELETE);
    if (!hService) {
        Logger.Error("OpenServiceA failed: ", WindowsErrorString(::GetLastError()));
        return false;
    }
    ScopedFunction hService_scope([&]() {
        ::CloseServiceHandle(hService);
    });

    if (!::DeleteService(hService)) {
        Logger.Error("DeleteService failed: ", WindowsErrorString(::GetLastError()));
        return false;
    }

    return true;
}


//-----------------------------------------------------------------------------
// Entrypoint

static SERVICE_STATUS_HANDLE m_StatusHandle = nullptr;

static void UpdateServiceState(DWORD state)
{
    SERVICE_STATUS status{};
    status.dwCurrentState = state;
    status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    if (state == SERVICE_RUNNING || state == SERVICE_STOPPED ||
        state == SERVICE_PAUSED) {
        status.dwWaitHint = 0;
    }
    else {
        status.dwWaitHint = 1000;
    }

    if (!::SetServiceStatus(m_StatusHandle, &status)) {
        Logger.Error("SetServiceStatus failed: ", WindowsErrorString(::GetLastError()));
    }
}

static void OnControlStop()
{
    Logger.Info("Service stop requested");

    UpdateServiceState(SERVICE_STOP_PENDING);

    StopCameraServer();

    UpdateServiceState(SERVICE_STOPPED);
}

static DWORD WINAPI ServiceControlHandlerEx(
    _In_ DWORD dwControl,
    _In_ DWORD /*dwEventType*/,
    _In_ LPVOID /*lpEventData*/,
    _In_ LPVOID /*lpContext*/) {
    if (dwControl == SERVICE_CONTROL_STOP ||
        dwControl == SERVICE_CONTROL_SHUTDOWN)
    {
        OnControlStop();
    }
    return NO_ERROR;
}

static void ServiceMainFunction(
    DWORD /*dwNumServicesArgs*/,
    LPSTR* /*lpServiceArgVectors*/)
{
    MessageBoxMinLevel = logger::Level::Silent;
    EnableEventLog = true;

    m_StatusHandle = ::RegisterServiceCtrlHandlerExA(
        MRCAM_SERVICE_NAME,
        ServiceControlHandlerEx,
        nullptr);

    UpdateServiceState(SERVICE_START_PENDING);

    if (!StartCameraServer()) {
        UpdateServiceState(SERVICE_STOPPED);
        return;
    }

    UpdateServiceState(SERVICE_RUNNING);
}

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    PSTR pCmdLine,
    int /*nCmdShow*/)
{
    CORE_UNUSED(hInstance);
    CORE_UNUSED(hPrevInstance);

    SetupLogging();

    MessageBoxMinLevel = logger::Level::Info;
    EnableEventLog = false;

    if (nullptr != StrIStr(pCmdLine, "/silent")) {
        MessageBoxMinLevel = logger::Level::Silent;
    }

    if (nullptr != StrIStr(pCmdLine, "/usermode")) {
        MrRunApplicationMode();
    }
    else if (nullptr != StrIStr(pCmdLine, "/install")) {
        if (!MrInstallService()) {
            return -1;
        }
    }
    else if (nullptr != StrIStr(pCmdLine, "/start")) {
        if (!MrStartService()) {
            return -1;
        }
    }
    else if (nullptr != StrIStr(pCmdLine, "/stop")) {
        if (!MrStopService()) {
            return -1;
        }
    }
    else if (nullptr != StrIStr(pCmdLine, "/uninstall")) {
        if (!MrUninstallService()) {
            return -1;
        }
    }
    else {
        SERVICE_TABLE_ENTRYA services[] = {
            {
                MRCAM_SERVICE_NAME,
                &ServiceMainFunction
            },
            {
                nullptr,
                nullptr
            }
        };

        bool success = (::StartServiceCtrlDispatcherA(services) != 0);
        if (!success) {
            Logger.Error("Pass /usermode to run in application mode");
            return -1;
        }
    }
    Logger.Debug("Terminated");
    return 0;
}
