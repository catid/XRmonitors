// Copyright 2019 Augmented Perception Corporation

#include "stdafx.h"

#include "KeyboardInput.hpp"
#include "MonitorTools.hpp"

namespace xrm {

static logger::Channel Logger("KeyboardInput");


//------------------------------------------------------------------------------
// Constants

static const uint8_t kRecenterVks[2] = {
    VK_LWIN,
    VK_SPACE
};
static const uint8_t kIncreaseVks[2] = {
    VK_LWIN,
    VK_NEXT
};
static const uint8_t kDecreaseVks[2] = {
    VK_LWIN,
    VK_PRIOR
};
static const uint8_t kAltIncreaseVks[3] = {
    VK_LWIN,
    VK_MENU,
    VK_UP
};
static const uint8_t kAltDecreaseVks[3] = {
    VK_LWIN,
    VK_MENU,
    VK_DOWN
};
static const uint8_t kPanLeftVks[3] = {
    VK_LWIN,
    VK_MENU,
    VK_LEFT
};
static const uint8_t kPanRightVks[3] = {
    VK_LWIN,
    VK_MENU,
    VK_RIGHT
};

const char* KeyboardShortcutToString(int shortcut)
{
    static_assert(Shortcut_Count == 7, "FIXME");
    switch (shortcut)
    {
    case Shortcut_Recenter: return "Recenter";
    case Shortcut_Increase: return "Increase";
    case Shortcut_Decrease: return "Decrease";
    case Shortcut_AltIncrease: return "AltIncrease";
    case Shortcut_AltDecrease: return "AltDecrease";
    case Shortcut_PanLeft: return "PanLeft";
    case Shortcut_PanRight: return "PanRight";
    default:
        break;
    }
    CORE_DEBUG_BREAK(); // Invalid input
    return "Unknown";
}

std::string VkToString(int vk)
{
    UINT vsc = 0;
    switch (vk)
    {
    case VK_LWIN: return "Left Win Key";
    case VK_RWIN: return "Right Win Key";
    case VK_LEFT: return "Left";
    case VK_UP: return "Up";
    case VK_RIGHT: return "Right";
    case VK_DOWN: return "Down";
    case VK_CONTROL: return "Ctrl";
    case VK_LCONTROL: return "Left Ctrl";
    case VK_RCONTROL: return "Right Ctrl";
    case VK_MENU: return "Alt";
    case VK_LMENU: return "Left Alt";
    case VK_RMENU: return "Right Alt";
    case VK_APPS: return "Win Key"; // This application munges Windows -> App key
    case VK_PRIOR: return "Page Up";
    case VK_NEXT: return "Page Down";
    case VK_END: return "End";
    case VK_HOME: return "Home";
    case VK_INSERT: return "Insert";
    case VK_DELETE: return "Delete";
    case VK_DIVIDE: return "Divide";
    case VK_NUMLOCK: return "NumLock";
        // Can get GetKeyNameTextA to work if we set this flag
        // but might as well just do the conversion ourselves in this case..
        //vsc |= KF_EXTENDED;
        //break;
    default:
        break;
    };

    vsc |= ::MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);
    vsc <<= 16; // Bit position required for API

    char str[64];
    int len = ::GetKeyNameTextA(vsc, str, sizeof(str));
    if (len <= 0) {
        Logger.Error("GetKeyNameTextA failed: ", WindowsErrorString(::GetLastError()));
        return "(badkey)";
    }
    return str;
}


//------------------------------------------------------------------------------
// Tools

static uint8_t RemapSuperKeys(uint8_t vk)
{
    if (vk == VK_LMENU || vk == VK_RMENU) {
        return VK_MENU;
    }
    if (vk == VK_LWIN || vk == VK_RWIN) {
        return VK_APPS;
    }
    if (vk == VK_LCONTROL || vk == VK_RCONTROL) {
        return VK_CONTROL;
    }
    if (vk == VK_LSHIFT || vk == VK_RSHIFT) {
        return VK_SHIFT;
    }
    return vk;
}


//------------------------------------------------------------------------------
// ShortcutKeys

bool ShortcutKeys::SetVks(const uint8_t* vks, int max_count)
{
    const int old_count = VkCount;
    bool changed = false;

    CORE_DEBUG_ASSERT(max_count <= XRM_UI_MAX_KEYS);
    int i;
    for (i = 0; i < max_count; ++i) {
        if (vks[i] == 0) {
            // If there were no vks speciifed:
            if (i == 0) {
                // Do not change anything
                return false;
            }
            break;
        }

        const uint8_t vk = RemapSuperKeys(vks[i]);

        if (Vks[i] != vk) {
            changed = true;
        }
        Vks[i] = vk;
    }
    VkCount = i;

    if (VkCount != old_count) {
        return true;
    }

    return changed;
}


//------------------------------------------------------------------------------
// KeyboardInput

// FIXME: How do I pass a reference to this?
static KeyboardInput* m_KeyboardObject = nullptr;

// Note: This is called before GetAsyncKeyState() is updated.
static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    bool suppress = false;

    if (nCode == HC_ACTION)
    {
        switch (wParam)
        {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            suppress = m_KeyboardObject->OnKeyDown((PKBDLLHOOKSTRUCT)lParam);
            break;
        case WM_KEYUP:
        case WM_SYSKEYUP:
            suppress = m_KeyboardObject->OnKeyUp((PKBDLLHOOKSTRUCT)lParam);
            break;
        }
    }

    if (suppress) {
        return TRUE;
    }

    return ::CallNextHookEx(nullptr, nCode, wParam, lParam);
}

bool KeyboardInput::Initialize()
{
    memset(&States, 0, sizeof(States));
    memset(&FilteredKeys, 0, sizeof(FilteredKeys));
    memset(&Shortcuts, 0, sizeof(Shortcuts));
    m_KeyboardObject = this;

    // Set default keys
    Shortcuts[Shortcut_Recenter].Keys.SetVks(kRecenterVks, CORE_ARRAY_COUNT(kRecenterVks));
    Shortcuts[Shortcut_Increase].Keys.SetVks(kIncreaseVks, CORE_ARRAY_COUNT(kIncreaseVks));
    Shortcuts[Shortcut_Decrease].Keys.SetVks(kDecreaseVks, CORE_ARRAY_COUNT(kDecreaseVks));
    Shortcuts[Shortcut_AltIncrease].Keys.SetVks(kAltIncreaseVks, CORE_ARRAY_COUNT(kAltIncreaseVks));
    Shortcuts[Shortcut_AltDecrease].Keys.SetVks(kAltDecreaseVks, CORE_ARRAY_COUNT(kAltDecreaseVks));
    Shortcuts[Shortcut_PanLeft].Keys.SetVks(kPanLeftVks, CORE_ARRAY_COUNT(kPanLeftVks));
    Shortcuts[Shortcut_PanRight].Keys.SetVks(kPanRightVks, CORE_ARRAY_COUNT(kPanRightVks));
    UpdateFilters();

    KeyboardHookHandle = ::SetWindowsHookExA(
        WH_KEYBOARD_LL,
        LowLevelKeyboardProc,
        0, // hModule
        0); // dwThreadId

    if (!KeyboardHookHandle) {
        Logger.Error("SetWindowsHookExA failed: ", WindowsErrorString(::GetLastError()));
        return false;
    }

    return true;
}

void KeyboardInput::Shutdown()
{
    if (KeyboardHookHandle)
    {
        ::UnhookWindowsHookEx(KeyboardHookHandle);
        KeyboardHookHandle = nullptr;
    }
}

void KeyboardInput::UpdateFilters()
{
    memset(&Filter, 0, sizeof(Filter));

    for (int i = 0; i < Shortcut_Count; ++i)
    {
        ShortcutInterpreter& shortcut = Shortcuts[i];

        memset(&shortcut.Filter, 0, sizeof(shortcut.Filter));

        std::ostringstream oss;
        oss << KeyboardShortcutToString(i) << ":";

        const int count = shortcut.Keys.VkCount;
        if (count == 0) {
            oss << "(none)";
        }

        for (int j = 0; j < count; ++j)
        {
            const uint8_t vk = shortcut.Keys.Vks[j];

            if (j > 0) {
                oss << " +";
            }
            oss << " " << VkToString(vk);

            switch (vk)
            {
            case VK_LMENU:
            case VK_RMENU:
            case VK_MENU:
                Filter.KeyUsed[VK_LMENU] = true;
                shortcut.Filter.KeyUsed[VK_LMENU] = true;
                Filter.KeyUsed[VK_RMENU] = true;
                shortcut.Filter.KeyUsed[VK_RMENU] = true;
                Filter.KeyUsed[VK_MENU] = true;
                shortcut.Filter.KeyUsed[VK_MENU] = true;
                break;
            case VK_LWIN:
            case VK_RWIN:
            case VK_APPS:
                Filter.KeyUsed[VK_LWIN] = true;
                shortcut.Filter.KeyUsed[VK_LWIN] = true;
                Filter.KeyUsed[VK_RWIN] = true;
                shortcut.Filter.KeyUsed[VK_RWIN] = true;
                Filter.KeyUsed[VK_APPS] = true;
                shortcut.Filter.KeyUsed[VK_APPS] = true;
                break;
            case VK_LCONTROL:
            case VK_RCONTROL:
            case VK_CONTROL:
                Filter.KeyUsed[VK_LCONTROL] = true;
                shortcut.Filter.KeyUsed[VK_LCONTROL] = true;
                Filter.KeyUsed[VK_RCONTROL] = true;
                shortcut.Filter.KeyUsed[VK_RCONTROL] = true;
                Filter.KeyUsed[VK_CONTROL] = true;
                shortcut.Filter.KeyUsed[VK_CONTROL] = true;
                break;
            case VK_LSHIFT:
            case VK_RSHIFT:
            case VK_SHIFT:
                Filter.KeyUsed[VK_LSHIFT] = true;
                shortcut.Filter.KeyUsed[VK_LSHIFT] = true;
                Filter.KeyUsed[VK_RSHIFT] = true;
                shortcut.Filter.KeyUsed[VK_RSHIFT] = true;
                Filter.KeyUsed[VK_SHIFT] = true;
                shortcut.Filter.KeyUsed[VK_SHIFT] = true;
                break;
            default:
                Filter.KeyUsed[vk] = true;
                shortcut.Filter.KeyUsed[vk] = true;
                break;
            }
        } // end for

        Logger.Info("Shortcut ", oss.str());
    } // end for
}

void KeyboardInput::UpdateShortcutKeys(const XrmUiData& ui_data)
{
    bool updated = false;

    CriticalLocker locker(CSection);

    updated |= Shortcuts[Shortcut_Recenter].Keys.SetVks(ui_data.RecenterKeys, XRM_UI_MAX_KEYS);
    updated |= Shortcuts[Shortcut_Increase].Keys.SetVks(ui_data.IncreaseKeys, XRM_UI_MAX_KEYS);
    updated |= Shortcuts[Shortcut_Decrease].Keys.SetVks(ui_data.DecreaseKeys, XRM_UI_MAX_KEYS);
    updated |= Shortcuts[Shortcut_AltIncrease].Keys.SetVks(ui_data.AltIncreaseKeys, XRM_UI_MAX_KEYS);
    updated |= Shortcuts[Shortcut_AltDecrease].Keys.SetVks(ui_data.AltDecreaseKeys, XRM_UI_MAX_KEYS);
    updated |= Shortcuts[Shortcut_PanLeft].Keys.SetVks(ui_data.PanLeftKeys, XRM_UI_MAX_KEYS);
    updated |= Shortcuts[Shortcut_PanRight].Keys.SetVks(ui_data.PanRightKeys, XRM_UI_MAX_KEYS);

    if (updated) {
        UpdateFilters();
    }
}

ShortcutTest KeyboardInput::TestShortcuts()
{
    ShortcutTest test;
    for (int i = 0; i < Shortcut_Count; ++i) {
        test.WasPressed[i] = Shortcuts[i].Pressed();
    }
    return test;
}

bool KeyboardInput::OnKeyDown(PKBDLLHOOKSTRUCT info)
{
    if (info->vkCode >= 256) {
        return false;
    }
    const uint8_t vk = (uint8_t)info->vkCode;

    if (States.KeyDown[vk]) {
        return false;
    }

    // Apply global filter
    if (!Filter.KeyUsed[vk]) {
        States.KeyDown[vk] = true; // Still remember presses
        return false;
    }

    // If any of the superkeys are down, they're considered pressed

    bool is_superkey = false;

    switch (vk)
    {
    case VK_LMENU:
    case VK_RMENU:
    case VK_MENU:
        if (States.KeyDown[VK_LMENU] ||
            States.KeyDown[VK_RMENU] ||
            States.KeyDown[VK_MENU])
        {
            States.KeyDown[vk] = true; // Still remember presses
            return false; // One of the other keys is already down
        }
        is_superkey = true;
        break;
    case VK_LWIN:
    case VK_RWIN:
    case VK_APPS:
        if (States.KeyDown[VK_LWIN] ||
            States.KeyDown[VK_RWIN] ||
            States.KeyDown[VK_APPS])
        {
            States.KeyDown[vk] = true; // Still remember presses
            return false; // One of the other keys is already down
        }
        is_superkey = true;
        break;
    case VK_LCONTROL:
    case VK_RCONTROL:
    case VK_CONTROL:
        if (States.KeyDown[VK_LCONTROL] ||
            States.KeyDown[VK_RCONTROL] ||
            States.KeyDown[VK_CONTROL])
        {
            States.KeyDown[vk] = true; // Still remember presses
            return false; // One of the other keys is already down
        }
        is_superkey = true;
        break;
    case VK_LSHIFT:
    case VK_RSHIFT:
    case VK_SHIFT:
        if (States.KeyDown[VK_LSHIFT] ||
            States.KeyDown[VK_RSHIFT] ||
            States.KeyDown[VK_SHIFT])
        {
            States.KeyDown[vk] = true; // Still remember presses
            return false; // One of the other keys is still down
        }
        is_superkey = true;
        break;
    default:
        break;
    }

    States.KeyDown[vk] = true;

    bool suppressed = false;

    CriticalLocker locker(CSection);

    for (int i = 0; i < Shortcut_Count; ++i)
    {
        ShortcutInterpreter& shortcut = Shortcuts[i];

        // Apply shortcut filter
        if (!shortcut.Filter.KeyUsed[vk]) {
            continue;
        }

        int matches = ++shortcut.MatchingKeyCount;
        if (matches >= shortcut.Keys.VkCount) {
            CORE_DEBUG_ASSERT(matches == shortcut.Keys.VkCount);
            shortcut.WasPressed = true; // Now active

            // Do not suppress superkeys because this causes issues
            if (!is_superkey) {
                FilteredKeys.KeyDown[vk] = true;
                suppressed = true; // Suppress the final key
            }
        }
    }

    return suppressed;
}

bool KeyboardInput::OnKeyUp(PKBDLLHOOKSTRUCT info)
{
    if (info->vkCode >= 256) {
        return false;
    }
    const uint8_t vk = (uint8_t)info->vkCode;

    if (!States.KeyDown[vk]) {
        return false;
    }
    States.KeyDown[vk] = false;

    // Apply global filter
    if (!Filter.KeyUsed[vk]) {
        return false;
    }

    // If the keydown was filtered:
    bool suppressed = false;
    if (FilteredKeys.KeyDown[vk]) {
        FilteredKeys.KeyDown[vk] = false;
        suppressed = true; // Filter keyup also
    }

    // Superkeys are only considered released if all the keys are released

    switch (vk)
    {
    case VK_LMENU:
    case VK_RMENU:
    case VK_MENU:
        if (States.KeyDown[VK_LMENU] ||
            States.KeyDown[VK_RMENU] ||
            States.KeyDown[VK_MENU])
        {
            return suppressed; // One of the other keys is still down
        }
        break;
    case VK_LWIN:
    case VK_RWIN:
    case VK_APPS:
        if (States.KeyDown[VK_LWIN] ||
            States.KeyDown[VK_RWIN] ||
            States.KeyDown[VK_APPS])
        {
            return suppressed; // One of the other keys is still down
        }
        break;
    case VK_LCONTROL:
    case VK_RCONTROL:
    case VK_CONTROL:
        if (States.KeyDown[VK_LCONTROL] ||
            States.KeyDown[VK_RCONTROL] ||
            States.KeyDown[VK_CONTROL])
        {
            return suppressed; // One of the other keys is still down
        }
        break;
    case VK_LSHIFT:
    case VK_RSHIFT:
    case VK_SHIFT:
        if (States.KeyDown[VK_LSHIFT] ||
            States.KeyDown[VK_RSHIFT] ||
            States.KeyDown[VK_SHIFT])
        {
            return suppressed; // One of the other keys is still down
        }
        break;
    default:
        break;
    }

    CriticalLocker locker(CSection);

    for (int i = 0; i < Shortcut_Count; ++i)
    {
        ShortcutInterpreter& shortcut = Shortcuts[i];

        // Apply shortcut filter
        if (!shortcut.Filter.KeyUsed[vk]) {
            continue;
        }

        const int matches = --shortcut.MatchingKeyCount;
        if (matches <= 0) {
            CORE_DEBUG_ASSERT(matches == 0);
            shortcut.MatchingKeyCount = 0; // Cannot go negative
        }
    }

    return suppressed;
}


} // namespace xrm
