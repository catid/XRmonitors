// Copyright 2019 Augmented Perception Corporation

/*
    For whatever reason, Windows seems to require there to be a window in our
    application in order to hook keyboard input, otherwise the keyboard hook
    hangs unexpectedly.
*/

#pragma once

#include "core_win32.hpp"

#include "KeyboardInput.hpp"

#include <memory>
#include <thread>
#include <atomic>

#include "thirdparty/DirectXTK/Inc/Mouse.h"

namespace xrm {


//------------------------------------------------------------------------------
// Constants

#define DESKTOP_WINDOW_CLASS "XRmonitorsClass"
#define DESKTOP_WINDOW_TITLE "XRmonitorsInput"


//------------------------------------------------------------------------------
// InputWindow

class InputWindow
{
public:
    bool Initialize();
    void Shutdown();

    bool IsTerminated() const
    {
        return Terminated;
    }

    HWND GetHandle()
    {
        return hWnd;
    }

    KeyboardInput* GetKeyboard()
    {
        return &Keyboard;
    }

protected:
    HWND hWnd = 0;

    std::shared_ptr<std::thread> Thread;
    std::atomic<bool> Terminated = ATOMIC_VAR_INIT(false);

    KeyboardInput Keyboard;


    void Loop();
};


} // namespace xrm
