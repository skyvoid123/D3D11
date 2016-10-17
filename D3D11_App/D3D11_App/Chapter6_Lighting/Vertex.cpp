#include "Vertex.h"
#include "Effects.h"

#pragma region InputLayoutDesc

const D3D11_INPUT_ELEMENT_DESC InputLayoutDesc::PosNormal[2] =
{
	{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
};

#pragma endregion

#pragma region InputLayouts

ID3D11InputLayout* InputLayouts::PosNormal = nullptr;

void InputLayouts::InitAll(ID3D11Device* device)
{
	D3DX11_PASS_DESC desc;
	Effects::BasicFX->Light1Tech->GetPassByIndex(0)->GetDesc(&desc);
	HR(device->CreateInputLayout(InputLayoutDesc::PosNormal, 2, desc.pIAInputSignature,
		desc.IAInputSignatureSize, &PosNormal));
}

void InputLayouts::DestroyAll()
{
	ReleaseCOM(PosNormal);
}

#pragma endregion