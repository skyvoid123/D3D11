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

	Light0TexTech = mFx->GetTechniqueByName("Light0Tex");
	Light1TexTech = mFx->GetTechniqueByName("Light1Tex");
	Light2TexTech = mFx->GetTechniqueByName("Light2Tex");
	Light3TexTech = mFx->GetTechniqueByName("Light3Tex");

	Light0TexAlphaClipTech = mFx->GetTechniqueByName("Light0TexAlphaClip");
	Light1TexAlphaClipTech = mFx->GetTechniqueByName("Light1TexAlphaClip");
	Light2TexAlphaClipTech = mFx->GetTechniqueByName("Light2TexAlphaClip");
	Light3TexAlphaClipTech = mFx->GetTechniqueByName("Light3TexAlphaClip");

	Light1FogTech = mFx->GetTechniqueByName("Light1Fog");
	Light2FogTech = mFx->GetTechniqueByName("Light2Fog");
	Light3FogTech = mFx->GetTechniqueByName("Light3Fog");

	Light0TexFogTech = mFx->GetTechniqueByName("Light0TexFog");
	Light1TexFogTech = mFx->GetTechniqueByName("Light1TexFog");
	Light2TexFogTech = mFx->GetTechniqueByName("Light2TexFog");
	Light3TexFogTech = mFx->GetTechniqueByName("Light3TexFog");

	Light0TexAlphaClipFogTech = mFx->GetTechniqueByName("Light0TexAlphaClipFog");
	Light1TexAlphaClipFogTech = mFx->GetTechniqueByName("Light1TexAlphaClipFog");
	Light2TexAlphaClipFogTech = mFx->GetTechniqueByName("Light2TexAlphaClipFog");
	Light3TexAlphaClipFogTech = mFx->GetTechniqueByName("Light3TexAlphaClipFog");

	WorldViewProj = mFx->GetVariableByName("gWorldViewProj")->AsMatrix();
	World = mFx->GetVariableByName("gWorld")->AsMatrix();
	WorldInvTranspose = mFx->GetVariableByName("gWorldInvTranspose")->AsMatrix();
	TexTransform = mFx->GetVariableByName("gTexTransform")->AsMatrix();
	EyePosW = mFx->GetVariableByName("gEyePosW")->AsVector();
	FogColor = mFx->GetVariableByName("gFogColor")->AsVector();
	FogStart = mFx->GetVariableByName("gFogStart")->AsScalar();
	FogRange = mFx->GetVariableByName("gFogRange")->AsScalar();
	DirLights = mFx->GetVariableByName("gDirLights");
	Mat = mFx->GetVariableByName("gMaterial");
	DiffuseMap = mFx->GetVariableByName("gDiffuseMap")->AsShaderResource();
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