#pragma once

#include "d3dUtil.h"

namespace Vertex
{
	struct Basic32
	{
		Basic32() : Pos(0.0f, 0.0f, 0.0f), Normal(0.0f, 0.0f, 0.0f), Tex(0.0f, 0.0f)
		{

		}

		XMFLOAT3 Pos;
		XMFLOAT3 Normal;
		XMFLOAT2 Tex;
	};
}

class InputLayoutDesc
{
public:
	// Init like const int A::a[4] = { 0, 1, 2, 3 }; in.cpp file.
	static const D3D11_INPUT_ELEMENT_DESC InstancedBasic32[8];
};

class InputLayouts
{
public:
	static void InitAll(ID3D11Device* device);
	static void DestroyAll();

	static ID3D11InputLayout* InstancedBasic32;
};