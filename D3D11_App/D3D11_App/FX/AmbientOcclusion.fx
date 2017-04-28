#include "LightHelper.fx"

cbuffer cbPerObject
{
    float4x4 gWorldViewProj;
};

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 Tex : TEXCOORD;
    float AmbientOcc : AMBIENT;
};

struct VertexOut
{
    float4 PosH : SV_Position;
    float AmbientOcc : AMBIENT;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;

    vout.PosH = mul(float4(vin.PosL, 1.f), gWorldViewProj);

    vout.AmbientOcc = vin.AmbientOcc;

    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    return float4(pin.AmbientOcc.xxx, 1.0f);
}

technique11 AmbientOcclusion
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS()));
    }
}