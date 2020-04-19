// Copyright 2019 Augmented Perception Corporation

#include "stdafx.h"

#include "MonitorTools.hpp"

#include <comdef.h>

namespace xrm {

static logger::Channel Logger("Tools");


//------------------------------------------------------------------------------
// Constants

static const char* kHeadlessGhosts[] = {
    "HDMI2K",
    "FH-DP4K",
    "FH-HD4K",
    "Mi TV"
};
static const int kHeadlessGhostsCount = CORE_ARRAY_COUNT(kHeadlessGhosts);


//------------------------------------------------------------------------------
// Tools

const char* MonitorInterfaceTypeString(EdidMonitorInterfaceType type)
{
    switch (type) {
    case EdidMonitorInterfaceType::Analog: return "Analog";
    case EdidMonitorInterfaceType::HDMI: return "HDMI";
    case EdidMonitorInterfaceType::DisplayPort: return "DisplayPort";
    default: break;
    }
    return "Unknown";
}

const char* DxgiRotationString(DXGI_MODE_ROTATION rotation)
{
    switch (rotation)
    {
    case DXGI_MODE_ROTATION_UNSPECIFIED: return "Unspecified (Not rotated?)";
    case DXGI_MODE_ROTATION_IDENTITY: return "Not rotated";
    case DXGI_MODE_ROTATION_ROTATE90: return "Rotated 90 degrees clockwise";
    case DXGI_MODE_ROTATION_ROTATE180: return "Rotated 180 degrees clockwise";
    case DXGI_MODE_ROTATION_ROTATE270: return "Rotated 270 degrees clockwise";
    default: break;
    }
    return "Probably not rotated";
}

bool IsHeadlessGhost(const char* display_name)
{
    for (int i = 0; i < kHeadlessGhostsCount; ++i) {
        if (0 == StrCaseCompare(display_name, kHeadlessGhosts[i])) {
            return true;
        }
    }
    return false;
}

bool IsDuetDisplay(const char* display_name)
{
    if (0 == StrCaseCompare(display_name, "Duet Display")) {
        return true;
    }
    return false;
}

std::string LUIDToString(const LUID& luid)
{
    std::ostringstream oss;
    oss << luid.HighPart << ':' << luid.LowPart;
    return oss.str();
}

bool LUIDMatch(const LUID& x, const LUID& y)
{
    return x.HighPart == y.HighPart && x.LowPart == y.LowPart;
}

HMONITOR GetPrimaryMonitorHandle()
{
    const POINT ptZero = { 0, 0 };
    return ::MonitorFromPoint(ptZero, MONITOR_DEFAULTTOPRIMARY);
}

unsigned AbsWidth(LONG left, LONG right)
{
    LONG delta = left - right;
    if (delta < 0) {
        delta = -delta;
    }
    return static_cast<unsigned>(delta);
}

std::string HresultString(HRESULT hr)
{
    std::ostringstream oss;
    oss << _com_error(hr).ErrorMessage() << " [hr=" << hr << "]";
    return oss.str();
}

void StripNonprintables(char* s)
{
    int len = (int)strlen(s);
    for (int i = len - 1; i >= 0; --i) {
        if (s[i] < '!' || s[i] > '~') {
            s[i] = '\0';
        }
        else {
            break;
        }
    }
}

std::string WideStringToUtf8String(const std::wstring& wide)
{
    if (wide.empty()) {
        return "";
    }

    // Calculate necessary buffer size
    int len = ::WideCharToMultiByte(
        CP_UTF8,
        0,
        wide.c_str(),
        static_cast<int>(wide.size()),
        NULL,
        0,
        NULL,
        NULL);

    // Perform actual conversion
    if (len > 0)
    {
        std::vector<char> buffer(len);

        len = ::WideCharToMultiByte(
            CP_UTF8,
            0,
            wide.c_str(),
            static_cast<int>(wide.size()),
            &buffer[0],
            static_cast<int>(buffer.size()),
            NULL,
            NULL);

        if (len > 0)
        {
            return std::string(&buffer[0], buffer.size());
        }
    }

    return "<failure>";
}


//------------------------------------------------------------------------------
// RectGlomp

void RectGlomp::Reset()
{
    Count = 0;

    GlompRect.left = 0;
    GlompRect.right = 0;
    GlompRect.top = 0;
    GlompRect.bottom = 0;
}

void RectGlomp::Grow(const RECT& rect)
{
    if (Count == 0) {
        GlompRect = rect;
        return;
    }

    if (GlompRect.left > rect.left) {
        GlompRect.left = rect.left;
    }
    if (GlompRect.right < rect.right) {
        GlompRect.right = rect.right;
    }
    if (GlompRect.top > rect.top) {
        GlompRect.top = rect.top;
    }
    if (GlompRect.bottom < rect.bottom) {
        GlompRect.bottom = rect.bottom;
    }
}


} // namespace xrm
