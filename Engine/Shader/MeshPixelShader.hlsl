struct VSOutput
{
	float4 pos : SV_POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
};

Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

float4 PS(VSOutput input) : SV_TARGET
{
	return gTexture.Sample(gSampler, input.uv);
}