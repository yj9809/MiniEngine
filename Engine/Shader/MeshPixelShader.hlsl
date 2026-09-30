struct VSOutput
{
	float4 pos : SV_POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
};

cbuffer MaterialConstantBuffer : register(b0)
{
	float4 gBaseColor;
};
cbuffer LightingConstantBuffer : register(b1)
{
	float4 gLightDirection;
};

Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

float4 PS(VSOutput input) : SV_TARGET
{
	float4 albedo = gTexture.Sample(gSampler, input.uv) * gBaseColor;

	float3 N = normalize(input.normal);
	float3 L = normalize(-gLightDirection.xyz);

	float diffuse = saturate(dot(N, L));

	return float4(albedo.rgb * diffuse, albedo.a);
}