// Copyright 2019 Augmented Perception Corporation

#include "stdafx.h"

#include "MonitorRenderModel.hpp"

namespace xrm {

static logger::Channel Logger("MonitorRenderModel");


//------------------------------------------------------------------------------
// MonitorRenderModel

void MonitorRenderModel::SetDpi(float dpi)
{
    Logger.Debug("Set DPI: ", dpi);

    DPI = dpi;

    MetersPerPixel = XRM_METERS_PER_INCH / dpi;
    PixelsPerMeter = dpi / XRM_METERS_PER_INCH;
}


} // namespace xrm
