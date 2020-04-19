// Copyright 2019 Augmented Perception Corporation

#pragma once

#include "core_win32.hpp"

#include <thread>
#include <atomic>

namespace xrm {


//------------------------------------------------------------------------------
// Tools

#define WINREG_HOLOGRAPHIC_PARENT_KEY   HKEY_CURRENT_USER
#define WINREG_HOLOGRAPHIC_SUBKEY       L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Holographic"
#define WINREG_IGNORE_HMD_PRESENCE      L"IgnoreHMDPresence" /* DWORD */


//------------------------------------------------------------------------------
// Tools

bool IsWinYPromptOpen();

void DismissWinY();

/*
    In the May 2019 Update for Windows 10, Microsoft added a Win+Y disable option.
    It can be programmatically modified too:

    HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Holographic
    DWORD Value: IgnoreHMDPresence
    Value 0 = Show Win+Y Prompt
    Value 1 = Do NOT show Win+Y Prompt

    Returns true if the value could be set, and false otherwise.
*/
bool SetMay2019WinYDisableRegValue(bool show_win_y_prompt);


//------------------------------------------------------------------------------
// WinYAutoDismiss

/*
    Windows Holographic will sometimes show a Win+Y Input Banner at the top of
    the screen and steal the keyboard/mouse away from the user's desktop.
    This class can detect when the banner is being show and automatically press
    the Win+Y keystroke to dismiss the banner, effectively keeping it closed.
*/
class WinYAutoDismiss
{
public:
    ~WinYAutoDismiss()
    {
        Stop();
    }

    void Start();
    void Stop();

    // This can be used to keep running the background thread but enable/disable
    // the behavior to auto-dismiss.
    void Enable(bool auto_dismiss_win_y);

protected:
    std::shared_ptr<std::thread> Thread;
    std::atomic<bool> Terminated = ATOMIC_VAR_INIT(false);

    std::atomic<bool> Enabled = ATOMIC_VAR_INIT(true);

    void Loop();
};


} // namespace xrm
