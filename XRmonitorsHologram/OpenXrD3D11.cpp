// Copyright 2019 Augmented Perception Corporation

#pragma once

#include "stdafx.h"

#include "OpenXrD3D11.hpp"

namespace xrm {

using namespace DirectX::SimpleMath;


//------------------------------------------------------------------------------
// Tools

void OpenXrToQuaternion(
    Quaternion& to,
    const XrQuaternionf& from)
{
    to.x = from.x;
    to.y = from.y;
    to.z = from.z;
    to.w = from.w;
}

void OpenXrToVector(
    Vector3& to,
    const XrVector3f& from)
{
    to.x = from.x;
    to.y = from.y;
    to.z = from.z;
}

void OpenXrFromQuaternion(
    XrQuaternionf& to,
    const Quaternion& from)
{
    to.x = from.x;
    to.y = from.y;
    to.z = from.z;
    to.w = from.w;
}

void OpenXrFromVector(
    XrVector3f& to,
    const Vector3& from)
{
    to.x = from.x;
    to.y = from.y;
    to.z = from.z;
}


//------------------------------------------------------------------------------
// PoseAliasingFilter

void PoseAliasingFilter::Filter(
    const XrPosef& pose0,
    const XrPosef& pose1,
    const DirectX::SimpleMath::Quaternion& head_orientation,
    const DirectX::SimpleMath::Vector3& head_position)
{
    Vector3 test(0.f, 0.f, -1.f), result;
    Vector3::Transform(test, head_orientation, result);

    const float d = Vector3::DistanceSquared(head_position, LastAcceptedHeadPosition);

    if (d >= 0.00001f) {
        LastAcceptedHeadOrientationResult = result;
        LastAcceptedHeadPosition = head_position;
        FilteredPoses[0] = pose0;
        FilteredPoses[1] = pose1;
        return;
    }

    const float t = Vector3::DistanceSquared(result, LastAcceptedHeadOrientationResult);

    if (t >= 0.0001f) {
        LastAcceptedHeadOrientationResult = result;
        LastAcceptedHeadPosition = head_position;
        FilteredPoses[0] = pose0;
        FilteredPoses[1] = pose1;
        return;
    }
}


} // namespace xrm
