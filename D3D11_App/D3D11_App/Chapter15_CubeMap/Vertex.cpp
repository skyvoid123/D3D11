#include "Vertex.h"
#include "Effects.h"

#pragma region InputLayoutDesc

const D3D11_INPUT_ELEMENT_DESC InputLayoutDesc::Pos[1] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
};

const D3D11_INPUT_ELEMENT_DESC InputLayoutDesc::Basic32[3] =
{
	{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0}
};

const D3D11_INPUT_ELEMENT_DESC InputLayoutDesc::TreePointSprite[2] = 
{
	{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{"SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
};

#pragma endregion

#pragma region InputLayouts

ID3D11InputLayout* InputLayouts::Pos = nullptr;
ID3D11InputLayout* InputLayouts::Basic32 = nullptr;
ID3D11InputLayout* InputLayouts::TreePointSprite = nullptr;

void InputLayouts::InitAll(ID3D11Device* device)
{
	D3DX11_PASS_DESC desc;

	Effects::SkyFX->SkyTech->GetPassByIndex(0)->GetDesc(&desc);
	HR(device->CreateInputLayout(InputLayoutDesc::Pos, 1, desc.pIAInputSignature,
		desc.IAInputSignatureSize, &Pos));

	Effects::BasicFX->Light1Tech->GetPassByIndex(0)->GetDesc(&desc);
	HR(device->CreateInputLayout(InputLayoutDesc::Basic32, 3, desc.pIAInputSignature,
		desc.IAInputSignatureSize, &Basic32));

	Effects::TreeSpriteFX->Light3Tech->GetPassByIndex(0)->GetDesc(&desc);
	HR(device->CreateInputLayout(InputLayoutDesc::TreePointSprite, 2, desc.pIAInputSignature,
		desc.IAInputSignatureSize, &TreePointSprite));
}

void InputLayouts::DestroyAll()
{
	ReleaseCOM(Basic32);
	ReleaseCOM(TreePointSprite);
	ReleaseCOM(Pos);
}

#pragma endregion