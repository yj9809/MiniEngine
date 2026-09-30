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
	float4 gLightColor;
	float4 gAmbientColor;
};

Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

float4 PS(VSOutput input) : SV_TARGET
{
	float4 albedo = gTexture.Sample(gSampler, input.uv) * gBaseColor;

	float3 N = normalize(input.normal);
	float3 L = normalize(-gLightDirection.xyz);

	float diffuseFactor = saturate(dot(N, L));
	
	float3 ambient = gAmbientColor.rgb * albedo.rgb;
	float3 diffuse = gLightColor.rgb * diffuseFactor;
	
	float3 lighting = ambient + diffuse;

	return float4(albedo.rgb * lighting, albedo.a);
}