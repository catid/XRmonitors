// A constant buffer that stores the model transform.
cbuffer ModelConstantBuffer : register(b0)
{
    float4x4 model[2];
};

// A constant buffer that stores each set of view and projection matrices in column-major format.
cbuffer ViewProjectionConstantBuffer : register(b1)
{
    float4x4 viewProjection[2];
};

// Per-vertex data used as input to the vertex shader.
struct VertexShaderInput
{
    min16float3 pos     : POSITION;
	min16float2 tex     : TEXCOORD1;
    uint        instId  : SV_InstanceID;
};

// Simple shader to do vertex processing on the GPU.
VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
	//float4 pos = float4(input.pos.x, input.pos.y, 0.0f, 1.0f);
	float4 pos = float4(input.pos, 1.0f);

    // Note which view this vertex has been sent to. Used for matrix lookup.
    // Taking the modulo of the instance ID allows geometry instancing to be used
    // along with stereo instanced drawing; in that case, two copies of each 
    // instance would be drawn, one for left and one for right.
    int idx = input.instId % 2;

    // Transform the vertex position into world space.
    pos = mul(pos, model[idx]);

    // Correct for perspective and project the vertex position onto the screen.
    pos = mul(pos, viewProjection[idx]);

	pos.z = 0.99f * pos.w; // Place it behind everything else
	//pos.z = 0.99f; // Place it behind everything else
	//pos.w = 1.f;

	output.pos = (min16float4)pos;

	min16float2 tex = input.tex;
	tex.x = (tex.x + idx) * 0.5f; // Pick left or right side of image based on view
	output.tex = tex;

    // Set the render target array index.
    output.viewId = idx;

    return output;
}
