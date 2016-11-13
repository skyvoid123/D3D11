#include "LightHelper.fx"

cbuffer cbPerFrame
{
    DirectionalLight gDirLights[3];
    float3 gEyePosW;

    float gFogStart;
    float gFogRange;
    float4 gFogColor;
};

cbuffer cbPerObject
{
    float4x4 gViewProj;
    Material gMaterial;
};

cbuffer cbFixed
{
    // Compute texture coordinates to stretch texture over quad.
    float2 gTexC[4] =
    {
        float2(0.f, 1.f),
        float2(0.f, 0.f),
        float2(1.f, 1.f),
        float2(1.f, 0.f)
    };
}

// Nonnumeric values cannot be added to a cbuffer.
Texture2DArray gTreeMapArray;

SamplerState samLinear
{
    Filter = MIN_MAG_MIP_LINEAR;
    
    AddressU = CLAMP;
    AddressV = CLAMP;
};

struct VertexIn
{
    float3 PosW : POSITION;
    float2 SizeW : SIZE;
};

struct VertexOut
{
    float3 CenterW : POSITION;
    float2 SizeW : SIZE;
};

struct GeoOut
{
    float4 PosH : SV_Position;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 Tex : TEXCOORD;
    uint PrimID : SV_PrimitiveID;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;

    vout.CenterW = vin.PosW;
    vout.SizeW = vin.SizeW;

    return vout;
}

 // We expand each point into a quad (4 vertices), so the maximum number of vertices
 // we output per geometry shader invocation is 4.
[maxvrtexcount(4)]
void GS(point VertexOut gin[1],
        uint primID : SV_PrimitiveID,
        inout TriangleStream<GeoOut> triStream)
{
    // Compute the local coordinate system of the sprite relative to the world
	// space such that the billboard is aligned with the y-axis and faces the eye.
    float3 up = float3(0.f, 1.f, 0.f);
    float3 look = gEyePosW - gin[0].CenterW;
    look.y = 0.f; // y-axis aligned, so project to xz-plane
    look = normalize(look);
    float3 right = cross(up, look);

    // Compute triangle strip vertices (quad) in world space.
    float halfWidth = 0.5f * gin[0].SizeW.x;
    float halfHeight = 0.5f * gin[0].SizeW.y;

    float4 v[4];
    v[0] = float4(gin[0].CenterW + halfWidth * right - halfHeight * up, 1.f);
    v[1] = float4(gin[0].CenterW + halfWidth * right + halfHeight * up, 1.f);
    v[0] = float4(gin[0].CenterW - halfWidth * right - halfHeight * up, 1.f);
    v[0] = float4(gin[0].CenterW - halfWidth * right + halfHeight * up, 1.f);

    // Transform quad vertices to world space and output 
	// them as a triangle strip.
    GeoOut gout;
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        gout.PosH = mul(v[i], gViewProj);
        gout.PosW = v[i].xyz;
        gout.NormalW = look;
        gout.Tex = gTexC[i];
        gout.PrimID = primID;

        triStream.Append(gout);
    }

}

float4 PS(GeoOut pin, uniform int gLightCount, uniform bool gUseTexture, uniform bool gAlphaClip, uniform bool gFogEnabled) : SV_Target
{
    pin.NormalW = normalize(pin.NormalW);

    float3 toEye = gEyePosW - pin.PosW;
    float distToEye = length(toEye);
    toEye /= distToEye;

    // Default to multiplicative identity.
    float4 texColor = float4(1.f, 1.f, 1.f, 1.f);
    if (gUseTexture)
    {
        // Sample texture.
        float3 uvw = float3(pin.Tex, pin.PrimID % 4);
        texColor = gTreeMapArray.Sample(samLinear, uvw);

        if (gAlphaClip)
        {
            // Discard pixel if texture alpha < 0.1.  Note that we do this
			// test as soon as possible so that we can potentially exit the shader 
			// early, thereby skipping the rest of the shader code.
            clip(texColor.a - .1f);
        }
    }

    // Lighting

    float4 litColor = texColor;
    if (gLightCount > 0)
    {
        float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
        float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
        float4 spec = float4(0.0f, 0.0f, 0.0f, 0.0f);

    // Sum the light contribution from each light source.  
    [unroll]
        for (int i = 0; i < gLightCount; ++i)
        {
            float4 A, D, S;
            ComputeDirectionalLight(gMaterial, gDirLights[i], pin.NormalW, toEye, A, D, S);

            ambient += A;
            diffuse += D;
            spec += S;
        }

        // Modulate with late add.
        litColor = texColor * (ambient + diffuse) + spec;
    }

    if (gFogEnabled)
    {
        float fogLerp = saturate((distToEye - gFogStart) / gFogRange);
        litColor = lerp(litColor, gFogColor, fogLerp);
    }

    // Common to take alpha from diffuse material and texture.
    litColor.a = gMaterial.Diffuse.a * texColor.a;

    return litColor;
}


technique11 Light3
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetGeometryShader(CompileShader(gs_5_0, GS()));
        SetPixelShader(CompileShader(ps_5_0, PS(3, false, false, false)));
    }
}

technique11 Light3TexAlphaClip
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetGeometryShader(CompileShader(gs_5_0, GS()));
        SetPixelShader(CompileShader(ps_5_0, PS(3, true, true, false)));
    }
}

technique11 Light3TexAlphaClipFog
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetGeometryShader(CompileShader(gs_5_0, GS()));
        SetPixelShader(CompileShader(ps_5_0, PS(3, true, true, true)));
    }
}