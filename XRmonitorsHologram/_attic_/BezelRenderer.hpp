// Copyright 2019 Augmented Perception Corporation

/*
    Bezels are rendered around the outside of the monitor textures.

    When VR render textures are created, a constant-width pixel boundary is
    set aside for the bezels.  Since we want the monitors to line up perfectly,
    the boundaries for neighboring monitors overlap and should be set to full
    transparent alpha.  This has the added affect to eliminating jaggies
    between monitors.

    Secret monitor bezels are set to a different color than normal monitors.

    TBD: In the future we can indicate the cursor position on the bezels,
    and we can also make the cursor-active monitor a different color.
    For now we only need to update the bezel colors when monitors are updated.
*/

#pragma once

#include "core_logger.hpp"

#include "MonitorEnumerator.hpp"
#include "D3D11CrossAdapterDuplication.hpp"
#include "D3D11SameAdapterDuplication.hpp"

namespace xrm {

using namespace core;


//------------------------------------------------------------------------------
// BezelRenderer

enum class BezelType
{
    Top,
    Bottom,
    Left,
    Right
};

enum class BezelColor
{
    Clear,
    Black,
    Red
};

class BezelRenderer
{
public:
    // Update bezels for new monitor configuration
    void UpdateMonitor(
        D3D11DeviceContext& dc,
        MonitorEnumerator* enumerator,
        std::vector<std::shared_ptr<ID3D11DesktopDuplication>>& dupes,
        int monitor_index);

    bool TrimBezelRect(
        MonitorEnumerator* enumerator,
        unsigned monitor_index,
        RECT& r1);

    bool RenderBezel(
        D3D11DeviceContext& dc,
        const RECT& bezel_rect,
        const D3D11_TEXTURE2D_DESC& vr_tex_desc,
        ID3D11Texture2D* vr_tex,
        const RECT& monitor_coords,
        BezelType type,
        BezelColor color);
};


} // namespace xrm
