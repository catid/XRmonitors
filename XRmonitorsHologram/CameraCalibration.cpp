// Copyright 2019 Augmented Perception Corporation

#include "stdafx.h"

#include "CameraCalibration.hpp"

#include "MonitorTools.hpp"

#include "thirdparty/INI.h"

namespace xrm {

static logger::Channel Logger("Calibration");


//------------------------------------------------------------------------------
// HeadsetCameraCalibration

void HeadsetCameraCalibration::Print()
{
    Logger.Info("Calibration:");
    Logger.Info(" * K1 = ", K1);
    Logger.Info(" * K2 = ", K2);
    Logger.Info(" * Scale = ", Scale);
    Logger.Info(" * OffsetX = ", OffsetX);
    Logger.Info(" * OffsetY = ", OffsetY);
    Logger.Info(" * RightOffsetY = ", RightOffsetY);
    Logger.Info(" * EyeCantX = ", EyeCantX);
    Logger.Info(" * EyeCantY = ", EyeCantY);
    Logger.Info(" * EyeCantZ = ", EyeCantZ);
}

bool HeadsetCameraCalibration::ReadForHeadset(std::string headset_name)
{
    bool success = false;

    const uint64_t t0 = GetTimeUsec();

    // "Samsung Windows Mixed Reality 800ZBA" -> "Samsung Windows Mixed Reality"
    if (core::StrIStr(headset_name.c_str(), "Samsung Windows Mixed Reality")) {
        headset_name = "Samsung Windows Mixed Reality";

        Logger.Debug("Samsung device: Setting name to `", headset_name, "`");
    }

    // "HP Reverb VR Headset VR1000-2xxx" -> "HP Reverb"
    if (core::StrIStr(headset_name.c_str(), "HP Reverb")) {
        headset_name = "HP Reverb";

        Logger.Debug("HP Reverb device: Setting name to `", headset_name, "`");
    }

    if (headset_name.empty()) {
        headset_name = "Unknown";
    }

    // Try reading both exe-relative and workdir-relative ini files:
    const char* camera_calibration_file = "camera_calibration.ini";
    std::string camera_calibration_path = GetFullFilePathFromRelative(camera_calibration_file);
    core::MappedReadOnlySmallFile file;
    if (file.Read(camera_calibration_path.c_str()) ||
        file.Read(camera_calibration_file))
    {
        Logger.Info("Calibration file: ", camera_calibration_path, " [size = ", file.GetDataBytes(), " bytes]");

        INI ini(file.GetData(), file.GetDataBytes(), true);
        K1 = ini.get(headset_name, "K1", K1);
        K2 = ini.get(headset_name, "K2", K2);
        Scale = ini.get(headset_name, "Scale", Scale);
        OffsetX = ini.get(headset_name, "OffsetX", OffsetX);
        OffsetY = ini.get(headset_name, "OffsetY", OffsetY);
        RightOffsetY = ini.get(headset_name, "RightOffsetY", RightOffsetY);
        EyeCantX = ini.get(headset_name, "EyeCantX", EyeCantX);
        EyeCantY = ini.get(headset_name, "EyeCantY", EyeCantY);
        EyeCantZ = ini.get(headset_name, "EyeCantZ", EyeCantZ);

        const uint64_t t1 = GetTimeUsec();
        Logger.Debug("Read camera calibration in ", (t1 - t0) / 1000.f, " msec");
    }
    else
    {
        Logger.Error("Failed to read calibration file: ", camera_calibration_path);
    }

    Print();

    return true;
}


} // namespace xrm
