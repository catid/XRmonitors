// Copyright 2019 Augmented Perception Corporation

#include "stdafx.h"

#include "ApplicationSettings.hpp"

#include "thirdparty/WinReg.hpp"
#include "MonitorTools.hpp"

namespace xrm {

using namespace core;

static logger::Channel Logger("AppSettings");


//------------------------------------------------------------------------------
// ApplicationSettings

bool ApplicationSettings::ReadSettings()
{
    // Set default values
    *this = ApplicationSettings();

    try {
        winreg::RegKey key(WINREG_PARENT_KEY, WINREG_SUBKEY, KEY_READ);

        // Read float key
        std::wstring dpi_wstr = key.GetStringValue(WINREG_VALUE_DPI);
        std::string dpi_str = WideStringToUtf8String(dpi_wstr);
        if (!dpi_str.empty()) {
            float temp = atof(dpi_str.c_str());
            if (temp != 0.f) {
                MonitorDpi = temp;
            }
        }

        return true;
    }
    catch (winreg::RegException& ex) {
        Logger.Error("Error reading settings: ", ex.what(),
            " ErrorCode=", ex.ErrorCodeStr(),
            " LastErr=", ex.LastErrStr());
    }

    return false;
}

bool ApplicationSettings::SaveSettings()
{
    try {
        winreg::RegKey key(WINREG_PARENT_KEY, WINREG_SUBKEY, KEY_WRITE);

        key.SetStringValue(WINREG_VALUE_DPI, std::to_wstring(MonitorDpi));

        return true;
    }
    catch (winreg::RegException& ex) {
        Logger.Error("Error writing settings: ", ex.what());
    }

    return false;
}


} // namespace xrm
