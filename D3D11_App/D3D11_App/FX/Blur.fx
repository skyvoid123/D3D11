cbuffer cbSettings
{
	float gWeights[9];
};

cbuffer cvFixed
{
	static const int gBlurRadius = 4;
};

Texture2D gInput;
RWTexture2D<float4> gOutput;

#define N 256
#define CacheSize (N + 2 * gBlurRadius)

groupshared float4 gCache[CacheSize];

[numthreads(N, 1, 1)]
void HorzBlurCS(int3 groupThreadID : SV_GroupThreadID,
							int3 dispatchThreadID : SV_DispatchThreadID)
{
	uint width, height;
	gInput.GetDimensions(width, height);

	//
	// Fill local thread storage to reduce bandwidth.  To blur 
	// N pixels, we will need to load N + 2*BlurRadius pixels
	// due to the blur radius.
	//

	// This thread group runs N threads.  To get the extra 2*BlurRadius pixels, 
	// have 2*BlurRadius threads sample an extra pixel.
	if (groupThreadID.x < gBlurRadius)
	{
		// Clamp out of bound samples that occur at image borders.
		int x = max(dispatchThreadID.x - gBlurRadius, 0);
		gCache[groupThreadID.x] = gInput[int2(x, dispatchThreadID.y)];
	}
	else if (groupThreadID.x >= N - gBlurRadius)
	{
		// Clamp out of bound samples that occur at image borders.
		int x = min(dispatchThreadID.x + gBlurRadius, width - 1);
		gCache[groupThreadID.x + 2 * gBlurRadius] = gInput[int2(x, dispatchThreadID.y)];
	}
	gCache[groupThreadID.x + gBlurRadius] = gInput[min(dispatchThreadID.xy, int2(width, height) - 1)];

	// Wait for all threads to finish.
	GroupMemoryBarrierWithGroupSync();

	//
	// Now blur each pixel.
	//
	float4 blurColor = float4(0, 0, 0, 0);

	[unroll]
	for (int i = -gBlurRadius; i <= gBlurRadius; ++i)
	{
		blurColor += gWeights[gBlurRadius + i] * gCache[groupThreadID.x + gBlurRadius + i];
	}

	gOutput[dispatchThreadID.xy] = blurColor;
}

[numthreads(1, N, 1)]
void VertBlurCS(int3 groupThreadID : SV_GroupThreadID,
							int3 dispatchTreadID : SV_DispatchThreadID)
{
	uint width, height;
	gInput.GetDimensions(width, height);

	if (groupThreadID.y < gBlurRadius)
	{
		int y = max(dispatchTreadID.y - gBlurRadius, 0);
		gCache[groupThreadID.y] = gInput[int2(dispatchTreadID.x, y)];
	}
	else if (groupThreadID.y >= N - gBlurRadius)
	{
		int y = min(dispatchTreadID.y + gBlurRadius, height - 1);
		gCache[groupThreadID.y + 2 * gBlurRadius] = gInput[int2(dispatchTreadID.x, y)];
	}
	gCache[groupThreadID.y + gBlurRadius] = gInput[min(dispatchTreadID.xy, int2(width, height) - 1)];

	GroupMemoryBarrierWithGroupSync();

	float4 blurColor = float4(0, 0, 0, 0);

	[unroll]
	for (int i = -gBlurRadius; i <= gBlurRadius; ++i)
	{
		blurColor += gWeights[gBlurRadius + i] * gCache[groupThreadID.y + gBlurRadius + i];
	}

	gOutput[dispatchTreadID.xy] = blurColor;
}

technique11 HorzBlur
{
	pass P0
	{
		SetVertexShader(NULL);
		SetPixelShader(NULL);
		SetComputeShader(CompileShader(cs_5_0, HorzBlurCS()));
	}
}

technique11 VertBlur
{
	pass P0
	{
		SetVertexShader(NULL);
		SetPixelShader(NULL);
		SetComputeShader(CompileShader(cs_5_0, VertBlurCS()));
	}
}