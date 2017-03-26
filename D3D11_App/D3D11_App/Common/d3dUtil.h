#pragma once

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBUG_MAP_ALLOC
#include <crtdbg.h>
#endif

//#include <D3DX11.h>
#include "d3dx11effect.h"
//#include <xnamath.h>
#include <DirectXMath.h>
#include "dxerr.h"
#include <cassert>
#include <ctime>
#include <algorithm>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include "MathHelper.h"
#include "LightHelper.h"

using namespace DirectX;

#if defined(DEBUG) | defined(_DEBUG)
#ifndef HR
#define HR(x)                                               \
	{                                                           \
		HRESULT hr = (x);                                       \
		if(FAILED(hr))                                          \
		{                                                       \
			DXTrace(__FILEW__, (DWORD)__LINE__, hr, L#x, true); \
		}                                                       \
	}
#endif

#else
#ifndef HR
#define HR(x) (x)
#endif
#endif 


#define ReleaseCOM(x) {if(x) {x->Release(); x = nullptr;}}

#define  SafeDelete(x) {delete x; x = nullptr;}

// #define XMGLOBALCONST extern CONST __declspec(selectany)
//   1. extern so there is only one copy of the variable, and not a separate
//      private copy in each .obj.
//   2. __declspec(selectany) so that the compiler does not complain about
//      multiple definitions in a .cpp file (it can pick anyone and discard 
//      the rest because they are constant--all the same).

namespace Colors
{
	XMGLOBALCONST XMVECTORF32 White = { 1.0f, 1.0f, 1.0f, 1.0f };
	XMGLOBALCONST XMVECTORF32 Black = { 0.0f, 0.0f, 0.0f, 1.0f };
	XMGLOBALCONST XMVECTORF32 Red = { 1.0f, 0.0f, 0.0f, 1.0f };
	XMGLOBALCONST XMVECTORF32 Green = { 0.0f, 1.0f, 0.0f, 1.0f };
	XMGLOBALCONST XMVECTORF32 Blue = { 0.0f, 0.0f, 1.0f, 1.0f };
	XMGLOBALCONST XMVECTORF32 Yellow = { 1.0f, 1.0f, 0.0f, 1.0f };
	XMGLOBALCONST XMVECTORF32 Cyan = { 0.0f, 1.0f, 1.0f, 1.0f };
	XMGLOBALCONST XMVECTORF32 Magenta = { 1.0f, 0.0f, 1.0f, 1.0f };

	XMGLOBALCONST XMVECTORF32 Silver = { 0.75f, 0.75f, 0.75f, 1.0f };
	XMGLOBALCONST XMVECTORF32 LightSteelBlue = { 0.69f, 0.77f, 0.87f, 1.0f };
}

struct Box
{
	XMFLOAT3 center;
	XMFLOAT3 extent;

	Box()
		: center(0, 0, 0)
		, extent(0, 0, 0)
	{

	}

	Box(const XMFLOAT3& c, const XMFLOAT3& e)
		: center(c)
		, extent(e)
	{
	}

	XMVECTOR GetMinV() const
	{
		XMVECTOR c = XMLoadFloat3(&center);
		XMVECTOR e = XMLoadFloat3(&extent);
		return c - e;
	}

	XMVECTOR GetMaxV() const
	{
		XMVECTOR c = XMLoadFloat3(&center);
		XMVECTOR e = XMLoadFloat3(&extent);
		return c + e;
	}
};

struct Ray
{
	XMFLOAT3 origin;
	XMFLOAT3 direction;

	Ray(const XMFLOAT3& o, const XMFLOAT3& d)
		: origin(o)
		, direction(d)
	{
	}

	Ray(FXMVECTOR o, CXMVECTOR d)
	{
		XMStoreFloat3(&origin, o);
		XMStoreFloat3(&direction, d);
	}

	bool IsIntersectBox(const Box& box, float* pDist);
	bool IsIntersectTriangle(FXMVECTOR v0, CXMVECTOR v1, CXMVECTOR v2, float* pDist);
};

void ExtractFrustumPlanes(XMFLOAT4 planes[6], CXMMATRIX M);