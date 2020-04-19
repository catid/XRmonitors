// Copyright 2019 Augmented Perception Corporation

#include "stdafx.h"
#include "resource.h"

#include "InputWindow.hpp"

#include "core.hpp"
#include "core_logger.hpp"
#include "core_win32.hpp"

namespace xrm {

using namespace core;

static logger::Channel Logger("InputWindow");


//------------------------------------------------------------------------------
// Constants

#define WM_USER_EXIT ( WM_USER + 100 )


//------------------------------------------------------------------------------
// WndProc

static LRESULT CALLBACK WndProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    if (message == WM_DESTROY)
    {
        Logger.Info("WM_DESTROY");
        PostQuitMessage(0);
        return 0;
    }

    switch (message)
    {
    case WM_ACTIVATEAPP:
    case WM_INPUT:
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MOUSEWHEEL:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
    case WM_MOUSEHOVER:
        DirectX::Mouse::ProcessMessage(message, wParam, lParam);
        break;
    }

    return ::DefWindowProcA(hWnd, message, wParam, lParam);
}


//------------------------------------------------------------------------------
// InputWindow

bool InputWindow::Initialize()
{
    Terminated = false;
    Thread = std::make_shared<std::thread>(&InputWindow::Loop, this);

    return true;
}

void InputWindow::Shutdown()
{
    ::PostMessageA(hWnd, WM_USER_EXIT, 0, 0);

    Terminated = true;
    JoinThread(Thread);
}

void InputWindow::Loop()
{
    core::SetCurrentThreadName("DWin");

    Logger.Debug("Starting window message pump");

    const HINSTANCE hInstance = (HINSTANCE)::GetModuleHandle(nullptr);

    WNDCLASSEXA wcex{};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = &WndProc;
    //wcex.cbClsExtra = 0;
    //wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAINFORM));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    //wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = DESKTOP_WINDOW_CLASS;
    //wcex.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAINFORM));

    ::RegisterClassExA(&wcex);

    // Create the window for the HolographicSpace.
    hWnd = ::CreateWindowExA(
        WS_EX_CLIENTEDGE | WS_EX_TOPMOST, // ex style
        DESKTOP_WINDOW_CLASS,
        DESKTOP_WINDOW_TITLE,
        0/*WS_VISIBLE*/, // style
        0, // x
        0, // y
        100, // width
        100, // height
        nullptr, // parent
        nullptr, // menu
        hInstance,
        nullptr); // param
    if (!hWnd) {
        Logger.Error("Failed to create window: ", WindowsErrorString(::GetLastError()));
        Terminated = true;
        return;
    }

    ::ShowWindow(hWnd, SWP_HIDEWINDOW);

#if 0
    MouseInput.SetWindow(hWnd);

    DirectX::Mouse::Mode mode = DirectX::Mouse::Mode::MODE_RELATIVE;
    MouseInput.SetMode(mode);
    bool left = false;
#endif

    Keyboard.Initialize();

    while (!Terminated)
    {
        MSG msg{};

        BOOL got = ::GetMessageA(&msg, nullptr, 0, 0);
        if (!got) {
            Logger.Error("GetMessage failed: ", WindowsErrorString(::GetLastError()));
            break;
        }

        if (msg.message == WM_USER_EXIT)
        {
            Logger.Debug("WM_USER_EXIT");
            ::DestroyWindow(hWnd);
            continue;
        }

        if (msg.message == WM_QUIT)
        {
            Logger.Debug("WM_QUIT");
            Terminated = true;
            break;
        }

        ::TranslateMessage(&msg);
        ::DispatchMessageA(&msg);

#if 0
        auto state = MouseInput.GetState();

        if (!left && state.leftButton)
        {
            Logger.Info("Click");
            Terminated = true;
        }
        left = state.leftButton;
#endif
    }

    Logger.Debug("Terminating window message pump");

    Keyboard.Shutdown();
}


} // namespace xrm
