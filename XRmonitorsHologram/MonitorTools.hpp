// Copyright 2019 Augmented Perception Corporation

#pragma once

#include "core_logger.hpp"
#include "core_string.hpp"
#include "core_win32.hpp" // Include first to adjust windows.h include
#include "core_serializer.hpp"
#include "core_mmap.hpp"

#include <DXGI.h>
#include <d3d11_4.h>
#include <wrl/client.h>

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

namespace xrm {

using namespace Microsoft::WRL;
using namespace core;


//------------------------------------------------------------------------------
// Constants

#define PI_FLOAT 3.14159265f
#define EPSILON_FLOAT 0.00001f

enum class EdidMonitorInterfaceType
{
    Unknown,
    Analog,
    HDMI,
    DisplayPort,
};


//------------------------------------------------------------------------------
// Tools

const char* MonitorInterfaceTypeString(EdidMonitorInterfaceType type);
const char* DxgiRotationString(DXGI_MODE_ROTATION rotation);

bool IsHeadlessGhost(const char* display_name);
bool IsDuetDisplay(const char* display_name);

std::string LUIDToString(const LUID& luid);
bool LUIDMatch(const LUID& x, const LUID& y);
HMONITOR GetPrimaryMonitorHandle();

unsigned AbsWidth(LONG left, LONG right);

std::string HresultString(HRESULT hr);

// Convert a Windows wide string to a UTF-8 (multi-byte) string.
std::string WideStringToUtf8String(const std::wstring& wide);

void StripNonprintables(char* s);


//------------------------------------------------------------------------------
// RectGlomp

class RectGlomp
{
public:
    void Reset();
    void Grow(const RECT& rect);

    bool Empty() const
    {
        return Count == 0;
    }

    unsigned Width() const
    {
        return GlompRect.right - GlompRect.left + 1;
    }

    unsigned Height() const
    {
        return GlompRect.bottom - GlompRect.top + 1;
    }

protected:
    RECT GlompRect;
    unsigned Count = 0;
};


} // namespace xrm
