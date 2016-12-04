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

#pragma region TreeSpriteEffect
TreeSpriteEffect::TreeSpriteEffect(ID3D11Device* device, const std::wstring& filename)
	: Effect(device, filename)
{
	Light3Tech = mFx->GetTechniqueByName("Light3");
	Light3TexAlphaClipTech = mFx->GetTechniqueByName("Light3TexAlphaClip");
	Light3TexAlphaClipFogTech = mFx->GetTechniqueByName("Light3TexAlphaClipFog");

	ViewProj = mFx->GetVariableByName("gViewProj")->AsMatrix();
	EyePosW = mFx->GetVariableByName("gEyePosW")->AsVector();
	FogColor = mFx->GetVariableByName("gFogColor")->AsVector();
	FogStart = mFx->GetVariableByName("gFogStart")->AsScalar();
	FogRange = mFx->GetVariableByName("gFogRange")->AsScalar();
	DirLights = mFx->GetVariableByName("gDirLights");
	Mat = mFx->GetVariableByName("gMaterial");
	TreeTextureMapArray = mFx->GetVariableByName("gTreeMapArray")->AsShaderResource();
}

TreeSpriteEffect::~TreeSpriteEffect()
{

}
#pragma endregion

#pragma region VecAddEffect
VecAddEffect::VecAddEffect(ID3D11Device* device, const std::wstring& filename)
	: Effect(device, filename)
{
	VecAddTech = mFx->GetTechniqueByName("VecAddTech");
	InputA = mFx->GetVariableByName("gInputA")->AsShaderResource();
	InputB = mFx->GetVariableByName("gInputB")->AsShaderResource();
	Output = mFx->GetVariableByName("gOutput")->AsUnorderedAccessView();
}

VecAddEffect::~VecAddEffect()
{
}
#pragma endregion

#pragma region BlurEffect
BlurEffect::BlurEffect(ID3D11Device* device, const std::wstring& filename)
	: Effect(device, filename)
{
	HorzBlurTech = mFx->GetTechniqueByName("HorzBlur");
	VertBlurTech = mFx->GetTechniqueByName("VertBlur");

	Weights = mFx->GetVariableByName("gWeights")->AsScalar();
	InputMap = mFx->GetVariableByName("gInput")->AsShaderResource();
	OutputMap = mFx->GetVariableByName("gOutput")->AsUnorderedAccessView();
}

BlurEffect::~BlurEffect()
{

}
#pragma endregion

#pragma region TessellationEffect
TessellationEffect::TessellationEffect(ID3D11Device* device, const std::wstring& filename)
	: Effect(device, filename)
{
	BasicTessellationTech = mFx->GetTechniqueByName("BasicTessTech");
	BezierTessellationTech = mFx->GetTechniqueByName("BezierTessTech");
	EyePosW = mFx->GetVariableByName("gEyePosW")->AsVector();
	World = mFx->GetVariableByName("gWorld")->AsMatrix();
	WorldViewProj = mFx->GetVariableByName("gWVP")->AsMatrix();
}
TessellationEffect::~TessellationEffect()
{

}
#pragma endregion

#pragma region Effects
BasicEffect* Effects::BasicFX = nullptr;
TreeSpriteEffect* Effects::TreeSpriteFX = nullptr;
VecAddEffect* Effects::VecAddFX = nullptr;
BlurEffect* Effects::BlurFX = nullptr;
TessellationEffect* Effects::TessellationFX = nullptr;

void Effects::InitAll(ID3D11Device* device)
{
	BasicFX = new BasicEffect(device, L"FX/Basic.fxo");
	TreeSpriteFX = new TreeSpriteEffect(device, L"FX/TreeSprite.fxo");
	VecAddFX = new VecAddEffect(device, L"FX/VecAdd.fxo");
	BlurFX = new BlurEffect(device, L"FX/Blur.fxo");
	TessellationFX = new TessellationEffect(device, L"FX/Tessellation.fxo");
}

void Effects::DestroyAll()
{
	SafeDelete(BasicFX);
	SafeDelete(TreeSpriteFX);
	SafeDelete(VecAddFX);
	SafeDelete(BlurFX);
	SafeDelete(TessellationFX);
}
#pragma endregion