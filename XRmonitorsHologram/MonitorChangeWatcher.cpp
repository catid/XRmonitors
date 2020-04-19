// Copyright 2019 Augmented Perception Corporation

#include "stdafx.h"

#include "MonitorChangeWatcher.hpp"

namespace xrm {

static logger::Channel Logger("Watcher");


//------------------------------------------------------------------------------
// MonitorChangeWatcher

static std::atomic<bool> m_MonitorsUpdated = ATOMIC_VAR_INIT(false);
static UINT m_UxdDisplayChangeMessage = 0;
static UINT m_HotUnplugDetected = 0;
static UINT m_HotplugDetected = 0;

bool CheckMonitorsUpdated()
{
    return m_MonitorsUpdated.exchange(false);
}

void TriggerMonitorEnumeration()
{
    m_MonitorsUpdated = true;
}

static LRESULT CALLBACK
MonitorChangeWatcher_WinProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    if (message == WM_DISPLAYCHANGE) {
        // Only use this one if we cannot get the preferred messages,
        // because it fires way too early.
        if (m_UxdDisplayChangeMessage == 0) {
            m_MonitorsUpdated = true;
        }
    }
    else if (message == m_UxdDisplayChangeMessage) {
        m_MonitorsUpdated = true;
    }
    else if (message == m_HotUnplugDetected) {
        m_MonitorsUpdated = true;
    }
    else if (message == m_HotplugDetected) {
        m_MonitorsUpdated = true;
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}

void MonitorChangeWatcher::Initialize()
{
    Terminated = false;
    Thread = std::make_shared<std::thread>(&MonitorChangeWatcher::Loop, this);
}

void MonitorChangeWatcher::Shutdown()
{
    Terminated = true;

    // Not thread-safe
    if (Window) {
        ::PostMessageW(Window, WM_QUIT, 0, 0);
    }

    if (Thread) {
        if (Thread->joinable()) {
            Thread->join();
        }
        Thread = nullptr;
    }
}

void MonitorChangeWatcher::Loop()
{
    SetCurrentThreadName("MonitorChanges");

    if (!WindowStart()) {
        Logger.Error("MonitorChangeWatcher: WindowStart failed during startup");
        WindowStop();
        return;
    }

    Logger.Debug("MonitorChangeWatcher: Loop running");

    while (!Terminated) {
        MSG msg{};
        BOOL bRet = ::GetMessageW(&msg, nullptr, 0, 0);
        if (bRet == 0 || bRet == -1) {
            break;
        }

        ::TranslateMessage(&msg);
        ::DispatchMessageW(&msg);

        ::Sleep(2);
    }

    Logger.Debug("MonitorChangeWatcher: Loop closing");

    WindowStop();
}

#define WINDOW_CLASS_NAME L"MonitorChangeWatcherClass"

bool MonitorChangeWatcher::WindowStart()
{
    HINSTANCE hInstance = ::GetModuleHandleA(nullptr);

    WNDCLASSEXW wc = {
        sizeof(WNDCLASSEXW),
        0,
        MonitorChangeWatcher_WinProc,
        0,
        0,
        hInstance,
        nullptr,
        nullptr,
        0,
        nullptr,
        WINDOW_CLASS_NAME,
        nullptr
    };

    // Do not bother unregistering it
    ::RegisterClassExW(&wc);

    // In order to hook events, we need to create a window and be waiting at GetMessage().
    Window = ::CreateWindowExW(
        0,
        WINDOW_CLASS_NAME,
        0,
        WS_OVERLAPPEDWINDOW,
        0,
        0,
        0,
        0,
        HWND_DESKTOP,
        0,
        hInstance,
        nullptr);

    if (!Window) {
        Logger.Debug("CreateWindowExW failed: ", WindowsErrorString(::GetLastError()));
        return false;
    }

    m_UxdDisplayChangeMessage = ::RegisterWindowMessageA("UxdDisplayChangeMessage");
    if (!m_UxdDisplayChangeMessage) {
        Logger.Error("RegisterWindowMessageA(UxdDisplayChangeMessage) failed: ", WindowsErrorString(::GetLastError()));
    }
    m_HotUnplugDetected = ::RegisterWindowMessageA("HotUnplugDetected");
    if (!m_HotUnplugDetected) {
        Logger.Error("RegisterWindowMessageA(HotUnplugDetected) failed: ", WindowsErrorString(::GetLastError()));
    }
    m_HotplugDetected = ::RegisterWindowMessageA("HotplugDetected");
    if (!m_HotplugDetected) {
        Logger.Error("RegisterWindowMessageA(HotplugDetected) failed: ", WindowsErrorString(::GetLastError()));
    }

    return true;
}

void MonitorChangeWatcher::WindowStop()
{
    if (Window) {
        ::DestroyWindow(Window);
        Window = nullptr;
    }
}


} // namespace xrm
