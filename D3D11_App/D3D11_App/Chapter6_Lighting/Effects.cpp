#include "Effects.h"

#pragma region Effect
Effect::Effect(ID3D11Device* device, const std::wstring& filename)
	: mFx(nullptr)
{
	std::ifstream fin(filename, std::ios::binary);
	fin.seekg(0, std::ios_base::end);
	int size = (int)fin.tellg();
	fin.seekg(0, std::ios_base::beg);
	std::vector<char> compileShader(size);

	fin.read(&compileShader[0], size);
	fin.close();

	HR(D3DX11CreateEffectFromMemory(&compileShader[0], size, 0, device, &mFx));
}

Effect::~Effect()
{
	ReleaseCOM(mFx);
}
#pragma endregion

#pragma region BasicEffect
BasicEffect::BasicEffect(ID3D11Device* device, const std::wstring& filename)
	: Effect(device, filename)
{
	Light1Tech = mFx->GetTechniqueByName("Light1");
	Light2Tech = mFx->GetTechniqueByName("Light2");
	Light3Tech = mFx->GetTechniqueByName("Light3");

	WorldViewProj = mFx->GetVariableByName("gWorldViewProj")->AsMatrix();
	World = mFx->GetVariableByName("gWorld")->AsMatrix();
	WorldInvTranspose = mFx->GetVariableByName("gWorldInvTranspose")->AsMatrix();
	EyePosW = mFx->GetVariableByName("gEyePosW")->AsMatrix();
	DirLights = mFx->GetVariableByName("gDirLights");
	Mat = mFx->GetVariableByName("gMaterial");
}

BasicEffect::~BasicEffect()
{
}
#pragma endregion

#pragma region Effects
BasicEffect* Effects::BasicFX = nullptr;

void Effects::InitAll(ID3D11Device* device)
{
	BasicFX = new BasicEffect(device, L"FX/Basic.fxo");
}

void Effects::DestroyAll()
{
	SafeDelete(BasicFX);
}
#pragma endregion