struct VSInput
{
    float3 pos : POSITION;
    float4 color : COLOR;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

cbuffer WVPConstant : register(b0)
{
    float4x4 wvp;
}

VSOutput VS(VSInput input)
{    
    VSOutput output;
    output.pos = mul(float4(input.pos, 1.0f), wvp);
    output.color = input.color;
    return output;
}