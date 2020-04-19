// Copyright 2019 Augmented Perception Corporation

#include "stdafx.h"

#include "WindowsHolographic.hpp"

#include <windows.h>
#include <HolographicSpaceInterop.h>
#include <winrt/Windows.Graphics.Holographic.h>
#include <windows.graphics.holographic.h>

#include "core_logger.hpp"
#include "MonitorTools.hpp"
using namespace core;

#include <winrt/Windows.Foundation.Metadata.h>

namespace xrm {

using namespace winrt::Windows::Graphics::Holographic;

static logger::Channel Logger("Holographic");


//------------------------------------------------------------------------------
// Constants

const char* ProximityToString(Proximity prox)
{
    switch (prox)
    {
    case Proximity::Absent: return "Absent";
    case Proximity::HeadPresent: return "HeadPresent";
    case Proximity::Undetermined: return "Undetermined";
    default: break;
    }
    return "(Invalid)";
}


//------------------------------------------------------------------------------
// Tools

std::string GetHeadsetModel()
{
    bool can_get_default = winrt::Windows::Foundation::Metadata::ApiInformation::IsMethodPresent(L"Windows.Graphics.Holographic.HolographicDisplay", L"GetDefault");
    if (can_get_default)
    {
        HolographicDisplay defaultHolographicDisplay = HolographicDisplay::GetDefault();
        if (defaultHolographicDisplay)
        {
            auto display_name = defaultHolographicDisplay.DisplayName();
            std::wstring wide_display_name = display_name.c_str();
            return WideStringToUtf8String(wide_display_name);
        }
    }

    return "";
}


//------------------------------------------------------------------------------
// XrmHolographicSpace

bool XrmHolographicSpace::Initialize(HWND hWnd)
{
    // Not ready for use
    Ready = false;

    try {
        winrt::init_apartment();

        // Use WinRT factory to create the holographic space.
        winrt::com_ptr<IHolographicSpaceInterop> holographicSpaceInterop =
            winrt::get_activation_factory<HolographicSpace, IHolographicSpaceInterop>();

        winrt::com_ptr<IHolographicSpace> spHolographicSpace;
        HRESULT hr = holographicSpaceInterop->CreateForWindow(
            hWnd,
            __uuidof(ABI::Windows::Graphics::Holographic::IHolographicSpace),
            winrt::put_abi(spHolographicSpace));

        if (FAILED(hr) || !spHolographicSpace) {
            Logger.Error("Failed to create Windows Holographic space: ", HresultString(hr));
            return false;
        }

        m_holographicSpace = spHolographicSpace.as<HolographicSpace>();

        // Ready for use
        Ready = true;
        return true;
    }
    catch (std::exception& ex)
    {
        Logger.Error("Exception during holographic space initialization: ", ex.what());
    }

    return false;
}

void XrmHolographicSpace::Shutdown()
{
    m_holographicSpace = nullptr;
}

Proximity XrmHolographicSpace::ProximityActive()
{
    if (!Ready) {
        return Proximity::Undetermined;
    }

    if (m_holographicSpace.UserPresence() == HolographicSpaceUserPresence::Absent) {
        return Proximity::Absent;
    }

    return Proximity::HeadPresent;
}


} // namespace xrm
