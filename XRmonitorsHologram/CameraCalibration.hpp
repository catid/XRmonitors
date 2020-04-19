// Copyright 2019 Augmented Perception Corporation

#pragma once

#include <string>

namespace xrm {


//------------------------------------------------------------------------------
// HeadsetCameraCalibration

struct HeadsetCameraCalibration
{
    // Defaults that seem to be pretty common:
    // These worked without adjustment for the DELL Visor and the Acer headset,
    // after tuning for the HP Windows Mixed Reality Headset.
    float K1 = -0.65f;
    float K2 = 0.f;
    float Scale = 1.9f;
    float OffsetX = 0.241f;
    float OffsetY = -0.178f;
    float RightOffsetY = 0.f;
    float EyeCantX = -0.391003f;
    float EyeCantY = -0.504997f;
    float EyeCantZ = 0.012f;

    bool ReadForHeadset(std::string headset_name);

    void Print();
};


} // namespace xrm
