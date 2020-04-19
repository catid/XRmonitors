// Copyright 2019 Augmented Perception Corporation

#pragma once

#include "core_logger.hpp"
#include "core_win32.hpp"

#include "xrm_ui_abi.hpp"

#include <mutex>
#include <atomic>

namespace xrm {

using namespace core;


//------------------------------------------------------------------------------
// Constants and Datatypes

enum KeyboardShortcuts
{
    Shortcut_Recenter,
    Shortcut_Increase,
    Shortcut_Decrease,
    Shortcut_AltIncrease,
    Shortcut_AltDecrease,
    Shortcut_PanLeft,
    Shortcut_PanRight,

    Shortcut_Count,
};

struct ShortcutTest
{
    bool WasPressed[Shortcut_Count];
};

struct KeyStates
{
    // Current state of all keys
    bool KeyDown[256];
};

struct ShortcutKeys
{
    uint8_t Vks[XRM_UI_MAX_KEYS];
    int VkCount = 0;

    // Reconfigure the VKs for the shortcut.
    // Returns true if they changed.
    bool SetVks(const uint8_t* vks, int max_count);
};

struct KeyFilter
{
    // Set to true if key is used in any shortcut key
    bool KeyUsed[256];
};

struct ShortcutInterpreter
{
    KeyFilter Filter;
    ShortcutKeys Keys;
    int MatchingKeyCount = 0;
    std::atomic<bool> WasPressed = ATOMIC_VAR_INIT(false);

    // Returns true if the shortcut was pressed since the last call
    bool Pressed()
    {
        return WasPressed.exchange(false);
    }
};


//------------------------------------------------------------------------------
// Tools

// Convert KeyboardShortcuts to string
const char* KeyboardShortcutToString(int shortcut);

// Convert virtual keycode to string
std::string VkToString(int vk);


//------------------------------------------------------------------------------
// KeyboardInput

class KeyboardInput
{
public:
    bool Initialize();
    void Shutdown();

    // Update shortcut keys
    void UpdateShortcutKeys(const XrmUiData& ui_data);

    // Check if a shortcut key was pressed, reseting its pressed state
    ShortcutTest TestShortcuts();

    // Return true to suppress key
    bool OnKeyDown(PKBDLLHOOKSTRUCT info);
    bool OnKeyUp(PKBDLLHOOKSTRUCT info);

protected:
    // If this is null then initialization failed
    HHOOK KeyboardHookHandle = nullptr;

    KeyStates States;
    KeyStates FilteredKeys;

    // Interpreters for each shortcut
    ShortcutInterpreter Shortcuts[Shortcut_Count];

    KeyFilter Filter;

    CriticalSection CSection;


    void UpdateFilters();
};


} // namespace xrm
