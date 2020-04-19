// Copyright 2019 Augmented Perception Corporation

#include "stdafx.h"

#include "MonitorRenderController.hpp"

#include "VertexTypes.h"

namespace xrm {

using namespace DirectX;

static logger::Channel Logger("MonitorRender");


//------------------------------------------------------------------------------
// Tools

static bool FindQuadraticRoots(float a, float b, float c, float& root1, float& root2)
{
    const float determinant = b * b - 4.f * a * c;
    if (determinant > EPSILON_FLOAT)
    {
        // Solution exists
        const float inv2a = 1.f / (2.f * a);
        const float sqrtd = sqrtf(determinant);
        root1 = (-b + sqrtd) * inv2a;
        root2 = (-b - sqrtd) * inv2a;
        return true;
    }
    if (determinant > -EPSILON_FLOAT) {
        // Tangent
        root2 = root1 = -b / (2.f * a);
        return true;
    }
    return false;
}

static void ApplyTransform(
    VertexPositionTexture& vertex,
    const Matrix& transform)
{
    Vector3 p(vertex.position.x, vertex.position.y, vertex.position.z);
    Vector3 q = Vector3::Transform(p, transform);
    vertex.position.x = q.x;
    vertex.position.y = q.y;
    vertex.position.z = q.z;
}


//------------------------------------------------------------------------------
// Datatypes

void SortedMonitor::SetFromMonitorEnumInfo(const MonitorEnumInfo& info)
{
    MonitorIndex = info.MonitorIndex;
    ScreenCenterX = (info.Coords.left + info.Coords.right + 1) / 2;
    ScreenCenterY = (info.Coords.top + info.Coords.bottom + 1) / 2;
}


//------------------------------------------------------------------------------
// MonitorRenderController

void MonitorRenderController::Initialize(
    WinYAutoDismiss* win_y,
    MonitorEnumerator* enumerator,
    MonitorRenderModel* render_model,
    HeadsetCameraCalibration* camera_calibration,
    CameraClient* cameras,
    ApplicationSettings* settings,
    InputWindow* input_window)
{
    WinY = win_y;
    Enumerator = enumerator;
    RenderModel = render_model;
    CameraCalibration = camera_calibration;
    Cameras = cameras;
    Settings = settings;
    Window = input_window;
    Keyboard = input_window->GetKeyboard();
}

void MonitorRenderController::Shutdown()
{
    CleanupDuplicates();
}

void MonitorRenderController::StartRendering(
    CameraImager* imager,
    XrComputerProperties* computer,
    XrHeadsetProperties* headset,
    XrRenderProperties* rendering,
    PluginManager* plugins)
{
    // Note: This is called once when rendering starts

    Imager = imager;
    Computer = computer;
    Headset = headset;
    Rendering = rendering;
    Plugins = plugins;

    // Tell service to appl USB hub power fix
    Cameras->ApplyUsbHubPowerFix();

    if (!CameraCalibration->ReadForHeadset(Headset->HeadsetModel)) {
        Logger.Error("Error reading headset calibration for HeadsetName: `", Headset->HeadsetModel, "`");
    }

    if (!Settings->ReadSettings()) {
        Logger.Warning("Failed to restore monitor DPI settings");
    }

    RenderModel->SetDpi(Settings->MonitorDpi);

    RecenterOnFirstEnum = true;
    UpdateMonitorEnumeration();
    if (!SortedMonitors.empty()) {
        OnRecenter();
        RecenterOnFirstEnum = false;
    }
    else {
        // Defer initial recenter until we enumerate some monitors
        RecenterOnFirstEnum = true;
    }
}

void MonitorRenderController::UpdateModel()
{
    // Note: This is called before any render operations each frame

    // Read UI settings:

    if (Cameras->ReadUiState(RenderModel->UiData))
    {
        if (RenderModel->UiData.Terminate) {
            return;
        }

        const bool auto_dismiss_win_y = RenderModel->UiData.DisableWinY != 0;
        WinY->Enable(auto_dismiss_win_y);
    }

    // Read latest camera frame:

    // TBD: NowOrientation
    RenderModel->UpdatedCamerasThisFrame = Imager->AcquireImage(
        Rendering->DeviceContext,
        Cameras);
    RenderModel->ExposureTimeUsec = Imager->ExposureTimeUsec;

    // Handle keystrokes:

    HandleKeystrokes();

    // Update monitor enumeration:

    if (Enumerator->UpdateEnumeration()) {
        UpdateMonitorEnumeration();

        // Recenter on the first enumeration success
        if (RecenterOnFirstEnum) {
            RecenterOnFirstEnum = false;
            OnRecenter();
        }
    }

    // Update desktop duplication:

    UpdateDesktopDuplication();

    // Update monitor model for quads:

    //UpdateMonitorModel();

    // Update gaze coordinates

    UpdateGaze();
}

void MonitorRenderController::HandleKeystrokes()
{
    // Detect keystrokes:

    Keyboard->UpdateShortcutKeys(RenderModel->UiData);

    ShortcutTest test = Keyboard->TestShortcuts();

    if (test.WasPressed[Shortcut_Recenter]) {
        OnRecenter();
    }
    if (test.WasPressed[Shortcut_Increase] || test.WasPressed[Shortcut_AltIncrease]) {
        OnIncrease();
    }
    if (test.WasPressed[Shortcut_Decrease] || test.WasPressed[Shortcut_AltDecrease]) {
        OnDecrease();
    }
    if (test.WasPressed[Shortcut_PanLeft]) {
        OnPanLeft();
    }
    if (test.WasPressed[Shortcut_PanRight]) {
        OnPanRight();
    }
}

void MonitorRenderController::SetCenteredMonitorForMouse()
{
    CURSORINFO ci;
    ci.cbSize = sizeof(ci);
    ::GetCursorInfo(&ci);
    const int x = ci.ptScreenPos.x;
    const int y = ci.ptScreenPos.y;

    for (auto& monitor : Enumerator->Monitors)
    {
        RECT coords = monitor->Coords;

        // If the cursor is on this monitor:
        if (x >= coords.left &&
            x < coords.right &&
            y >= coords.top &&
            y < coords.bottom)
        {
            CenteredMonitorIndex = monitor->MonitorIndex;
            return;
        }
    }
}

void MonitorRenderController::OnRecenter()
{
    Logger.Info("Keystroke: Recenter");

    RenderModel->RecenterPosition = Rendering->HeadPosition;
    RenderModel->RecenterOrientation = Rendering->HeadOrientation;

    SetCenteredMonitorForMouse();
    RenderModel->ScreenFocusCenterX = SortedMonitors[CenteredMonitorIndex].ScreenCenterX;
    RenderModel->ScreenFocusCenterY = SortedMonitors[CenteredMonitorIndex].ScreenCenterY;

    // Update pitch/yaw
    UpdatePitchYaw();

    // Update desktop positions
    SolveDesktopPositions();
}

void MonitorRenderController::OnIncrease()
{
    Logger.Info("Keystroke: Increase");

    //RenderModel->RecenterPosition = Rendering->HeadPosition;
    //RenderModel->RecenterOrientation = Rendering->HeadOrientation;

    float dpi = RenderModel->DPI;
    dpi /= 1.1f;
    if (dpi < XRM_MIN_DPI) {
        dpi = XRM_MIN_DPI;
    }
    RenderModel->SetDpi(dpi);

    Settings->MonitorDpi = dpi;
    Settings->SaveSettings();

    // Do not update pitch/yaw and do not recenter

    // Update desktop positions
    SolveDesktopPositions();
}

void MonitorRenderController::OnDecrease()
{
    Logger.Info("Keystroke: Decrease");

    //RenderModel->RecenterPosition = Rendering->HeadPosition;
    //RenderModel->RecenterOrientation = Rendering->HeadOrientation;

    float dpi = RenderModel->DPI;
    dpi *= 1.1f;
    if (dpi > XRM_MAX_DPI) {
        dpi = XRM_MAX_DPI;
    }
    RenderModel->SetDpi(dpi);

    Settings->MonitorDpi = dpi;
    Settings->SaveSettings();

    // Do not update pitch/yaw and do not recenter

    // Update desktop positions
    SolveDesktopPositions();
}

void MonitorRenderController::OnPanLeft()
{
    Logger.Info("Keystroke: PanLeft");

    if (CenteredMonitorIndex <= 0) {
        return;
    }

    //RenderModel->RecenterPosition = Rendering->HeadPosition;
    //RenderModel->RecenterOrientation = Rendering->HeadOrientation;

    --CenteredMonitorIndex;
    RenderModel->ScreenFocusCenterX = SortedMonitors[CenteredMonitorIndex].ScreenCenterX;
    RenderModel->ScreenFocusCenterY = SortedMonitors[CenteredMonitorIndex].ScreenCenterY;

    // Note: We must call this twice to set the cursor to a new monitor
    ::SetCursorPos(RenderModel->ScreenFocusCenterX, RenderModel->ScreenFocusCenterY);
    ::SetCursorPos(RenderModel->ScreenFocusCenterX, RenderModel->ScreenFocusCenterY);

    Logger.Info("ScreenFocusCenterX = ", RenderModel->ScreenFocusCenterX);
    Logger.Info("ScreenFocusCenterY = ", RenderModel->ScreenFocusCenterY);

    // Do not update pitch/yaw

    // Update desktop positions
    SolveDesktopPositions();
}

void MonitorRenderController::OnPanRight()
{
    Logger.Info("Keystroke: PanRight");

    if (CenteredMonitorIndex + 1 >= (unsigned)SortedMonitors.size()) {
        return;
    }

    //RenderModel->RecenterPosition = Rendering->HeadPosition;
    //RenderModel->RecenterOrientation = Rendering->HeadOrientation;

    ++CenteredMonitorIndex;
    RenderModel->ScreenFocusCenterX = SortedMonitors[CenteredMonitorIndex].ScreenCenterX;
    RenderModel->ScreenFocusCenterY = SortedMonitors[CenteredMonitorIndex].ScreenCenterY;

    // Note: We must call this twice to set the cursor to a new monitor
    ::SetCursorPos(RenderModel->ScreenFocusCenterX, RenderModel->ScreenFocusCenterY);
    ::SetCursorPos(RenderModel->ScreenFocusCenterX, RenderModel->ScreenFocusCenterY);

    Logger.Info("ScreenFocusCenterX = ", RenderModel->ScreenFocusCenterX);
    Logger.Info("ScreenFocusCenterY = ", RenderModel->ScreenFocusCenterY);

    // Do not update pitch/yaw

    // Update desktop positions
    SolveDesktopPositions();
}

void MonitorRenderController::UpdateMonitorModel()
{
#if 0
    const unsigned count = (unsigned)Enumerator->Monitors.size();
    for (unsigned i = 0; i < count; ++i)
    {
        auto& desc = RenderModel->Monitors[i]->Dupe->VrRenderTextureDesc;

        float width_m = RenderModel->MetersPerPixel * desc.Width;
        float height_m = RenderModel->MetersPerPixel * desc.Height;

        RenderModel->Monitors[i]->Size.x = width_m;
        RenderModel->Monitors[i]->Size.y = height_m;

        // FIXME: How do we calculate this based on head position?
        RenderModel->Monitors[i]->Orientation = RenderModel->RecenterOrientation;
        RenderModel->Monitors[i]->Position.x = width_m * i - width_m / 2.f;
        RenderModel->Monitors[i]->Position.y = 0;
        RenderModel->Monitors[i]->Position.z = -RenderModel->HmdFocalDistanceMeters;
    }
#endif
}


//------------------------------------------------------------------------------
// MonitorRenderController : Enumeration

void MonitorRenderController::UpdateMonitorEnumeration()
{
    Logger.Info("Updating monitor enumeration");

    CleanupDuplicates();

    const unsigned count = (unsigned)Enumerator->Monitors.size();
    if (count == 0) {
        Duplicates.clear();
        RenderModel->Monitors.clear();
        return;
    }

    Duplicates.resize(count);
    RenderModel->Monitors.resize(count);
    SortedMonitors.resize(count);

    Logger.Info("Starting duplication");

    for (unsigned i = 0; i < count; ++i)
    {
        Enumerator->Monitors[i]->LogInfo();

        RenderModel->Monitors[i] = std::make_shared<MonitorRenderState>();

        if (LUIDMatch(Enumerator->Monitors[i]->AdapterLuid, Headset->AdapterLuid)) {
            Duplicates[i] = std::make_shared<D3D11SameAdapterDuplication>();
        }
        else {
            Duplicates[i] = std::make_shared<D3D11CrossAdapterDuplication>();
        }

        RenderModel->Monitors[i]->Dupe = Duplicates[i].get();
        RenderModel->Monitors[i]->MonitorInfo = Enumerator->Monitors[i].get();

        Duplicates[i]->Initialize(
            Enumerator->Monitors[i],
            &Rendering->DeviceContext);

        SortedMonitors[i].SetFromMonitorEnumInfo(*Enumerator->Monitors[i]);
    }

    std::sort(SortedMonitors.begin(), SortedMonitors.end());

    // Update desktop positions
    SolveDesktopPositions();
}

void MonitorRenderController::CleanupDuplicates()
{
    for (auto& dupe : Duplicates) {
        dupe->StartShutdown();
    }

    for (auto& dupe : Duplicates) {
        dupe->Shutdown();
    }

    Duplicates.clear();
}

void MonitorRenderController::UpdateDesktopDuplication()
{
    const uint64_t t0 = GetTimeUsec();

    const int count = Enumerator->Monitors.size();

    bool needs_restart = false;

    for (int i = 0; i < count; ++i) {
        Duplicates[i]->AcquireVrRenderTexture();

        if (Duplicates[i]->IsTerminated()) {
            needs_restart = true;
        }
    }

    const uint64_t t1 = GetTimeUsec();
    const uint64_t update_usec = t1 - t0;

    if (update_usec > 5000) {
        Logger.Warning("Slow duplication: Blit took ", (t1 - t0), " usec");
    }

    if (needs_restart) {
        Logger.Error("Restarting monitor enumeration on duplication failure");
        UpdateMonitorEnumeration();
    }
}

void MonitorRenderController::UpdatePitchYaw()
{
    // Calculate pitch:

    Vector3 neutral(0.f, 0.f, 1.f);
    Vector3 oriented = Vector3::Transform(neutral, RenderModel->RecenterOrientation);

    // Calculate yaw rotation matrix
    const float recenter_yaw = -atan2f(oriented.x, oriented.z);
    const float yaw_offset = PI_FLOAT * 0.5f;
    RenderModel->PoseTransform = Matrix::CreateRotationY(-recenter_yaw + yaw_offset);

    // Pitch thresholds at which we place monitors on the ceiling or floor
    // Most people can more easily look up than down
    const float look_up_limit = PI_FLOAT * 0.2f;
    const float look_down_limit = -PI_FLOAT * 0.15f;

    // Calculate pitch matrix if we need one
    float recenter_pitch = atan2f(sqrtf(oriented.x * oriented.x + oriented.z * oriented.z), oriented.y) - PI_FLOAT * 0.5f;
    if (recenter_pitch > look_up_limit || recenter_pitch < look_down_limit) {
        Vector3 pitch_neutral(1.f, 0.f, 0.f);
        Vector3 pitch_oriented = Vector3::Transform(pitch_neutral, RenderModel->RecenterOrientation);
        RenderModel->PoseTransform *= Matrix::CreateFromAxisAngle(pitch_oriented, recenter_pitch);
    }

    RenderModel->PoseTransform *= Matrix::CreateTranslation(RenderModel->CurveCenter);
    RenderModel->PoseTransform.Invert(RenderModel->InversePoseTransform);
}

void MonitorRenderController::SolveDesktopPositions()
{
    Logger.Info("Solving desktop positions");

    // Find extents:

    bool first = true;
    RECT extents{};

    for (auto& monitor : Enumerator->Monitors)
    {
        if (first) {
            first = false;
            extents = monitor->Coords;
        }
        else {
            RECT rect = monitor->Coords;
            if (extents.left > rect.left) {
                extents.left = rect.left;
            }
            if (extents.right < rect.right) {
                extents.right = rect.right;
            }
            if (extents.top < rect.top) {
                extents.top = rect.top;
            }
            if (extents.bottom > rect.bottom) {
                extents.bottom = rect.bottom;
            }
        }
    }

    // TBD: This limit is no longer needed
    int desktop_width = (extents.right - extents.left);
    float desktop_width_m = desktop_width * RenderModel->MetersPerPixel;
    const float max_width = PI_FLOAT * 2.f * RenderModel->HmdFocalDistanceMeters;
    if (desktop_width_m > max_width)
    {
        const float max_meters_per_pixel = max_width / (float)desktop_width;
        const float min_dpi = XRM_METERS_PER_INCH / max_meters_per_pixel;
        RenderModel->SetDpi(min_dpi);

        Settings->MonitorDpi = min_dpi;
        Settings->SaveSettings();
    }

#if 0
    // FIXME: Get monitor curvature working
    int right_width = extents.right - RenderModel->ScreenFocusCenterX;
    int left_width = RenderModel->ScreenFocusCenterX - extents.left;
    int longer_side = std::max(right_width, left_width);
    const float target_theta = PI_FLOAT / 3.f;
    const float r = RenderModel->HmdFocalDistanceMeters;

    float s, theta = target_theta;

    for (int i = 0; i < 100; ++i)
    {
        s = longer_side * RenderModel->MetersPerPixel / theta;

        //Logger.Info("s * sinf(theta) = ", s * sinf(theta));
        //Logger.Info("s * cosf(theta) = ", s * cosf(theta));
        //Logger.Info("s = ", s);
        //Logger.Info("r = ", r);

        float actual_theta = -atanf(s * sinf(theta) / (s * cosf(theta) - (s - r)));

        //Logger.Info("theta = ", theta);
        //Logger.Info("actual_theta = ", actual_theta);
        //Logger.Info("target_theta = ", target_theta);

        if (actual_theta < target_theta) {
            break;
        }

        theta -= (actual_theta - target_theta) / 8.f;
    }

    // If the radius is above the normal focal distance:
    if (s > r)
    {
        Logger.Info("Must adjust cylinder radius larger to avoid neck strain = ", s);
        RenderModel->CurveRadiusMeters = s;

        Vector3 neutral(0.f, 0.f, s - r);

        //Quaternion orient = Quaternion::CreateFromYawPitchRoll(RenderModel->RecenterYaw, RenderModel->RecenterPitch, 0.f);
        Quaternion orient = RenderModel->RecenterOrientation;

        Vector3 oriented = Vector3::Transform(neutral, orient);
        //Vector3 oriented = Vector3::Transform(neutral, RenderModel->PitchTransform);

        RenderModel->CurveCenter = RenderModel->RecenterPosition + oriented;
    }
    else
#endif // FIXME
    {
        RenderModel->CurveRadiusMeters = RenderModel->HmdFocalDistanceMeters;
        RenderModel->CurveCenter = RenderModel->RecenterPosition;
    }

    for (auto& monitor : RenderModel->Monitors) {
        GenerateCylindricalGeometry(
            monitor->MonitorInfo,
            monitor.get());
    }

    // Update plugin positions
    Plugins->SolveDesktopPositions();
}

void MonitorRenderController::GenerateCylindricalGeometry(
    MonitorEnumInfo* enum_info,
    MonitorRenderState* render_state)
{
    const float radius_m = RenderModel->CurveRadiusMeters;

    const float x0 = RenderModel->MetersPerPixel * (enum_info->Coords.left - RenderModel->ScreenFocusCenterX);
    const float x1 = RenderModel->MetersPerPixel * (enum_info->Coords.right - RenderModel->ScreenFocusCenterX);
    const float len_m = x1 - x0;

    const float inv_radius_m = 1.f / radius_m;

    const float y0 = -RenderModel->MetersPerPixel * (enum_info->Coords.top - RenderModel->ScreenFocusCenterY);
    const float y1 = -RenderModel->MetersPerPixel * (enum_info->Coords.bottom - RenderModel->ScreenFocusCenterY);

    int target_quad_count = static_cast<int>(len_m / 0.01f);
    if (target_quad_count > 50) {
        target_quad_count = 50;
    }
    else if (target_quad_count < 4) {
        target_quad_count = 4;
    }
    const float dx = len_m / target_quad_count;
    const float du = 1.f / target_quad_count;

    // Make sure we do not leave very slim geometry on the right cap
    const float x1_end = x1 - dx * 1.5f;

    std::vector<VertexPositionTexture> vertices;

    int quad_count = 0;
    for (float x = x0, u = 0.f; x < x1_end; x += dx, u += du)
    {
        VertexPositionTexture top, bottom;

        // (s, t) = (x, z)
        // theta = Pi/2 -> (r, 0)
        // theta = 0 -> (0, -r)
        // theta = -Pi/2 -> (-r, 0)
        // theta = Pi -> (0, r)
        const float theta = x * inv_radius_m;
        const float s = radius_m * cosf(theta);
        const float t = radius_m * sinf(theta);

        top.position.x = s;
        top.position.y = y0;
        top.position.z = t;
        top.textureCoordinate.x = u;
        top.textureCoordinate.y = 0.f;

        ApplyTransform(top, RenderModel->PoseTransform);

        bottom.position.x = s;
        bottom.position.y = y1;
        bottom.position.z = t;
        bottom.textureCoordinate.x = u;
        bottom.textureCoordinate.y = 1.f;

        ApplyTransform(bottom, RenderModel->PoseTransform);

        vertices.push_back(bottom);
        vertices.push_back(top);
        ++quad_count;
    }

    // Final column:
    {
        VertexPositionTexture top, bottom;

        const float theta = x1 * inv_radius_m;
        const float s = radius_m * cosf(theta);
        const float t = radius_m * sinf(theta);

        top.position.x = s;
        top.position.y = y0;
        top.position.z = t;
        top.textureCoordinate.x = 1.f;
        top.textureCoordinate.y = 0.f;

        ApplyTransform(top, RenderModel->PoseTransform);

        bottom.position.x = s;
        bottom.position.y = y1;
        bottom.position.z = t;
        bottom.textureCoordinate.x = 1.f;
        bottom.textureCoordinate.y = 1.f;

        ApplyTransform(bottom, RenderModel->PoseTransform);

        vertices.push_back(bottom);
        vertices.push_back(top);
    }

    std::vector<uint16_t> indices;

    // Counter-clockwise winding facing the center
    for (int i = 0; i < quad_count; ++i)
    {
        indices.push_back(i * 2);
        indices.push_back(i * 2 + 1);
        indices.push_back(i * 2 + 2);

        indices.push_back(i * 2 + 2);
        indices.push_back(i * 2 + 1);
        indices.push_back(i * 2 + 3);
    }

    const D3D11_SUBRESOURCE_DATA vertex_data{ vertices.data() };
    const CD3D11_BUFFER_DESC vertex_desc(
        vertices.size() * sizeof(VertexPositionTexture),
        D3D11_BIND_VERTEX_BUFFER);
    XR_CHECK_HRCMD(Rendering->DeviceContext.Device->CreateBuffer(
        &vertex_desc,
        &vertex_data,
        render_state->VertexBuffer.ReleaseAndGetAddressOf()));

    const D3D11_SUBRESOURCE_DATA index_data{ indices.data() };
    const CD3D11_BUFFER_DESC index_desc(
        indices.size() * sizeof(uint16_t),
        D3D11_BIND_INDEX_BUFFER);
    XR_CHECK_HRCMD(Rendering->DeviceContext.Device->CreateBuffer(
        &index_desc,
        &index_data,
        render_state->IndexBuffer.ReleaseAndGetAddressOf()));

    render_state->IndexCount = (int)indices.size();
}

void MonitorRenderController::UpdateGaze()
{
    RenderModel->GazeValid = false;

    /*
        World coordinates:
        +x is right
        +y is up
        -z is forward
    */

    const Vector3& position = Rendering->HeadPosition;
    const Quaternion& orientation = Rendering->HeadOrientation;
    const Matrix& inverse_transform = RenderModel->InversePoseTransform;
    const float r = RenderModel->CurveRadiusMeters;

    // Pick a point along the ray looking forward
    Vector3 forward_ray(0.f, 0.f, -1.f);
    Vector3 forward = position + Vector3::Transform(forward_ray, orientation);

    // Transform viewer position and forward point back into neutral cylinder coordinates
    Vector3 c_position = Vector3::Transform(position, inverse_transform);
    Vector3 c_forward = Vector3::Transform(forward, inverse_transform);

    // Vector that points in the direction of the look angle
    Vector3 c_dir = c_forward - c_position;

    // Quadratic equation for axis-aligned infinite cylinder
    const float x = c_position.x;
    const float y = c_position.z;
    const float vx = c_dir.x;
    const float vy = c_dir.z;

    const float A = vx * vx + vy * vy;
    const float B = 2.f * (x * vx + y * vy);
    const float C = x * x + y * y - r * r;

    float root1, root2;
    if (!FindQuadraticRoots(A, B, C, root1, root2)) {
        // Does not intersect the infinite cylinder
        return;
    }

    // Pick the one that's farther away or the positive one
    const float larger_t = (root1 > root2) ? root1 : root2;

    if (larger_t < -EPSILON_FLOAT) {
        // Entirely behind us
        return;
    }

    // Up/down screen position
    const float iz = c_position.y + larger_t * c_dir.y;
    RenderModel->GazeY = static_cast<int>(-iz * RenderModel->PixelsPerMeter) + RenderModel->ScreenFocusCenterY;

    const float ix = c_position.x + larger_t * vx;
    const float iy = c_position.z + larger_t * vy;
    const float theta = atan2f(iy, ix);
    RenderModel->GazeX = static_cast<int>(theta * RenderModel->CurveRadiusMeters * RenderModel->PixelsPerMeter) + RenderModel->ScreenFocusCenterX;
    RenderModel->GazeValid = true;
}


} // namespace xrm
