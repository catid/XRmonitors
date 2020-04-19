// Copyright 2019 Augmented Perception Corporation

#pragma once

#include "thirdparty/WinReg.hpp"

#include <memory>
#include <thread>
#include <atomic>

namespace xrm {


//------------------------------------------------------------------------------
// Constants

#define WINREG_PARENT_KEY   HKEY_CURRENT_USER
#define WINREG_SUBKEY       L"SOFTWARE\\XRmonitors"
#define WINREG_VALUE_DPI    L"MonitorDpi" /* float-as-string */

#define XRM_DEFAULT_DPI 50.f


//------------------------------------------------------------------------------
// ApplicationSettings

struct ApplicationSettings
{
    float MonitorDpi = XRM_DEFAULT_DPI;


    bool ReadSettings();
    bool SaveSettings();
};


} // namespace xrm
