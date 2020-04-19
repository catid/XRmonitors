// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
    min16float4 pos   : SV_POSITION;
	min16float2 tex   : TEXCOORD1;
};

Texture2D shaderTexture;
SamplerState SampleType;

// The pixel shader passes through the color data. The color data from 
// is interpolated and assigned to a pixel at the rasterization step.
min16float4 main(PixelShaderInput input) : SV_TARGET
{
	min16float4 color = shaderTexture.Sample(SampleType, input.tex);
	return min16float4(color.r, color.r, color.r, 1.0);
}
