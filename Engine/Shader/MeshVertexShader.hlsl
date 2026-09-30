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
};

cbuffer WVPConstant : register(b0)
{
    float4x4 wvp;
}

cbuffer WorldConstant : register(b1)
{
    float4x4 world;
}

VSOutput VS(VSInput input)
{
    VSOutput output;
    output.pos = mul(float4(input.pos, 1.0f), wvp);
    // 월드 행렬을 사용하여 법선 벡터를 월드 공간으로 변환합니다.
    output.normal = normalize(mul(float4(input.normal, 0.0f), world).xyz);
    output.uv = input.uv;
    return output;
}