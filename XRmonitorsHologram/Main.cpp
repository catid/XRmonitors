// Copyright 2019 Augmented Perception Corporation

#include "stdafx.h"

#include "Main.hpp"

#include "core_logger.hpp"
#include "core_string.hpp"
#include "core_win32.hpp"
#include "core_serializer.hpp"

#include <shlwapi.h>
#include <shlobj.h>
#pragma comment(lib,"shlwapi.lib")

#include <fstream>
#include <iomanip>
#include <ctime>

namespace xrm {

using namespace core;

static logger::Channel Logger("Main");


//-----------------------------------------------------------------------------
// Logger

static std::string GetLogFilePath()
{
    char szPath[MAX_PATH];

    HRESULT hr = ::SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, szPath);

    if (FAILED(hr)) {
        Logger.Error("SHGetFolderPathA failed: ", HresultString(hr));
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

static void OpenNextLogFile()
{
    std::string log_file_path = LogFilePath;
    log_file_path += "\\XRmonitorsHologram.";
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
    ::OutputDebugStringA(formatted.c_str());

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


//------------------------------------------------------------------------------
// MainApplication

bool MainApplication::Run()
{
    SetupLogging();

    if (!Window.Initialize()) {
        Logger.Error("Failed to initialize input window");
        return false;
    }

    bool disable_win_y = false;
    XrmUiData data{};

    if (!Cameras.Start()) {
        Logger.Warning("Failed to start camera access");
    }
    else {
        if (!Cameras.ReadUiState(data)) {
            Logger.Warning("Failed to read UI state");
        } else {
            disable_win_y = data.DisableWinY != 0;

            Logger.Debug("Initial DisableWinY = ", disable_win_y);
        }
    }

    Enumerator.Initialize();

#ifdef CORE_DEBUG
    disable_win_y = true;
#endif

    WinY.Enable(disable_win_y);
    WinY.Start();

    RenderController.Initialize(
        &WinY,
        &Enumerator,
        &RenderModel,
        &CameraCalibration,
        &Cameras,
        &Settings,
        &Window);

    const bool program_init = Graphics.Initialize(
        &RenderController,
        &RenderModel,
        &CameraCalibration);

    AutoHandle parent;

    if (data.UiParentPid != 0)
    {
        parent = ::OpenProcess(SYNCHRONIZE, TRUE, data.UiParentPid);
        if (!parent.Valid()) {
            Logger.Error("UiParentPid OpenProcess failed: ", WindowsErrorString(::GetLastError()));
        }
        else {
            Logger.Info("Opened parent process");
        }
    }
    else {
        Logger.Warning("UiParentPid not specified");
    }

    if (!program_init) {
        Logger.Error("Graphics program failed to initialize: OpenXR setup must be completed first");
    }
    else for (;;) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        if (Graphics.IsTerminated()) {
            Logger.Info("Terminating on graphics failure");
            break;
        }
#if !defined(CORE_DEBUG)
        if (RenderModel.UiData.Terminate) {
            Logger.Info("Commanded to terminate by UI");
            break;
        }
#endif

        if (parent.Valid())
        {
            const DWORD wait = ::WaitForSingleObject(parent.Get(), 0);
            if (wait == WAIT_OBJECT_0)
            {
                Logger.Warning("****** Detected UI termination - Closing ******");
                break;
            }
        }
    }

    Graphics.Shutdown();
    RenderController.Shutdown();
    WinY.Stop();
    Enumerator.Shutdown();
    Cameras.Stop();
    Window.Shutdown();

    return program_init;
}


} // namespace xrm


//------------------------------------------------------------------------------
// Entrypoint

int APIENTRY wWinMain(
    _In_     HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_     LPWSTR    lpCmdLine,
    _In_     int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    core::SetCurrentThreadName("Main");

    bool program_init = xrm::MainApplication().Run();

    if (!program_init) {
        return -1; // Indicates OpenXR setup needs to be completed first
    }
    else {
        return 0;
    }
}
