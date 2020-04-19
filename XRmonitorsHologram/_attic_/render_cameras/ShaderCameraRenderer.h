// Copyright 2019 Augmented Perception Corporation

#pragma once

#include "stdafx.h"

#include "ShaderCameraStructures.h"
#include "../GlobalRenderState.hpp"
#include "../CameraCalibration.hpp"

namespace RenderCameras {


struct Vertex {
	XrVector3f Position;
	XrVector3f Color;
};

struct ModelConstantBuffer {
	XMFLOAT4X4 Model;
};

struct ViewProjectionConstantBuffer {
	XMFLOAT4X4 ViewProjection;
};


// This sample renderer instantiates a basic rendering pipeline.
class ShaderCameraRenderer
{
public:
	ShaderCameraRenderer(
		const xrm::HeadsetCameraCalibration& calibration,
		std::shared_ptr<DX::DeviceResources> const& deviceResources);
    void CreateDeviceDependentResources(const xrm::HeadsetCameraCalibration& calibration);
    void ReleaseDeviceDependentResources();

	bool UpdateTexture(const uint8_t* image, unsigned width, unsigned height);

	// Render spherical monitor
	bool RenderCamera(
		const xrm::HeadsetCameraCalibration& calibration,
		SpatialLocation const& headset_location,
		DX::CameraResources* camera_resources);

private:
    // Cached pointer to device resources.
    std::shared_ptr<DX::DeviceResources>            m_deviceResources;

    // Direct3D resources for cube geometry.
    Microsoft::WRL::ComPtr<ID3D11InputLayout>       m_inputLayout;
    Microsoft::WRL::ComPtr<ID3D11Buffer>            m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer>            m_indexBuffer;
    Microsoft::WRL::ComPtr<ID3D11VertexShader>      m_vertexShader;
    Microsoft::WRL::ComPtr<ID3D11GeometryShader>    m_geometryShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>       m_pixelShader;
    Microsoft::WRL::ComPtr<ID3D11Buffer>            m_modelConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11SamplerState>      m_samplerState;

    // System resources for cube geometry.
    ModelConstantBuffer m_modelConstantBufferData;
    uint32_t m_indexCount = 0;

	// Texture that we can map to CPU memory (on dupe device)
	ComPtr<ID3D11Texture2D> StagingTexture;
	D3D11_TEXTURE2D_DESC StagingTextureDesc;

	// Texture that we can map to CPU memory (on VR device)
	ComPtr<ID3D11Texture2D> RenderTexture;
	D3D11_TEXTURE2D_DESC RenderTextureDesc;

    // Variables used with the rendering loop.
    bool m_loadingComplete = false;

    // If the current D3D Device supports VPRT, we can avoid using a geometry
    // shader just to set the render target array index.
    bool m_usingVprtShaders = false;

	int LastAcceptedBright = 0;
	unsigned FrameSkipped = 0;


	bool CreateTextures(unsigned width, unsigned height);


	std::vector<VertexPosition> Vertices;
	std::vector<uint16_t> Indices;

	void GenerateMesh(float k1);
	void WarpVertex(VertexPosition& vp, float k1);
};


} // namespace RenderCameras
