#pragma pack_matrix(row_major)

struct VSInput
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float3 worldPosition : TEXCOORD1;
};

cbuffer WVPConstant : register(b0)
{
    float4x4 wvp;
}

cbuffer ObjectConstant : register(b1)
{
    float4x4 world;
    float4x4 normal;
}

VSOutput VS(VSInput input)
{
    VSOutput output;
    output.pos = mul(float4(input.pos, 1.0f), wvp);
    // Normal Matrix로 법선을 월드 공간으로 변환합니다.
    output.normal = normalize(mul(input.normal, (float3x3)normal));
    output.uv = input.uv;
    output.worldPosition = mul(float4(input.pos, 1.0f), world).xyz;
    return output;
}