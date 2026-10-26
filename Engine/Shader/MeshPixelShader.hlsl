struct VSOutput
{
	float4 pos : SV_POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
	float3 worldPosition : TEXCOORD1;
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

	float4 gSkyContribution;
	float4 gGroundContribution;

	uint gDirectionalLightCount;
	float3 gLightingPadding;
};

cbuffer CameraConstantBuffer : register(b2)
{
	float4 gCameraPosition;
}

Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

float4 PS(VSOutput input) : SV_TARGET
{
	float4 albedo = gTexture.Sample(gSampler, input.uv) * gBaseColor;

	float3 N = normalize(input.normal);
	float skyWeight = saturate(N.z * 0.5f + 0.5f);
	float3 ambient = lerp(gGroundContribution.rgb, gSkyContribution.rgb, skyWeight);
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
