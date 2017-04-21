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

	Light1ReflectTech    = mFx->GetTechniqueByName("Light1Reflect");
	Light2ReflectTech    = mFx->GetTechniqueByName("Light2Reflect");
	Light3ReflectTech    = mFx->GetTechniqueByName("Light3Reflect");

	Light0TexReflectTech = mFx->GetTechniqueByName("Light0TexReflect");
	Light1TexReflectTech = mFx->GetTechniqueByName("Light1TexReflect");
	Light2TexReflectTech = mFx->GetTechniqueByName("Light2TexReflect");
	Light3TexReflectTech = mFx->GetTechniqueByName("Light3TexReflect");

	Light0TexAlphaClipReflectTech = mFx->GetTechniqueByName("Light0TexAlphaClipReflect");
	Light1TexAlphaClipReflectTech = mFx->GetTechniqueByName("Light1TexAlphaClipReflect");
	Light2TexAlphaClipReflectTech = mFx->GetTechniqueByName("Light2TexAlphaClipReflect");
	Light3TexAlphaClipReflectTech = mFx->GetTechniqueByName("Light3TexAlphaClipReflect");

	Light1FogReflectTech    = mFx->GetTechniqueByName("Light1FogReflect");
	Light2FogReflectTech    = mFx->GetTechniqueByName("Light2FogReflect");
	Light3FogReflectTech    = mFx->GetTechniqueByName("Light3FogReflect");

	Light0TexFogReflectTech = mFx->GetTechniqueByName("Light0TexFogReflect");
	Light1TexFogReflectTech = mFx->GetTechniqueByName("Light1TexFogReflect");
	Light2TexFogReflectTech = mFx->GetTechniqueByName("Light2TexFogReflect");
	Light3TexFogReflectTech = mFx->GetTechniqueByName("Light3TexFogReflect");

	Light0TexAlphaClipFogReflectTech = mFx->GetTechniqueByName("Light0TexAlphaClipFogReflect");
	Light1TexAlphaClipFogReflectTech = mFx->GetTechniqueByName("Light1TexAlphaClipFogReflect");
	Light2TexAlphaClipFogReflectTech = mFx->GetTechniqueByName("Light2TexAlphaClipFogReflect");
	Light3TexAlphaClipFogReflectTech = mFx->GetTechniqueByName("Light3TexAlphaClipFogReflect");


	WorldViewProj = mFx->GetVariableByName("gWorldViewProj")->AsMatrix();
	World = mFx->GetVariableByName("gWorld")->AsMatrix();
	WorldInvTranspose = mFx->GetVariableByName("gWorldInvTranspose")->AsMatrix();
	ShadowTransform = mFx->GetVariableByName("gShadowTransform")->AsMatrix();
	TexTransform = mFx->GetVariableByName("gTexTransform")->AsMatrix();
	EyePosW = mFx->GetVariableByName("gEyePosW")->AsVector();
	FogColor = mFx->GetVariableByName("gFogColor")->AsVector();
	FogStart = mFx->GetVariableByName("gFogStart")->AsScalar();
	FogRange = mFx->GetVariableByName("gFogRange")->AsScalar();
	DirLights = mFx->GetVariableByName("gDirLights");
	Mat = mFx->GetVariableByName("gMaterial");
	DiffuseMap = mFx->GetVariableByName("gDiffuseMap")->AsShaderResource();
	ShadowMap = mFx->GetVariableByName("gShadowMap")->AsShaderResource();
	CubeMap = mFx->GetVariableByName("gCubeMap")->AsShaderResource();
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

#pragma region SkyEffect
SkyEffect::SkyEffect(ID3D11Device* device, const std::wstring& filename)
	: Effect(device, filename)
{
	SkyTech = mFx->GetTechniqueByName("SkyTech");
	WorldViewProj = mFx->GetVariableByName("gWorldViewProj")->AsMatrix();
	CubeMap = mFx->GetVariableByName("gCubeMap")->AsShaderResource();
}

SkyEffect::~SkyEffect()
{
}

#pragma endregion

#pragma region NormalMapEffect
NormalMapEffect::NormalMapEffect(ID3D11Device* device, const std::wstring& filename)
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

	Light1ReflectTech = mFx->GetTechniqueByName("Light1Reflect");
	Light2ReflectTech = mFx->GetTechniqueByName("Light2Reflect");
	Light3ReflectTech = mFx->GetTechniqueByName("Light3Reflect");

	Light0TexReflectTech = mFx->GetTechniqueByName("Light0TexReflect");
	Light1TexReflectTech = mFx->GetTechniqueByName("Light1TexReflect");
	Light2TexReflectTech = mFx->GetTechniqueByName("Light2TexReflect");
	Light3TexReflectTech = mFx->GetTechniqueByName("Light3TexReflect");

	Light0TexAlphaClipReflectTech = mFx->GetTechniqueByName("Light0TexAlphaClipReflect");
	Light1TexAlphaClipReflectTech = mFx->GetTechniqueByName("Light1TexAlphaClipReflect");
	Light2TexAlphaClipReflectTech = mFx->GetTechniqueByName("Light2TexAlphaClipReflect");
	Light3TexAlphaClipReflectTech = mFx->GetTechniqueByName("Light3TexAlphaClipReflect");

	Light1FogReflectTech = mFx->GetTechniqueByName("Light1FogReflect");
	Light2FogReflectTech = mFx->GetTechniqueByName("Light2FogReflect");
	Light3FogReflectTech = mFx->GetTechniqueByName("Light3FogReflect");

	Light0TexFogReflectTech = mFx->GetTechniqueByName("Light0TexFogReflect");
	Light1TexFogReflectTech = mFx->GetTechniqueByName("Light1TexFogReflect");
	Light2TexFogReflectTech = mFx->GetTechniqueByName("Light2TexFogReflect");
	Light3TexFogReflectTech = mFx->GetTechniqueByName("Light3TexFogReflect");

	Light0TexAlphaClipFogReflectTech = mFx->GetTechniqueByName("Light0TexAlphaClipFogReflect");
	Light1TexAlphaClipFogReflectTech = mFx->GetTechniqueByName("Light1TexAlphaClipFogReflect");
	Light2TexAlphaClipFogReflectTech = mFx->GetTechniqueByName("Light2TexAlphaClipFogReflect");
	Light3TexAlphaClipFogReflectTech = mFx->GetTechniqueByName("Light3TexAlphaClipFogReflect");

	WorldViewProj = mFx->GetVariableByName("gWorldViewProj")->AsMatrix();
	World = mFx->GetVariableByName("gWorld")->AsMatrix();
	WorldInvTranspose = mFx->GetVariableByName("gWorldInvTranspose")->AsMatrix();
	ShadowTransform = mFx->GetVariableByName("gShadowTransform")->AsMatrix();
	TexTransform = mFx->GetVariableByName("gTexTransform")->AsMatrix();
	EyePosW = mFx->GetVariableByName("gEyePosW")->AsVector();
	FogColor = mFx->GetVariableByName("gFogColor")->AsVector();
	FogStart = mFx->GetVariableByName("gFogStart")->AsScalar();
	FogRange = mFx->GetVariableByName("gFogRange")->AsScalar();
	DirLights = mFx->GetVariableByName("gDirLights");
	Mat = mFx->GetVariableByName("gMaterial");
	DiffuseMap = mFx->GetVariableByName("gDiffuseMap")->AsShaderResource();
	CubeMap = mFx->GetVariableByName("gCubeMap")->AsShaderResource();
	ShadowMap = mFx->GetVariableByName("gShadowMap")->AsShaderResource();
	NormalMap = mFx->GetVariableByName("gNormalMap")->AsShaderResource();
}

NormalMapEffect::~NormalMapEffect()
{
}

#pragma endregion

#pragma region DisplacementMapEffect
DisplacementMapEffect::DisplacementMapEffect(ID3D11Device* device, const std::wstring& filename)
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

	Light1ReflectTech = mFx->GetTechniqueByName("Light1Reflect");
	Light2ReflectTech = mFx->GetTechniqueByName("Light2Reflect");
	Light3ReflectTech = mFx->GetTechniqueByName("Light3Reflect");

	Light0TexReflectTech = mFx->GetTechniqueByName("Light0TexReflect");
	Light1TexReflectTech = mFx->GetTechniqueByName("Light1TexReflect");
	Light2TexReflectTech = mFx->GetTechniqueByName("Light2TexReflect");
	Light3TexReflectTech = mFx->GetTechniqueByName("Light3TexReflect");

	Light0TexAlphaClipReflectTech = mFx->GetTechniqueByName("Light0TexAlphaClipReflect");
	Light1TexAlphaClipReflectTech = mFx->GetTechniqueByName("Light1TexAlphaClipReflect");
	Light2TexAlphaClipReflectTech = mFx->GetTechniqueByName("Light2TexAlphaClipReflect");
	Light3TexAlphaClipReflectTech = mFx->GetTechniqueByName("Light3TexAlphaClipReflect");

	Light1FogReflectTech = mFx->GetTechniqueByName("Light1FogReflect");
	Light2FogReflectTech = mFx->GetTechniqueByName("Light2FogReflect");
	Light3FogReflectTech = mFx->GetTechniqueByName("Light3FogReflect");

	Light0TexFogReflectTech = mFx->GetTechniqueByName("Light0TexFogReflect");
	Light1TexFogReflectTech = mFx->GetTechniqueByName("Light1TexFogReflect");
	Light2TexFogReflectTech = mFx->GetTechniqueByName("Light2TexFogReflect");
	Light3TexFogReflectTech = mFx->GetTechniqueByName("Light3TexFogReflect");

	Light0TexAlphaClipFogReflectTech = mFx->GetTechniqueByName("Light0TexAlphaClipFogReflect");
	Light1TexAlphaClipFogReflectTech = mFx->GetTechniqueByName("Light1TexAlphaClipFogReflect");
	Light2TexAlphaClipFogReflectTech = mFx->GetTechniqueByName("Light2TexAlphaClipFogReflect");
	Light3TexAlphaClipFogReflectTech = mFx->GetTechniqueByName("Light3TexAlphaClipFogReflect");

	ViewProj = mFx->GetVariableByName("gViewProj")->AsMatrix();
	WorldViewProj = mFx->GetVariableByName("gWorldViewProj")->AsMatrix();
	World = mFx->GetVariableByName("gWorld")->AsMatrix();
	WorldInvTranspose = mFx->GetVariableByName("gWorldInvTranspose")->AsMatrix();
	ShadowTransform = mFx->GetVariableByName("gShadowTransform")->AsMatrix();
	TexTransform = mFx->GetVariableByName("gTexTransform")->AsMatrix();
	EyePosW = mFx->GetVariableByName("gEyePosW")->AsVector();
	FogColor = mFx->GetVariableByName("gFogColor")->AsVector();
	FogStart = mFx->GetVariableByName("gFogStart")->AsScalar();
	FogRange = mFx->GetVariableByName("gFogRange")->AsScalar();
	DirLights = mFx->GetVariableByName("gDirLights");
	Mat = mFx->GetVariableByName("gMaterial");
	HeightScale = mFx->GetVariableByName("gHeightScale")->AsScalar();
	MaxTessDistance = mFx->GetVariableByName("gMaxTessDistance")->AsScalar();
	MinTessDistance = mFx->GetVariableByName("gMinTessDistance")->AsScalar();
	MinTessFactor = mFx->GetVariableByName("gMinTessFactor")->AsScalar();
	MaxTessFactor = mFx->GetVariableByName("gMaxTessFactor")->AsScalar();
	DiffuseMap = mFx->GetVariableByName("gDiffuseMap")->AsShaderResource();
	CubeMap = mFx->GetVariableByName("gCubeMap")->AsShaderResource();
	ShadowMap = mFx->GetVariableByName("gShadowMap")->AsShaderResource();
	NormalMap = mFx->GetVariableByName("gNormalMap")->AsShaderResource();
}

DisplacementMapEffect::~DisplacementMapEffect()
{
}
#pragma endregion

#pragma region TerrainEffect
TerrainEffect::TerrainEffect(ID3D11Device* device, const std::wstring& filename)
	: Effect(device, filename)
{
	Light1Tech = mFx->GetTechniqueByName("Light1");
	Light2Tech = mFx->GetTechniqueByName("Light2");
	Light3Tech = mFx->GetTechniqueByName("Light3");
	Light1FogTech = mFx->GetTechniqueByName("Light1Fog");
	Light2FogTech = mFx->GetTechniqueByName("Light2Fog");
	Light3FogTech = mFx->GetTechniqueByName("Light3Fog");

	ViewProj = mFx->GetVariableByName("gViewProj")->AsMatrix();
	EyePosW = mFx->GetVariableByName("gEyePosW")->AsVector();
	FogColor = mFx->GetVariableByName("gFogColor")->AsVector();
	FogStart = mFx->GetVariableByName("gFogStart")->AsScalar();
	FogRange = mFx->GetVariableByName("gFogRange")->AsScalar();
	DirLights = mFx->GetVariableByName("gDirLights");
	Mat = mFx->GetVariableByName("gMaterial");

	MinDist = mFx->GetVariableByName("gMinDist")->AsScalar();
	MaxDist = mFx->GetVariableByName("gMaxDist")->AsScalar();
	MinTess = mFx->GetVariableByName("gMinTess")->AsScalar();
	MaxTess = mFx->GetVariableByName("gMaxTess")->AsScalar();
	TexelCellSpaceU = mFx->GetVariableByName("gTexelCellSpaceU")->AsScalar();
	TexelCellSpaceV = mFx->GetVariableByName("gTexelCellSpaceV")->AsScalar();
	WorldCellSpace = mFx->GetVariableByName("gWorldCellSpace")->AsScalar();
	WorldFrustumPlanes = mFx->GetVariableByName("gWorldFrustumPlanes")->AsVector();

	LayerMapArray = mFx->GetVariableByName("gLayerMapArray")->AsShaderResource();
	BlendMap = mFx->GetVariableByName("gBlendMap")->AsShaderResource();
	HeightMap = mFx->GetVariableByName("gHeightMap")->AsShaderResource();
}

TerrainEffect::~TerrainEffect()
{
}
#pragma endregion

#pragma region BuildShadowMapEffect
BuildShadowMapEffect::BuildShadowMapEffect(ID3D11Device* device, const std::wstring& filename)
	: Effect(device, filename)
{
	BuildShadowMapTech = mFx->GetTechniqueByName("BuildShadowMapTech");
	BuildShadowMapAlphaClipTech = mFx->GetTechniqueByName("BuildShadowMapAlphaClipTech");

	TessBuildShadowMapTech = mFx->GetTechniqueByName("TessBuildShadowMapTech");
	TessBuildShadowMapAlphaClipTech = mFx->GetTechniqueByName("TessBuildShadowMapAlphaClipTech");

	ViewProj = mFx->GetVariableByName("gViewProj")->AsMatrix();
	WorldViewProj = mFx->GetVariableByName("gWorldViewProj")->AsMatrix();
	World = mFx->GetVariableByName("gWorld")->AsMatrix();
	WorldInvTranspose = mFx->GetVariableByName("gWorldInvTranspose")->AsMatrix();
	TexTransform = mFx->GetVariableByName("gTexTransform")->AsMatrix();
	EyePosW = mFx->GetVariableByName("gEyePosW")->AsVector();
	HeightScale = mFx->GetVariableByName("gHeightScale")->AsScalar();
	MaxTessDistance = mFx->GetVariableByName("gMaxTessDistance")->AsScalar();
	MinTessDistance = mFx->GetVariableByName("gMinTessDistance")->AsScalar();
	MinTessFactor = mFx->GetVariableByName("gMinTessFactor")->AsScalar();
	MaxTessFactor = mFx->GetVariableByName("gMaxTessFactor")->AsScalar();
	DiffuseMap = mFx->GetVariableByName("gDiffuseMap")->AsShaderResource();
	NormalMap = mFx->GetVariableByName("gNormalMap")->AsShaderResource();
}

BuildShadowMapEffect::~BuildShadowMapEffect()
{
}
#pragma endregion

#pragma region DebugTexEffect
DebugTexEffect::DebugTexEffect(ID3D11Device* device, const std::wstring& filename)
	: Effect(device, filename)
{
	ViewArgbTech = mFx->GetTechniqueByName("ViewArgbTech");
	ViewRedTech = mFx->GetTechniqueByName("ViewRedTech");
	ViewGreenTech = mFx->GetTechniqueByName("ViewGreenTech");
	ViewBlueTech = mFx->GetTechniqueByName("ViewBlueTech");
	ViewAlphaTech = mFx->GetTechniqueByName("ViewAlphaTech");

	WorldViewProj = mFx->GetVariableByName("gWorldViewProj")->AsMatrix();
	Texture = mFx->GetVariableByName("gTexture")->AsShaderResource();
}

DebugTexEffect::~DebugTexEffect()
{

}

#pragma endregion

#pragma region Effects
BasicEffect* Effects::BasicFX = nullptr;
TreeSpriteEffect* Effects::TreeSpriteFX = nullptr;
SkyEffect* Effects::SkyFX = nullptr;
NormalMapEffect* Effects::NormalMapFX = nullptr;
DisplacementMapEffect* Effects::DisplacementMapFX = nullptr;
TerrainEffect* Effects::TerrainFX = nullptr;
BuildShadowMapEffect* Effects::BuildShadowMapFX = nullptr;
DebugTexEffect* Effects::DebugTexFX = nullptr;

void Effects::InitAll(ID3D11Device* device)
{
	BasicFX = new BasicEffect(device, L"FX/Basic.fxo");
	TreeSpriteFX = new TreeSpriteEffect(device, L"FX/TreeSprite.fxo");
	SkyFX = new SkyEffect(device, L"FX/Sky.fxo");
	NormalMapFX = new NormalMapEffect(device, L"FX/NormalMap.fxo");
	DisplacementMapFX = new DisplacementMapEffect(device, L"FX/DisplacementMap.fxo");
	TerrainFX = new TerrainEffect(device, L"FX/Terrain.fxo");
	BuildShadowMapFX = new BuildShadowMapEffect(device, L"FX/BuildShadowMap.fxo");
	DebugTexFX = new DebugTexEffect(device, L"FX/DebugTexture.fxo");
}

void Effects::DestroyAll()
{
	SafeDelete(BasicFX);
	SafeDelete(TreeSpriteFX);
	SafeDelete(SkyFX);
	SafeDelete(NormalMapFX);
	SafeDelete(DisplacementMapFX);
	SafeDelete(TerrainFX);
	SafeDelete(BuildShadowMapFX);
	SafeDelete(DebugTexFX);
}
#pragma endregion