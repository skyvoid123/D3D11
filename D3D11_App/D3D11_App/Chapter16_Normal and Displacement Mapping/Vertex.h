#pragma once

#include "d3dUtil.h"

namespace Vertex
{
	struct Pos
	{
		Pos(float x, float y, float z)
			: X(x), Y(y), Z(z)
		{}

		float X;
		float Y;
		float Z;
	};

	struct Basic32
	{
		Basic32() : Pos(0.0f, 0.0f, 0.0f), Normal(0.0f, 0.0f, 0.0f), Tex(0.0f, 0.0f)
		{

		}

		XMFLOAT3 Pos;
		XMFLOAT3 Normal;
		XMFLOAT2 Tex;
	};

	struct TreePointSprite
	{
		XMFLOAT3 Pos;
		XMFLOAT2 Size;
	};

	struct PosNormalTexTan
	{
		XMFLOAT3 Pos;
		XMFLOAT3 Normal;
		XMFLOAT2 Tex;
		XMFLOAT3 TangentU;
	};
}

class InputLayoutDesc
{
public:
	// Init like const int A::a[4] = { 0, 1, 2, 3 }; in.cpp file.
	static const D3D11_INPUT_ELEMENT_DESC Pos[1];
	static const D3D11_INPUT_ELEMENT_DESC Basic32[3];
	static const D3D11_INPUT_ELEMENT_DESC TreePointSprite[2];
	static const D3D11_INPUT_ELEMENT_DESC PosNormalTexTan[4];
};

class InputLayouts
{
public:
	static void InitAll(ID3D11Device* device);
	static void DestroyAll();

	static ID3D11InputLayout* Pos;
	static ID3D11InputLayout* Basic32;
	static ID3D11InputLayout* TreePointSprite;
	static ID3D11InputLayout* PosNormalTexTan;
};