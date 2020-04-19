// Copyright 2019 Augmented Perception Corporation

#pragma once

#include "core_win32.hpp"

#include <winrt\Windows.Graphics.Holographic.h>

namespace xrm {


//------------------------------------------------------------------------------
// Constants

enum class Proximity
{
    HeadPresent,
    Absent,
    Undetermined
};

const char* ProximityToString(Proximity prox);


//------------------------------------------------------------------------------
// Tools

std::string GetHeadsetModel();


//------------------------------------------------------------------------------
// XrmHolographicSpace

class XrmHolographicSpace
{
public:
	bool Initialize(HWND hWnd);
	void Shutdown();

    Proximity ProximityActive();

protected:
	winrt::Windows::Graphics::Holographic::HolographicSpace m_holographicSpace = nullptr;

    std::atomic<bool> Ready = ATOMIC_VAR_INIT(false);
};


} // namespace xrm
