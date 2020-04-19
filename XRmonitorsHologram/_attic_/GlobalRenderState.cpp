// Copyright 2019 Augmented Perception Corporation

#include "stdafx.h"

#include "GlobalRenderState.hpp"
#include <math.h>

namespace xrm {

static logger::Channel Logger("Global");


//------------------------------------------------------------------------------
// GlobalRenderState

// Dots (pixels) per inch
void GlobalRenderState::SetDPI(float dpi)
{
	// 0.0254 m per inch
	DPI = dpi;
	MetersPerPixel = 0.0254f / dpi;
	PixelsPerMeter = dpi / 0.0254f;
}


//------------------------------------------------------------------------------
// SphericalGeometry

bool SphericalGeometry::InitializeFromExtent(
	float meters_per_pixel,
	RECT screen_extent,
	int center_x,
	int center_y,
	float hmd_focal_distance_meters)
{
	HmdFocalDistanceMeters = hmd_focal_distance_meters;
	MetersPerPixel = meters_per_pixel;
	CenterX = center_x;
	CenterY = center_y;

	const int ul_y = abs(screen_extent.top - center_y);
	const int lr_y = abs(screen_extent.bottom - center_y);
	int largest_y = ul_y;
	if (largest_y < lr_y) {
		largest_y = lr_y;
	}
	if (largest_y <= 0) {
		return false;
	}
	const float y_meters = largest_y * meters_per_pixel;

	Scaling = false;

	// Expand the sphere up/down to keep the content from going too far back overhead
	ScaleY = 1.f;
	curve_limit_y = hmd_focal_distance_meters * 3.14159265f * 0.5f * limit_factor_y;
	if (y_meters > curve_limit_y) {
		// Stretch the sphere vertically to fit everything
		ScaleY = curve_limit_y / y_meters;
		Scaling = true;
	}

	const int x_extent = screen_extent.right - screen_extent.left;
	if (x_extent <= 0) {
		return false;
	}
	const float x_meters = x_extent * meters_per_pixel;

	// Expand the sphere left/right to keep the content from going too far back overhead
	ScaleX = 1.f;
	curve_limit_x = hmd_focal_distance_meters * 3.14159265f * 2.f;
	if (x_meters > curve_limit_x) {
		// Stretch the sphere horizontally to fit everything
		ScaleX = curve_limit_x / x_meters;
		Scaling = true;
	}

	if (ScaleX > ScaleY) {
		ScaleX = ScaleY;
	}
	else if (ScaleY > ScaleX) {
		ScaleY = ScaleX;
	}

	return true;
}

bool SphericalGeometry::ViewerInsideSphere(float3 position) const
{
	const float x = position.x * ScaleX;
	const float y = position.y * ScaleY;
	const float z = position.z;

	const float r_sqr = x * x + y * y + z * z;
	return r_sqr < HmdFocalDistanceMeters * HmdFocalDistanceMeters;
}

void SphericalGeometry::QuadsForArc(const RECT& rect, int& quads_x, int& quads_y) const
{
	const float arc_meters_x = abs(rect.right - rect.left) * MetersPerPixel * ScaleX;
	const float arc_meters_y = abs(rect.bottom - rect.top) * MetersPerPixel * ScaleY;

	quads_x = (int)(max_quads_x * arc_meters_x / curve_limit_x) + 1;
	if (quads_x > max_quads_x) {
		quads_x = max_quads_x;
	}
	else if (quads_x <= 0) {
		quads_x = 1;
	}
	quads_y = (int)(max_quads_y * arc_meters_y / curve_limit_y) + 1;
	if (quads_y > max_quads_y) {
		quads_y = max_quads_y;
	}
	else if (quads_y <= 0) {
		quads_y = 1;
	}
}

void SphericalGeometry::ConvertScreenToAngle(int screen_x, int screen_y, float2& angle) const
{
	const int x = screen_x - CenterX;
	const int y = screen_y - CenterY;

	const float k = MetersPerPixel / HmdFocalDistanceMeters;

	angle.x = 3.14159265f + 3.14159265f * 0.5f + x * k * ScaleX;
	angle.y = 3.14159265f * 0.5f + y * k * ScaleY;
}

void SphericalGeometry::ConvertAngleTo3DPosition(const float2& angle, float radius, float3& position) const
{
	// From:
	// https://en.wikipedia.org/wiki/Spherical_coordinate_system

	const float k = radius * sinf(angle.y);

	position.z = k * sinf(angle.x);
	position.x = k * cosf(angle.x);
	position.y = radius * cosf(angle.y);
}


//------------------------------------------------------------------------------
// MonitorSphericalSection

void MonitorSphericalSection::GenerateVectors(
	const SphericalGeometry& geometry,
	const RECT& rect_coords)
{
	Vertices.clear();
	Indices.clear();

	int quad_cols, quad_rows;
	geometry.QuadsForArc(rect_coords, quad_cols, quad_rows);
	const int vertices_rows = quad_rows + 1;
	const int vertices_cols = quad_cols + 1;
	if (vertices_cols > 128) {
		Logger.Error("vertices_cols exploded INIT");
	}

	float2 ul, lr;
	geometry.ConvertScreenToAngle(rect_coords.left, rect_coords.top, ul);
	geometry.ConvertScreenToAngle(rect_coords.right, rect_coords.bottom, lr);

	const float arc_rads_x = lr.x - ul.x;
	const float arc_rads_y = lr.y - ul.y;

	const float inv_quad_cols = 1.f / quad_cols;
	const float inv_quad_rows = 1.f / quad_rows;
	const float rads_per_quad_x = arc_rads_x * inv_quad_cols;
	const float rads_per_quad_y = arc_rads_y * inv_quad_rows;

	const float u_per_quad = inv_quad_cols;
	const float v_per_quad = inv_quad_rows;
	const float u0 = 0.f;
	const float v0 = 0.f;

	float y_rads = ul.y;
	float v = v0;
	unsigned ul_index = 0;

	for (unsigned row = 0; row < vertices_rows; ++row)
	{
		float x_rads = ul.x;
		float u = u0;

		if (vertices_cols > 128) {
			Logger.Error("vertices_cols exploded");
		}

		for (unsigned col = 0; col < vertices_cols; ++col)
		{
			float2 angle;
			angle.x = x_rads;
			angle.y = y_rads;

			// Upper-left vertex of this quad
			float3 vertex;
			geometry.ConvertAngleTo3DPosition(angle, vertex);

			RenderMonitors::VertexPosition vp;
			vp.pos.x = vertex.x;
			vp.pos.y = vertex.y;
			vp.pos.z = vertex.z;
			vp.tex.x = u;
			vp.tex.y = v;
			Vertices.push_back(vp);

			if (col > 0 && row > 0)
			{
				// Two inward-facing triangles:
				Indices.push_back(ul_index - vertices_cols);
				Indices.push_back(ul_index - 1);
				Indices.push_back(ul_index - vertices_cols - 1);

				Indices.push_back(ul_index - vertices_cols);
				Indices.push_back(ul_index);
				Indices.push_back(ul_index - 1);
			}

			ul_index++;
			x_rads += rads_per_quad_x;
			u += u_per_quad;
		}

		y_rads += rads_per_quad_y;
		v += v_per_quad;
	}

	IndexCount = (unsigned)Indices.size();
}

bool MonitorSphericalSection::CreateD3D11Buffers(ID3D11Device4 * device)
{
	VertexBuffer.Reset();
	IndexBuffer.Reset();

	D3D11_SUBRESOURCE_DATA vertexBufferData{};
	vertexBufferData.pSysMem = Vertices.data();
	vertexBufferData.SysMemPitch = 0;
	vertexBufferData.SysMemSlicePitch = 0;

	const CD3D11_BUFFER_DESC vertexBufferDesc(
		sizeof(RenderMonitors::VertexPosition) * static_cast<UINT>(Vertices.size()),
		D3D11_BIND_VERTEX_BUFFER);

	HRESULT hr = device->CreateBuffer(
		&vertexBufferDesc,
		&vertexBufferData,
		&VertexBuffer);
	if (FAILED(hr)) {
		Logger.Error("CreateBuffer(vertex) failed: ", HresultString(hr));
		return false;
	}

	D3D11_SUBRESOURCE_DATA index_buffer_data{};
	index_buffer_data.pSysMem = Indices.data();
	index_buffer_data.SysMemPitch = 0;
	index_buffer_data.SysMemSlicePitch = 0;

	CD3D11_BUFFER_DESC index_buffer_desc(
		(UINT)(sizeof(uint16_t) * IndexCount),
		D3D11_BIND_INDEX_BUFFER);

	hr = device->CreateBuffer(
		&index_buffer_desc,
		&index_buffer_data,
		&IndexBuffer);
	if (FAILED(hr)) {
		Logger.Error("CreateBuffer(index) failed: ", HresultString(hr));
		return false;
	}

	return true;
}


//------------------------------------------------------------------------------
// BezelSphericalSection

void BezelSphericalSection::StartVectors()
{
	Vertices.clear();
	Indices.clear();
}

void BezelSphericalSection::AddRect(
	const SphericalGeometry& geometry,
	const RECT& rect_coords)
{
	int quad_cols, quad_rows;
	geometry.QuadsForArc(rect_coords, quad_cols, quad_rows);
	const int vertices_rows = quad_rows + 1;
	const int vertices_cols = quad_cols + 1;

	float2 ul, lr;
	geometry.ConvertScreenToAngle(rect_coords.left, rect_coords.top, ul);
	geometry.ConvertScreenToAngle(rect_coords.right, rect_coords.bottom, lr);

	const float arc_rads_x = lr.x - ul.x;
	const float arc_rads_y = lr.y - ul.y;

	const float inv_quad_cols = 1.f / quad_cols;
	const float inv_quad_rows = 1.f / quad_rows;
	const float rads_per_quad_x = arc_rads_x * inv_quad_cols;
	const float rads_per_quad_y = arc_rads_y * inv_quad_rows;

	float y_rads = ul.y;
	unsigned ul_index = (unsigned)Vertices.size();

	for (unsigned row = 0; row < vertices_rows; ++row)
	{
		float x_rads = ul.x;

		for (unsigned col = 0; col < vertices_cols; ++col)
		{
			float2 angle;
			angle.x = x_rads;
			angle.y = y_rads;

			// Upper-left vertex of this quad
			float3 vertex;
			geometry.ConvertAngleTo3DPosition(angle, vertex);

			RenderBezels::VertexPosition vp;
			vp.pos.x = vertex.x;
			vp.pos.y = vertex.y;
			vp.pos.z = vertex.z;

			Vertices.push_back(vp);

			if (col > 0 && row > 0)
			{
				// Two inward-facing triangles:
				Indices.push_back(ul_index - vertices_cols);
				Indices.push_back(ul_index - 1);
				Indices.push_back(ul_index - vertices_cols - 1);

				Indices.push_back(ul_index - vertices_cols);
				Indices.push_back(ul_index);
				Indices.push_back(ul_index - 1);
			}

			ul_index++;
			x_rads += rads_per_quad_x;
		}

		y_rads += rads_per_quad_y;
	}

	IndexCount = (unsigned)Indices.size();
}

bool BezelSphericalSection::CreateD3D11Buffers(ID3D11Device4 * device)
{
	D3D11_SUBRESOURCE_DATA vertexBufferData{};
	vertexBufferData.pSysMem = Vertices.data();
	vertexBufferData.SysMemPitch = 0;
	vertexBufferData.SysMemSlicePitch = 0;

	const CD3D11_BUFFER_DESC vertexBufferDesc(
		sizeof(RenderBezels::VertexPosition) * static_cast<UINT>(Vertices.size()),
		D3D11_BIND_VERTEX_BUFFER);

	HRESULT hr = device->CreateBuffer(
		&vertexBufferDesc,
		&vertexBufferData,
		&VertexBuffer);
	if (FAILED(hr)) {
		Logger.Error("CreateBuffer(vertex) failed: ", HresultString(hr));
		return false;
	}

	D3D11_SUBRESOURCE_DATA index_buffer_data{};
	index_buffer_data.pSysMem = Indices.data();
	index_buffer_data.SysMemPitch = 0;
	index_buffer_data.SysMemSlicePitch = 0;

	CD3D11_BUFFER_DESC index_buffer_desc(
		(UINT)(sizeof(uint16_t) * IndexCount),
		D3D11_BIND_INDEX_BUFFER);

	hr = device->CreateBuffer(
		&index_buffer_desc,
		&index_buffer_data,
		&IndexBuffer);
	if (FAILED(hr)) {
		Logger.Error("CreateBuffer(index) failed: ", HresultString(hr));
		return false;
	}

	return true;
}


} // namespace xrm
