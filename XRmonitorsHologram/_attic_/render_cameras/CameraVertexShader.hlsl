// Per-vertex data passed to the geometry shader.
struct VertexShaderOutput
{
    min16float4 pos     : SV_POSITION;
	min16float2 tex     : TEXCOORD1;

    // The render target array index will be set by the geometry shader.
    uint        viewId  : TEXCOORD0;
};

#include "CameraVertexShaderShared.hlsl"
