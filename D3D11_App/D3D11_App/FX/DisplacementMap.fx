#include "LightHelper.fx"

cbuffer cbPerFrame
{
    DirectionalLight gDirLights[3];
    float3 gEyePosW;

    float gFogStart;
    float gFogRange;
    float4 gFogColor;

    float gHeightScale;
    float gMaxTessDistance;
    float gMinTessDistance;
    float gMinTessFactor;
    float gMaxTessFactor;
}

cbuffer cbPerObject
{
    float4x4 gWorld;
    float4x4 gWorldInvTranspose;
    float4x4 gViewProj;
    float4x4 gWorldViewProj;
    float4x4 gShadowTransform;
    float4x4 gTexTransform;
    Material gMaterial;
};

// Nonnumeric values cannot be added to a cbuffer.
Texture2D gDiffuseMap;
Texture2D gShadowMap;
Texture2D gNormalMap;
TextureCube gCubeMap;

SamplerState samLinear
{
    Filter = MIN_MAG_MIP_LINEAR;
    
    AddressU = wrap;
    AddressV = wrap;
};

SamplerComparisonState samShadow
{
    Filter = COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
    AddressU = BORDER;
    AddressV = BORDER;
    AddressW = BORDER;
    BorderColor = float4(0.0f, 0.0f, 0.0f, 0.0f);

    ComparisonFunc = LESS;
};

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 Tex : TEXCOORD;
    float3 TangentL : TANGENT;
};

struct VertexOut
{
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float3 TangentW : TANGENT;
    float2 Tex : TEXCOORD;
    float TessFactor : TESS;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;

    // Transform to world space space.
    vout.PosW = mul(float4(vin.PosL, 1.f), gWorld).xyz;
    vout.NormalW = mul(vin.NormalL, (float3x3) gWorldInvTranspose);
    vout.TangentW = mul(vin.TangentL, (float3x3) gWorld);
    vout.Tex = mul(float4(vin.Tex, 0.f, 1.f), gTexTransform).xy;

    float d = distance(vout.PosW, gEyePosW);
    // Normalized tessellation factor. 
	// The tessellation is 
	//   0 if d >= gMinTessDistance and
	//   1 if d <= gMaxTessDistance.  
    float tess = saturate((gMinTessDistance - d) / (gMinTessDistance - gMaxTessDistance));
   	// Rescale [0,1] --> [gMinTessFactor, gMaxTessFactor].
    vout.TessFactor = gMinTessFactor + tess * (gMaxTessFactor - gMinTessFactor);

    return vout;
}

struct PatchTess
{
    float EdgeTess[3] : SV_TessFactor;
    float InsideTess : SV_InsideTessFactor;
};

PatchTess PatchHS(InputPatch<VertexOut, 3> patch)
{
    PatchTess pt;
    // Average tess factors along edges, and pick an edge tess factor for 
	// the interior tessellation.  It is important to do the tess factor
	// calculation based on the edge properties so that edges shared by 
	// more than one triangle will have the same tessellation factor.  
	// Otherwise, gaps can appear.
    pt.EdgeTess[0] = 0.5f * (patch[1].TessFactor + patch[2].TessFactor);
    pt.EdgeTess[1] = 0.5f * (patch[0].TessFactor + patch[2].TessFactor);
    pt.EdgeTess[2] = 0.5f * (patch[0].TessFactor + patch[1].TessFactor);
    pt.InsideTess = pt.EdgeTess[0];

    return pt;
}

struct HullOut
{
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float3 TangentW : TANGENT;
    float2 Tex : TEXCOORD;
};


[patchconstantfunc("PatchHS")]
[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
HullOut HS(InputPatch<VertexOut, 3> patch,
            uint i : SV_OutputControlPointID)
{
    HullOut hout;
    
    hout.PosW = patch[i].PosW;
    hout.NormalW = patch[i].NormalW;
    hout.TangentW = patch[i].TangentW;
    hout.Tex = patch[i].Tex;

    return hout;
}

struct DomainOut
{
    float4 PosH : SV_Position;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float3 TangentW : TANGENT;
    float2 Tex : TEXCOORD0;
    float4 ShadowPosH : TEXCOORD1;
};

// The domain shader is called for every vertex created by the tessellator.  
// It is like the vertex shader after tessellation.
[domain("tri")]
DomainOut DS(PatchTess patchTess,
            float3 bary : SV_DomainLocation,
            const OutputPatch<HullOut, 3> tri)
{
    DomainOut dout;

    // Interpolate patch attributes to generated vertices.
    dout.PosW = bary.x * tri[0].PosW + bary.y * tri[1].PosW + bary.z * tri[2].PosW;
    dout.NormalW = bary.x * tri[0].NormalW + bary.y * tri[1].NormalW + bary.z * tri[2].NormalW;
    dout.TangentW = bary.x * tri[0].TangentW + bary.y * tri[1].TangentW + bary.z * tri[2].TangentW;
    dout.Tex = bary.x * tri[0].Tex + bary.y * tri[1].Tex + bary.z * tri[2].Tex;

    // Interpolating normal can unnormalize it, so normalize it.
    dout.NormalW = normalize(dout.NormalW);

    //
	// Displacement mapping.
	//

    // Choose the mipmap level based on distance to the eye; specifically, choose
	// the next miplevel every MipInterval units, and clamp the miplevel in [0,6].
    const float MipInterval = 20.0f;
    float mipLevel = clamp((distance(dout.PosW, gEyePosW) - MipInterval) / MipInterval, 0.0f, 6.0f);
    
    // Sample height map (stored in alpha channel).
    float h = gNormalMap.SampleLevel(samLinear, dout.Tex, mipLevel).a;

    // Offset vertex along normal.
    dout.PosW += (gHeightScale * (h - 1.0f)) * dout.NormalW;

	// Generate projective tex-coords to project shadow map onto scene.
    dout.ShadowPosH = mul(float4(dout.PosW, 1.0f), gShadowTransform);

    // Project to homogeneous clip space.
    dout.PosH = mul(float4(dout.PosW, 1.0f), gViewProj);

    return dout;
}


float4 PS(DomainOut pin,
            uniform int gLightCount,
            uniform bool gUseTexture,
            uniform bool gAlphaClip,
            uniform bool gFogEnabled,
            uniform bool gReflectionEnabled) : SV_Target
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
        texColor = gDiffuseMap.Sample(samLinear, pin.Tex);

        if (gAlphaClip)
        {
            // Discard pixel if texture alpha < 0.1.  Note that we do this
			// test as soon as possible so that we can potentially exit the shader 
			// early, thereby skipping the rest of the shader code.
            clip(texColor.a - .1f);
        }
    }

    // Normal mapping
    float3 normalMapSample = gNormalMap.Sample(samLinear, pin.Tex).rgb;
    float3 bumpedNormalW = NormalSampleToWorldSpace(normalMapSample, pin.NormalW, pin.TangentW);

    // Lighting
    float4 litColor = texColor;
    if (gLightCount > 0)
    {
        float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
        float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
        float4 spec = float4(0.0f, 0.0f, 0.0f, 0.0f);

		// Only the first light casts a shadow.
        float3 shadow = float3(1.0f, 1.0f, 1.0f);
        shadow[0] = CalcShadowFactor(samShadow, gShadowMap, pin.ShadowPosH);

        // Sum the light contribution from each light source.  
        [unroll]
        for (int i = 0; i < gLightCount; ++i)
        {
            float4 A, D, S;
            ComputeDirectionalLight(gMaterial, gDirLights[i], bumpedNormalW, toEye, A, D, S);

            ambient += A;
            diffuse += shadow[i] * D;
            spec += shadow[i] * S;
        }

        // Modulate with late add.
        litColor = texColor * (ambient + diffuse) + spec;

        if (gReflectionEnabled)
        {
            float3 incident = -toEye;
            float3 reflectionVector = reflect(incident, bumpedNormalW);
            float4 reflectionColor = gCubeMap.Sample(samLinear, reflectionVector);

            litColor += gMaterial.Reflect * reflectionColor;
        }
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

technique11 Light1
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(1, false, false, false, false)));
    }
}

technique11 Light2
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(2, false, false, false, false)));
    }
}

technique11 Light3
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(3, false, false, false, false)));
    }
}

technique11 Light0Tex
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(0, true, false, false, false)));
    }
}

technique11 Light1Tex
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(1, true, false, false, false)));
    }
}

technique11 Light2Tex
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(2, true, false, false, false)));
    }
}

technique11 Light3Tex
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(3, true, false, false, false)));
    }
}

technique11 Light0TexAlphaClip
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(0, true, true, false, false)));
    }
}

technique11 Light1TexAlphaClip
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(1, true, true, false, false)));
    }
}

technique11 Light2TexAlphaClip
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(2, true, true, false, false)));
    }
}

technique11 Light3TexAlphaClip
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(3, true, true, false, false)));
    }
}

technique11 Light1Fog
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(1, false, false, true, false)));
    }
}

technique11 Light2Fog
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(2, false, false, true, false)));
    }
}

technique11 Light3Fog
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(3, false, false, true, false)));
    }
}

technique11 Light0TexFog
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(0, true, false, true, false)));
    }
}

technique11 Light1TexFog
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(1, true, false, true, false)));
    }
}

technique11 Light2TexFog
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(2, true, false, true, false)));
    }
}

technique11 Light3TexFog
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(3, true, false, true, false)));
    }
}

technique11 Light0TexAlphaClipFog
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(0, true, true, true, false)));
    }
}

technique11 Light1TexAlphaClipFog
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(1, true, true, true, false)));
    }
}

technique11 Light2TexAlphaClipFog
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(2, true, true, true, false)));
    }
}

technique11 Light3TexAlphaClipFog
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(3, true, true, true, false)));
    }
}

technique11 Light1Reflect
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(1, false, false, false, true)));
    }
}

technique11 Light2Reflect
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(2, false, false, false, true)));
    }
}

technique11 Light3Reflect
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(3, false, false, false, true)));
    }
}

technique11 Light0TexReflect
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(0, true, false, false, true)));
    }
}

technique11 Light1TexReflect
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(1, true, false, false, true)));
    }
}

technique11 Light2TexReflect
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(2, true, false, false, true)));
    }
}

technique11 Light3TexReflect
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(3, true, false, false, true)));
    }
}

technique11 Light0TexAlphaClipReflect
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(0, true, true, false, true)));
    }
}

technique11 Light1TexAlphaClipReflect
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(1, true, true, false, true)));
    }
}

technique11 Light2TexAlphaClipReflect
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(2, true, true, false, true)));
    }
}

technique11 Light3TexAlphaClipReflect
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(3, true, true, false, true)));
    }
}

technique11 Light1FogReflect
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(1, false, false, true, true)));
    }
}

technique11 Light2FogReflect
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(2, false, false, true, true)));
    }
}

technique11 Light3FogReflect
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(3, false, false, true, true)));
    }
}

technique11 Light0TexFogReflect
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(0, true, false, true, true)));
    }
}

technique11 Light1TexFogReflect
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(1, true, false, true, true)));
    }
}

technique11 Light2TexFogReflect
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(2, true, false, true, true)));
    }
}

technique11 Light3TexFogReflect
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(3, true, false, true, true)));
    }
}

technique11 Light0TexAlphaClipFogReflect
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(0, true, true, true, true)));
    }
}

technique11 Light1TexAlphaClipFogReflect
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(1, true, true, true, true)));
    }
}

technique11 Light2TexAlphaClipFogReflect
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(2, true, true, true, true)));
    }
}

technique11 Light3TexAlphaClipFogReflect
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(3, true, true, true, true)));
    }
}