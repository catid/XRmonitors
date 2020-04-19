// Copyright 2019 Augmented Perception Corporation

#include "stdafx.h"

#include "HolographicInputBanner.hpp"

#include "thirdparty/WinReg.hpp"

namespace xrm {

using namespace core;

static logger::Channel Logger("HolographicBanner");


//------------------------------------------------------------------------------
// Tools

bool IsWinYPromptOpen()
{
    return (FindWindowA("HolographicInputBannerWndClass", nullptr) != nullptr);
}

void DismissWinY()
{
    // Press: Win, Y

    keybd_event(
        VK_LWIN,
        MapVirtualKeyA(VK_LWIN, MAPVK_VK_TO_VSC),
        KEYEVENTF_EXTENDEDKEY,
        0);
    keybd_event(
        VkKeyScan('Y'),
        MapVirtualKeyA(VK_LWIN, MAPVK_VK_TO_VSC), // Y
        0,
        0);

    // Release: Win, Y

    keybd_event(
        VK_LWIN,
        MapVirtualKeyA(VK_LWIN, MAPVK_VK_TO_VSC),
        KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP,
        0);
    keybd_event(
        VkKeyScan('Y'),
        MapVirtualKeyA(VK_LWIN, MAPVK_VK_TO_VSC), // Y
        KEYEVENTF_KEYUP,
        0);
}

bool SetMay2019WinYDisableRegValue(bool show_win_y_prompt)
{
    try {
        winreg::RegKey key(WINREG_HOLOGRAPHIC_PARENT_KEY, WINREG_HOLOGRAPHIC_SUBKEY, KEY_WRITE);

        key.SetDwordValue(
            WINREG_IGNORE_HMD_PRESENCE,
            show_win_y_prompt ? 0 : 1);

        return true;
    }
    catch (winreg::RegException& ex) {
        Logger.Error("Error reading settings: ", ex.what(),
            " ErrorCode=", ex.ErrorCodeStr(),
            " LastErr=", ex.LastErrStr());
    }

    return false;
}


//------------------------------------------------------------------------------
// WinYAutoDismiss

void WinYAutoDismiss::Start()
{
    Terminated = false;
    Thread = std::make_shared<std::thread>(&WinYAutoDismiss::Loop, this);
}

void WinYAutoDismiss::Stop()
{
    Terminated = true;
    JoinThread(Thread);
}

void WinYAutoDismiss::Enable(bool auto_dismiss_win_y)
{
    Enabled = auto_dismiss_win_y;
}

void WinYAutoDismiss::Loop()
{
    SetCurrentThreadName("WinY");

    unsigned dismiss_countdown = 0;

    while (!Terminated)
    {
        ::Sleep(50);

        if (!Enabled) {
            continue;
        }

        // Wait a bit between actions
        if (dismiss_countdown > 0) {
            --dismiss_countdown;
            continue;
        }

        // If the input banner is up:
        if (IsWinYPromptOpen())
        {
            Logger.Info("Detected Win+Y Input Banner: Dismissing it.");

            DismissWinY();
            dismiss_countdown = 5;
        }
    }
}


} // namespace xrm
