cbuffer cbPerFrame
{
    float3 gEyePosW;
};

cbuffer cbPerObject
{
    float4x4 gWorld;
    float4x4 gWVP;
};

struct VertexIn
{
    float3 PosL : POSITION;
};

struct VertexOut
{
    float3 PosL : POSITION;
};

struct HullOut
{
    float3 PosL : POSITION;
};

struct DomainOut
{
    float4 PosH : SV_Position;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    vout.PosL = vin.PosL;
    return vout;
}

struct PatchTess
{
    float EdgeTess[4] : SV_TessFactor;
    float InsideTess[2] : SV_InsideTessFactor;
};

//////////////////////////////////////////////////////////////////////////////////
//BasicTessellation
/////////////////////////////////////////////////////////////////////////////////
PatchTess BasicConstantHS(InputPatch<VertexOut, 4> patch)
{
    PatchTess pt;
    
    float3 centerL = 0.25f * (patch[0].PosL + patch[1].PosL + patch[2].PosL + patch[3].PosL);
    float3 centerW = mul(float4(centerL, 1.f), gWorld).xyz;

    float d = distance(gEyePosW, centerW);

    // Tessellate the patch based on distance from the eye such that
	// the tessellation is 0 if d >= d1 and 64 if d <= d0.  The interval
	// [d0, d1] defines the range we tessellate in.
	
    const float d0 = 20.f;
    const float d1 = 100.f;
    float tess = saturate((d1 - d) / (d1 - d0)) * 64;

    // Uniformly tessellate the patch.
    pt.EdgeTess[0] = tess;
    pt.EdgeTess[1] = tess;
    pt.EdgeTess[2] = tess;
    pt.EdgeTess[3] = tess;

    pt.InsideTess[0] = tess;
    pt.InsideTess[1] = tess;

    return pt;
}

[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("BasicConstantHS")]
[maxtessfactor(64.f)]
HullOut BasicHS(InputPatch<VertexOut, 4> patch, uint i : SV_OutputControlPointID)
{
    HullOut hout;
    hout.PosL = patch[i].PosL;
    return hout;
}

// The domain shader is called for every vertex created by the tessellator.  
// It is like the vertex shader after tessellation.
[domain("quad")]
DomainOut BasicDS(PatchTess patchTess,
                        float2 uv : SV_DomainLocation,
                        const OutputPatch<HullOut, 4> patch)
{
    DomainOut dout;

    // Bilinear interpolation.
    float3 v1 = lerp(patch[0].PosL, patch[1].PosL, uv.x);
    float3 v2 = lerp(patch[2].PosL, patch[3].PosL, uv.x);
    float3 p = lerp(v1, v2, uv.y);

   	// Displacement mapping
    p.y = 0.3f * (p.z * sin(p.x) + p.x * cos(p.z));

    dout.PosH = mul(float4(p, 1.f), gWVP);

    return dout;
}

//////////////////////////////////////////////////////////////////////////////////
//Bezier Tessellation
/////////////////////////////////////////////////////////////////////////////////
PatchTess BezierConstantHS(InputPatch<VertexOut, 16> patch)
{
    PatchTess pt;

    // Uniformly tessellate the patch.
    pt.EdgeTess[0] = 25;
    pt.EdgeTess[1] = 25;
    pt.EdgeTess[2] = 25;
    pt.EdgeTess[3] = 25;

    pt.InsideTess[0] = 25;
    pt.InsideTess[1] = 25;

    return pt;
}

// This Hull Shader part is commonly used for a coordinate basis change, 
// for example changing from a quad to a Bezier bi-cubic.
[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(16)]
[patchconstantfunc("BezierConstantHS")]
[maxtessfactor(64.0f)]
HullOut BezierHS(InputPatch<VertexOut, 16> patch, uint i : SV_OutputControlPointID)
{
    HullOut hout;
    hout.PosL = patch[i].PosL;
    return hout;
}

float4 BernsteinBasis(float t)
{
    float invT = 1.f - t;
    return float4(invT * invT * invT,
                        3.f * t * invT * invT,
                        3.f * t * t * invT,
                        t * t * t);
}

float3 CubicBezierSum(const OutputPatch<HullOut, 16> patch, float4 basisU, float4 basisV)
{
    float3 sum = float3(0.f, 0.f, 0.f);
    sum += basisV.x * (basisU.x * patch[0].PosL + basisU.y * patch[1].PosL + basisU.z * patch[2].PosL + basisU.w * patch[3].PosL);
    sum += basisV.y * (basisU.x * patch[4].PosL + basisU.y * patch[5].PosL + basisU.z * patch[6].PosL + basisU.w * patch[7].PosL);
    sum += basisV.z * (basisU.x * patch[8].PosL + basisU.y * patch[9].PosL + basisU.z * patch[10].PosL + basisU.w * patch[11].PosL);
    sum += basisV.w * (basisU.x * patch[12].PosL + basisU.y * patch[13].PosL + basisU.z * patch[14].PosL + basisU.w * patch[15].PosL);

    return sum;
}

// The domain shader is called for every vertex created by the tessellator.  
// It is like the vertex shader after tessellation.
[domain("quad")]
DomainOut BezierDS(PatchTess patchTess, float2 uv : SV_DomainLocation,
                                    const OutputPatch<HullOut, 16> patch)
{
    DomainOut dout;

    float4 basisU = BernsteinBasis(uv.x);
    float4 basisV = BernsteinBasis(uv.y);

    float3 p = CubicBezierSum(patch, basisU, basisV);
    dout.PosH = mul(float4(p, 1.f), gWVP);

    return dout;
}

float4 PS(DomainOut pin) : SV_Target
{
    return float4(1.f, 1.f, 1.f, 1.f);
}

technique11 BasicTessTech
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, BasicHS()));
        SetDomainShader(CompileShader(ds_5_0, BasicDS()));
        SetPixelShader(CompileShader(ps_5_0, PS()));
    }
}

technique11 BezierTessTech
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, BezierHS()));
        SetDomainShader(CompileShader(ds_5_0, BezierDS()));
        SetPixelShader(CompileShader(ps_5_0, PS()));
    }
}