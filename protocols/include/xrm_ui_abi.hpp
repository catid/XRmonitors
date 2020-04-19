// Copyright 2019 Augmented Perception Corporation

#ifndef XRM_UI_ABI_HPP
#define XRM_UI_ABI_HPP

#include <stdint.h>
#include <string.h>

#include <string>
#include <atomic>


//------------------------------------------------------------------------------
// Constants

#define XRM_UI_SHARED_MEMORY_NAME "Global\\XRmonitorsUI"


//------------------------------------------------------------------------------
// XRmonitors UI Shared Memory Layout

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4324)  // Padded struct
#endif
#pragma pack(push)
#pragma pack(1)

#define XRM_UI_MAX_KEYS 8

struct XrmUiData
{
    // PID to watch - If it dies the parent is dead
    uint32_t UiParentPid;

    // Automatically dismiss Win+Y prompt?
    uint8_t DisableWinY;

    // Enable pass-through cameras?
    uint8_t EnablePassthrough;

    // Enable blue-light filter?
    uint8_t EnableBlueLightFilter;

    // Kill render process?
    uint8_t Terminate;

    // Key bindings
    uint8_t RecenterKeys[XRM_UI_MAX_KEYS];
    uint8_t IncreaseKeys[XRM_UI_MAX_KEYS];
    uint8_t DecreaseKeys[XRM_UI_MAX_KEYS];
    uint8_t AltIncreaseKeys[XRM_UI_MAX_KEYS];
    uint8_t AltDecreaseKeys[XRM_UI_MAX_KEYS];
    uint8_t PanLeftKeys[XRM_UI_MAX_KEYS];
    uint8_t PanRightKeys[XRM_UI_MAX_KEYS];
};

struct XrmUiSharedMemoryLayout
{
    // Offset = 0
    std::atomic<uint32_t> BeforeWriteCounter = ATOMIC_VAR_INIT(0);
    uint8_t PaddingBefore[32 - 4];

    // Offset = 32
    std::atomic<uint32_t> AfterWriteCounter = ATOMIC_VAR_INIT(0);
    uint8_t PaddingAfter[32 - 4];

    // Offset = 64
    XrmUiData Data;

    bool Read(uint32_t& epoch, XrmUiData& data) const;
};

#pragma pack(pop)
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

static const uint32_t kXrmUiSharedMemoryBytes = static_cast<uint32_t>(sizeof(XrmUiSharedMemoryLayout));

#endif // XRM_UI_ABI_HPP
