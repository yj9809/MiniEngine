struct VSOutput
{
	float4 pos : SV_POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
};

float4 PS(VSOutput input) : SV_TARGET
{
	return float4(input.normal * 0.5f + 0.5f, 1.0f);
}