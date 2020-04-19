// Copyright 2019 Augmented Perception Corporation

#pragma once

#include "stdafx.h"

#include "MonitorTools.hpp"

#include <SimpleMath.h>
#include <VertexTypes.h>

#include <vector>

namespace xrm {

using namespace DirectX;
using namespace DirectX::SimpleMath; // Vector3


//------------------------------------------------------------------------------
// GlobalRenderState

/*
	When in recentering mode:

	Animate to rendering screen content in small fronto-parallel quads.
	Display a reticle where the user is gazing.

	On leaving recentering mode:

	Animate to rendering screen content at normal sizes, re-centered.
	Display cursor as normal.
*/

struct GlobalRenderState
{
	// Do we have a recenter pose yet?
	bool HaveRecenterPose = false;

	// If non-zero: This is the first pose time in ticks
	uint64_t FirstPoseTicks = 0;

	// In recentering mode?
	bool RecenterActive = false;
	uint64_t RecenterChangeTicks = 0;

	// Based on HMD type
	float HmdFocalDistanceMeters = 1.4f;

	// Monitor scale
	float DPI = 300.f;
	float PixelsPerMeter = 0.f;
	float MetersPerPixel = 0.0001f;

	// Monitor with the mouse last time we reset
	int MouseFocusMonitor = 0;

	// Pixel position on the desktop where the center is at
	int ScreenFocusCenterX = 0;
	int ScreenFocusCenterY = 0;

    Vector3 HeadsetReset_Position;
	Quaternion HeadsetReset_Orientation;


	// Dots (pixels) per inch
	void SetDPI(float dpi);
};


//------------------------------------------------------------------------------
// SphericalGeometry

/*
	This is a spherical coordinate system so that the reset angle screen
	distance is perfectly e.g. 1.4 meters from the viewer, but the radius
	of the sphere is large enough so that the monitors do not get distorted
	too much or wrap all the way around.

	Functions are provided to map from screen x, y coordinates to angles
	left/right (phi) and up/down (theta), and from angles to x,y,z points
	in the space relative to the viewer position.
*/
class SphericalGeometry
{
public:
	// Initialize the space from the whole screen rectangle extents
	bool InitializeFromExtent(
		float meters_per_pixel,
		RECT screen_extent,
		int center_x,
		int center_y,
		float hmd_focal_distance_meters);

	// Is the viewer inside the sphere (or very close to the edge?)
	bool ViewerInsideSphere(Vector3 position) const;

	// Returns the number of quads needed for a smooth-looking arc between
	// the given two x or y positions
	void QuadsForArc(const RECT& rect, int& quads_x, int& quads_y) const;

	// Convert (x, y) from screen to (phi_x, theta_y) spherical coordinates
	void ConvertScreenToAngle(int screen_x, int screen_y, Vector2& angle) const;

	// Convert (phi_x, theta_y) to (x, y, z) 3D position
	void ConvertAngleTo3DPosition(const Vector2& angle, float radius, Vector3& position) const;

	// Position on the surface of the sphere
	void ConvertAngleTo3DPosition(const Vector2& angle, Vector3& position) const
	{
		ConvertAngleTo3DPosition(angle, HmdFocalDistanceMeters, position);
	}

	float GetSphereRadius() const
	{
		return HmdFocalDistanceMeters;
	}

	bool IsScalingToFit() const
	{
		return Scaling;
	}

protected:
	// TBD: Configure this?
	const float limit_factor_y = 0.6f;

	// TBD: Configure this?
	const int max_quads_x = 100;
	const int max_quads_y = 50;

	float HmdFocalDistanceMeters = 0.f;
	float MetersPerPixel = 0.f;
	int CenterX = 0;
	int CenterY = 0;

	float curve_limit_x = 0.f;
	float curve_limit_y = 0.f;

	float ScaleX = 1.f;
	float ScaleY = 1.f;

	bool Scaling = false;
};


//------------------------------------------------------------------------------
// MonitorSphericalSection

struct MonitorSphericalSection
{
	void GenerateVectors(
		const SphericalGeometry& geometry,
		const RECT& rect_coords);

	std::vector<VertexPositionTexture> Vertices;
	std::vector<uint16_t> Indices;
	unsigned IndexCount = 0;

	bool CreateD3D11Buffers(ID3D11Device4* device);

	ComPtr<ID3D11Buffer> VertexBuffer;
	ComPtr<ID3D11Buffer> IndexBuffer;

    Vector3 Center;
	Quaternion Orientation;

    Matrix ModelTransform;
};


//------------------------------------------------------------------------------
// BezelSphericalSection

struct BezelSphericalSection
{
	void StartVectors();
	void AddRect(
		const SphericalGeometry& geometry,
		const RECT& rect_coords);
	bool IsEmpty() const {
		return Vertices.empty();
	}

	std::vector<VertexPosition> Vertices;
	std::vector<uint16_t> Indices;
	unsigned IndexCount = 0;

	bool CreateD3D11Buffers(ID3D11Device4* device);

	ComPtr<ID3D11Buffer> VertexBuffer;
	ComPtr<ID3D11Buffer> IndexBuffer;

	Vector3 Center;
	Quaternion Orientation;

    Matrix ModelTransform;
};


//------------------------------------------------------------------------------
// MonitorRenderInfo

struct MonitorRenderInfo
{
	bool RenderBezel;
	Vector3 BezelColor;

	// How is the physical monitor rotated?
	float RotationDegrees = 0.f;
	DXGI_MODE_ROTATION Rotation;

	MonitorSphericalSection SphereMonitor;
	BezelSphericalSection SphereBezel;

    Vector3 PreResetPosition;
    Vector3 CenterPosition;
	Quaternion Orientation;
	float WidthMeters = 1.f;
	float HeightMeters = 1.f;

	// Transform used for rendering
	Matrix ModelTransform;

	// Workspace
	float OffsetX = 0.f;
	float OffsetY = 0.f;
};


//------------------------------------------------------------------------------
// CameraRenderInfo

struct CameraRenderInfo
{
    Vector3 Center;
	Quaternion Orientation;
};


} // namespace xrm
