#pragma pack_matrix(row_major)

struct VSInput
{
    float3 pos : POSITION;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
};

cbuffer WVPConstant : register(b0)
{
    float4x4 wvp;
}

VSOutput VS(VSInput input)
{
    VSOutput output;
    output.pos = mul(float4(input.pos, 1.0f), wvp);
    return output;
}