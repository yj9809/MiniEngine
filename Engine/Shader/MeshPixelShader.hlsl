struct VSOutput
{
	float4 pos : SV_POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
};

#define MAX_DIRECTIONAL_LIGHTS 4

struct DirectionalLightGPUData
{
	float4 direction;
	float4 colorAndIntensity;
};

cbuffer MaterialConstantBuffer : register(b0)
{
	float4 gBaseColor;
};
cbuffer LightingConstantBuffer : register(b1)
{
	DirectionalLightGPUData gDirectionalLights[MAX_DIRECTIONAL_LIGHTS];
	float4 gAmbientColor;
	uint gDirectionalLightCount;
	float3 gLightingPadding;
};

Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

float4 PS(VSOutput input) : SV_TARGET
{
	float4 albedo = gTexture.Sample(gSampler, input.uv) * gBaseColor;

	float3 N = normalize(input.normal);
	float3 ambient = gAmbientColor.rgb * albedo.rgb;
	float3 diffuse = float3(0.0f, 0.0f, 0.0f);

	for (uint index = 0; index < gDirectionalLightCount; ++index)
	{
		DirectionalLightGPUData light = gDirectionalLights[index];
		float3 L = normalize(-light.direction.xyz);
		float diffuseFactor = saturate(dot(N, L));

		diffuse += light.colorAndIntensity.rgb * light.colorAndIntensity.w * diffuseFactor;
	}
	
	float3 lighting = ambient + diffuse;

	return float4(albedo.rgb * lighting, albedo.a);
}
