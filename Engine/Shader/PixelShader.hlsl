struct VSOutput
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

float4 PS(VSOutput input) : SV_TARGET
{
    return input.color;
}