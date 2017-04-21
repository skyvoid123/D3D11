#include "d3dApp.h"
#include "d3dx11Effect.h"
#include "GeometryGenerator.h"
#include "MathHelper.h"
#include "LightHelper.h"
#include "DDSTextureLoader.h"
#include "Effects.h"
#include "Vertex.h"
#include "RenderStates.h"
#include "Sky.h"
#include "ShadowMap.h"
#include "Camera.h"

enum RenderOptions
{
	RenderOptionsBasic = 0,
	RenderOptionsNormalMap = 1,
	RenderOptionsDisplacementMap = 2
};

struct BoundingSphere
{
	BoundingSphere() : m_Center(0.0f, 0.0f, 0.0f), m_Radius(0.0f) {}
	XMFLOAT3 m_Center;
	float m_Radius;
};

class ShadowsApp : public D3DApp
{
public:
	ShadowsApp(HINSTANCE hInstance);
	~ShadowsApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

	void OnMouseDown(WPARAM btnState, int x, int y);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y);

private:
	void DrawSceneToShadowMap();
	void DrawScreenQuad();
	void BuildShadowTransform();
	void BuildShapeGeometryBuffers();
	void BuildSkullGeometryBuffers();
	void BuildScreenQuadGeometryBuffers();

private:
	ID3D11Buffer* m_ShapesVB;
	ID3D11Buffer* m_ShapesIB;

	ID3D11Buffer* m_SkullVB;
	ID3D11Buffer* m_SkullIB;

	ID3D11Buffer* m_ScreenQuadVB;
	ID3D11Buffer* m_ScreenQuadIB;

	ID3D11ShaderResourceView* m_FloorTexSRV;
	ID3D11ShaderResourceView* m_StoneTexSRV;
	ID3D11ShaderResourceView* m_BrickTexSRV;

	ID3D11ShaderResourceView* m_FloorNormalTexSRV;
	ID3D11ShaderResourceView* m_StoneNormalTexSRV;
	ID3D11ShaderResourceView* m_BrickNormalTexSRV;

	BoundingSphere m_SceneBounds;

	static const int SHADOW_MAP_SIZE = 2048;
	ShadowMap* m_ShadowMap;
	XMFLOAT4X4 m_LightView;
	XMFLOAT4X4 m_LightProj;
	XMFLOAT4X4 m_ShadowTransform;

	float m_LightRotationAngle;
	XMFLOAT3 m_OriginalLightDir[3];
	DirectionalLight m_DirLights[3];

	Material m_GridMat;
	Material m_BoxMat;
	Material m_CylinderMat;
	Material m_SphereMat;
	Material m_SkullMat;

	// Define transformations from local spaces to world space.
	XMFLOAT4X4 m_SphereWorld[10];
	XMFLOAT4X4 m_CylWorld[10];
	XMFLOAT4X4 m_BoxWorld;
	XMFLOAT4X4 m_GridWorld;
	XMFLOAT4X4 m_SkullWorld;

	int m_BoxVertexOffset;
	int m_GridVertexOffset;
	int m_SphereVertexOffset;
	int m_CylinderVertexOffset;

	UINT m_BoxIndexOffset;
	UINT m_GridIndexOffset;
	UINT m_SphereIndexOffset;
	UINT m_CylinderIndexOffset;

	UINT m_BoxIndexCount;
	UINT m_GridIndexCount;
	UINT m_SphereIndexCount;
	UINT m_CylinderIndexCount;

	UINT m_SkullIndexCount;

	RenderOptions m_RenderOption;

	Sky* m_Skys[3];
	Sky* m_CurrentSky;

	Camera m_Camera;

	POINT m_LastMousePos;
};

ShadowsApp::ShadowsApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
	, m_ShapesVB(nullptr)
	, m_ShapesIB(nullptr)
	, m_SkullVB(nullptr)
	, m_SkullIB(nullptr)
	, m_ScreenQuadVB(nullptr)
	, m_ScreenQuadIB(nullptr)
	, m_FloorTexSRV(nullptr)
	, m_StoneTexSRV(nullptr)
	, m_BrickTexSRV(nullptr)
	, m_FloorNormalTexSRV(nullptr)
	, m_StoneNormalTexSRV(nullptr)
	, m_BrickNormalTexSRV(nullptr)
	, m_ShadowMap(nullptr)
	, m_LightRotationAngle(0.0f)
	, m_CurrentSky(nullptr)
	, m_RenderOption(RenderOptionsBasic)
{
	main_wnd_caption_ = L"Shadows Demo";

	// Estimate the scene bounding sphere manually since we know how the scene was constructed.
	// The grid is the "widest object" with a width of 20 and depth of 30.0f, and centered at
	// the world space origin.  In general, you need to loop over every world space vertex
	// position and compute the bounding sphere.
	m_SceneBounds.m_Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_SceneBounds.m_Radius = sqrtf(10.0f * 10.0f + 15.0f * 15.0f);

	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&m_GridWorld, I);

	XMMATRIX boxScale = XMMatrixScaling(3.0f, 1.0f, 3.0f);
	XMMATRIX boxOffset = XMMatrixTranslation(0.0f, 0.5f, 0.0f);
	XMStoreFloat4x4(&m_BoxWorld, XMMatrixMultiply(boxScale, boxOffset));

	XMMATRIX skullScale = XMMatrixScaling(0.5f, 0.5f, 0.5f);
	XMMATRIX skullOffset = XMMatrixTranslation(0.0f, 1.0f, 0.0f);
	XMStoreFloat4x4(&m_SkullWorld, XMMatrixMultiply(skullScale, skullOffset));

	for (int i = 0; i < 5; ++i)
	{
		XMStoreFloat4x4(&m_CylWorld[i * 2 + 0], XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i*5.0f));
		XMStoreFloat4x4(&m_CylWorld[i * 2 + 1], XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i*5.0f));

		XMStoreFloat4x4(&m_SphereWorld[i * 2 + 0], XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i*5.0f));
		XMStoreFloat4x4(&m_SphereWorld[i * 2 + 1], XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i*5.0f));
	}

	m_DirLights[0].Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_DirLights[0].Diffuse = XMFLOAT4(0.7f, 0.7f, 0.6f, 1.0f);
	m_DirLights[0].Specular = XMFLOAT4(0.8f, 0.8f, 0.7f, 1.0f);
	m_DirLights[0].Direction = XMFLOAT3(-0.57735f, -0.57735f, 0.57735f);

	m_DirLights[1].Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	m_DirLights[1].Diffuse = XMFLOAT4(0.40f, 0.40f, 0.40f, 1.0f);
	m_DirLights[1].Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_DirLights[1].Direction = XMFLOAT3(0.707f, -0.707f, 0.0f);

	m_DirLights[2].Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	m_DirLights[2].Diffuse = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_DirLights[2].Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_DirLights[2].Direction = XMFLOAT3(0.0f, 0.0, -1.0f);

	m_OriginalLightDir[0] = m_DirLights[0].Direction;
	m_OriginalLightDir[1] = m_DirLights[1].Direction;
	m_OriginalLightDir[2] = m_DirLights[2].Direction;

	m_GridMat.Ambient = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	m_GridMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_GridMat.Specular = XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);
	m_GridMat.Reflect = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

	m_CylinderMat.Ambient = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_CylinderMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_CylinderMat.Specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 32.0f);
	m_CylinderMat.Reflect = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

	m_SphereMat.Ambient = XMFLOAT4(0.2f, 0.3f, 0.4f, 1.0f);
	m_SphereMat.Diffuse = XMFLOAT4(0.2f, 0.3f, 0.4f, 1.0f);
	m_SphereMat.Specular = XMFLOAT4(0.9f, 0.9f, 0.9f, 16.0f);
	m_SphereMat.Reflect = XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);

	m_BoxMat.Ambient = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_BoxMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_BoxMat.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
	m_BoxMat.Reflect = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

	m_SkullMat.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_SkullMat.Diffuse = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_SkullMat.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
	m_SkullMat.Reflect = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
}

ShadowsApp::~ShadowsApp()
{
	d3d_context_->ClearState();
	ReleaseCOM(m_ShapesVB);
	ReleaseCOM(m_ShapesIB);
	ReleaseCOM(m_SkullVB);
	ReleaseCOM(m_SkullIB);
	ReleaseCOM(m_FloorTexSRV);
	ReleaseCOM(m_StoneTexSRV);
	ReleaseCOM(m_BrickTexSRV);
	ReleaseCOM(m_FloorNormalTexSRV);
	ReleaseCOM(m_StoneNormalTexSRV);
	ReleaseCOM(m_BrickNormalTexSRV);

	for (int i = 0; i < 3; ++i)
	{
		SafeDelete(m_Skys[i]);
	}

	SafeDelete(m_ShadowMap);

	Effects::DestroyAll();
	InputLayouts::DestroyAll();
	RenderStates::DestroyAll();
}

bool ShadowsApp::Init()
{
	if (!D3DApp::Init())
	{
		return false;
	}

	Effects::InitAll(d3d_device_);
	InputLayouts::InitAll(d3d_device_);
	RenderStates::InitAll(d3d_device_);

	ID3D11Resource* texRes = nullptr;
	HR(DirectX::CreateDDSTextureFromFile(d3d_device_,
		L"Textures/floor.dds", &texRes, &m_FloorTexSRV));
	ReleaseCOM(texRes);

	HR(DirectX::CreateDDSTextureFromFile(d3d_device_,
		L"Textures/stone.dds", &texRes, &m_StoneTexSRV));
	ReleaseCOM(texRes);

	HR(DirectX::CreateDDSTextureFromFile(d3d_device_,
		L"Textures/bricks.dds", &texRes, &m_BrickTexSRV));
	ReleaseCOM(texRes);

	HR(DirectX::CreateDDSTextureFromFile(d3d_device_,
		L"Textures/floor_nmap.dds", &texRes, &m_FloorNormalTexSRV));
	ReleaseCOM(texRes);

	HR(DirectX::CreateDDSTextureFromFile(d3d_device_,
		L"Textures/stones_nmap.dds", &texRes, &m_StoneNormalTexSRV));
	ReleaseCOM(texRes);

	HR(DirectX::CreateDDSTextureFromFile(d3d_device_,
		L"Textures/bricks_nmap.dds", &texRes, &m_BrickNormalTexSRV));
	ReleaseCOM(texRes);

	BuildShapeGeometryBuffers();
	BuildSkullGeometryBuffers();
	BuildScreenQuadGeometryBuffers();

	m_Skys[0] = new Sky(d3d_device_, L"Textures/grasscube1024.dds", 5000.f);
	m_Skys[1] = new Sky(d3d_device_, L"Textures/snowcube1024.dds", 5000.f);
	m_Skys[2] = new Sky(d3d_device_, L"Textures/sunsetcube1024.dds", 5000.f);
	m_CurrentSky = m_Skys[0];

	m_ShadowMap = new ShadowMap(d3d_device_, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);

	return true;
}

void ShadowsApp::OnResize()
{
	D3DApp::OnResize();

	m_Camera.SetLens(.25f * MathHelper::Pi, AspectRatio(), 1.f, 1000.f);
}

void ShadowsApp::UpdateScene(float dt)
{
	//
	// Switch the sky based on key presses.
	//
	if (GetAsyncKeyState('1') & 0x8000)
		m_CurrentSky = m_Skys[0];

	if (GetAsyncKeyState('2') & 0x8000)
		m_CurrentSky = m_Skys[1];

	if (GetAsyncKeyState('3') & 0x8000)
		m_CurrentSky = m_Skys[2];

	//
	// Control the camera.
	//
	if (GetAsyncKeyState('W') & 0x8000)
		m_Camera.Walk(20.0f * dt);

	if (GetAsyncKeyState('S') & 0x8000)
		m_Camera.Walk(-20.0f * dt);

	if (GetAsyncKeyState('A') & 0x8000)
		m_Camera.Strafe(-20.0f * dt);

	if (GetAsyncKeyState('D') & 0x8000)
		m_Camera.Strafe(20.0f * dt);

	//
	// Switch the rendering effect based on key presses.
	//
	if (GetAsyncKeyState('7') & 0x8000)
		m_RenderOption = RenderOptionsBasic;

	if (GetAsyncKeyState('8') & 0x8000)
		m_RenderOption = RenderOptionsNormalMap;

	if (GetAsyncKeyState('9') & 0x8000)
		m_RenderOption = RenderOptionsDisplacementMap;

	//
	// Animate the lights (and hence shadows).
	//
	m_LightRotationAngle += 0.1f * dt;
	XMMATRIX R = XMMatrixRotationY(m_LightRotationAngle);
	for (int i = 0; i < 3; ++i)
	{
		XMVECTOR lightDir = XMLoadFloat3(&m_OriginalLightDir[i]);
		lightDir = XMVector3TransformNormal(lightDir, R);
		XMStoreFloat3(&m_DirLights[i].Direction, lightDir);
	}

	BuildShadowTransform();
}

void ShadowsApp::DrawScene()
{
	m_ShadowMap->BindDsvAndSetNullRenderTarget(d3d_context_);
	DrawSceneToShadowMap();
	d3d_context_->RSSetState(nullptr);

	//
	// Restore the back and depth buffer to the OM stage.
	//
	d3d_context_->OMSetRenderTargets(1, &render_target_view_, depth_stencil_view_);
	d3d_context_->RSSetViewports(1, &screen_viewport_);
	d3d_context_->ClearRenderTargetView(render_target_view_, (const float*)&Colors::Silver);
	d3d_context_->ClearDepthStencilView(depth_stencil_view_, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

	m_Camera.UpdateViewMatrix();
	XMMATRIX viewProj = m_Camera.ViewProj();

	// Set per frame constants.
	Effects::BasicFX->SetDirLights(m_DirLights);
	Effects::BasicFX->SetEyePosW(m_Camera.GetPosition());
	Effects::BasicFX->SetCubeMap(m_CurrentSky->GetCubeMapSRV());
	Effects::BasicFX->SetShadowMap(m_ShadowMap->DepthMapSRV());

	Effects::NormalMapFX->SetDirLights(m_DirLights);
	Effects::NormalMapFX->SetEyePosW(m_Camera.GetPosition());
	Effects::NormalMapFX->SetCubeMap(m_CurrentSky->GetCubeMapSRV());
	Effects::NormalMapFX->SetShadowMap(m_ShadowMap->DepthMapSRV());

	Effects::DisplacementMapFX->SetDirLights(m_DirLights);
	Effects::DisplacementMapFX->SetEyePosW(m_Camera.GetPosition());
	Effects::DisplacementMapFX->SetCubeMap(m_CurrentSky->GetCubeMapSRV());
	Effects::DisplacementMapFX->SetShadowMap(m_ShadowMap->DepthMapSRV());

	// These properties could be set per object if needed.
	Effects::DisplacementMapFX->SetHeightScale(0.07f);
	Effects::DisplacementMapFX->SetMaxTessDistance(1.0f);
	Effects::DisplacementMapFX->SetMinTessDistance(25.0f);
	Effects::DisplacementMapFX->SetMinTessFactor(1.0f);
	Effects::DisplacementMapFX->SetMaxTessFactor(5.0f);

	// Figure out which technique to use for different geometry.
	ID3DX11EffectTechnique* activeTech = nullptr;
	ID3DX11EffectTechnique* activeSphereTech = Effects::BasicFX->Light3ReflectTech;
	ID3DX11EffectTechnique* activeSkullTech = Effects::BasicFX->Light3ReflectTech;

	switch (m_RenderOption)
	{
	case RenderOptionsBasic:
		activeTech = Effects::BasicFX->Light3TexTech;
		d3d_context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		break;
	case RenderOptionsNormalMap:
		activeTech = Effects::NormalMapFX->Light3TexTech;
		d3d_context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		break;
	case RenderOptionsDisplacementMap:
		activeTech = Effects::DisplacementMapFX->Light3TexTech;
		d3d_context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
		break;
	default:
		break;
	}

	XMMATRIX world;
	XMMATRIX worldInvTranspose;
	XMMATRIX worldViewProj;
	XMMATRIX shadowTransform = XMLoadFloat4x4(&m_ShadowTransform);

	//
	// Draw the grid, cylinders, and box without any cubemap reflection.
	// 
	UINT stride = sizeof(Vertex::PosNormalTexTan);
	UINT offset = 0;

	d3d_context_->IASetInputLayout(InputLayouts::PosNormalTexTan);
	d3d_context_->IASetVertexBuffers(0, 1, &m_ShapesVB, &stride, &offset);
	d3d_context_->IASetIndexBuffer(m_ShapesIB, DXGI_FORMAT_R32_UINT, 0);

	if (GetAsyncKeyState('F') & 0x8000)
		d3d_context_->RSSetState(RenderStates::WireframeRS);

	D3DX11_TECHNIQUE_DESC techDesc;
	activeTech->GetDesc(&techDesc);
	for (int p = 0; p < techDesc.Passes; ++p)
	{
		// Draw the grid.
		world = XMLoadFloat4x4(&m_GridWorld);
		worldInvTranspose = MathHelper::InverseTranspose(world);
		worldViewProj = world * viewProj;

		switch (m_RenderOption)
		{
		case RenderOptionsBasic:
			Effects::BasicFX->SetWorld(world);
			Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
			Effects::BasicFX->SetWorldViewProj(worldViewProj);
			Effects::BasicFX->SetShadowTransform(world * shadowTransform);
			Effects::BasicFX->SetTexTransform(XMMatrixScaling(8.0f, 10.0f, 1.0f));
			Effects::BasicFX->SetMaterial(m_GridMat);
			Effects::BasicFX->SetDiffuseMap(m_FloorTexSRV);
			break;
		case RenderOptionsNormalMap:
			Effects::NormalMapFX->SetWorld(world);
			Effects::NormalMapFX->SetWorldInvTranspose(worldInvTranspose);
			Effects::NormalMapFX->SetWorldViewProj(worldViewProj);
			Effects::NormalMapFX->SetShadowTransform(world * shadowTransform);
			Effects::NormalMapFX->SetTexTransform(XMMatrixScaling(8.0f, 10.0f, 1.0f));
			Effects::NormalMapFX->SetMaterial(m_GridMat);
			Effects::NormalMapFX->SetDiffuseMap(m_FloorTexSRV);
			Effects::NormalMapFX->SetNormalMap(m_FloorNormalTexSRV);
			break;
		case RenderOptionsDisplacementMap:
			Effects::DisplacementMapFX->SetWorld(world);
			Effects::DisplacementMapFX->SetWorldInvTranspose(worldInvTranspose);
			Effects::DisplacementMapFX->SetViewProj(viewProj);
			Effects::DisplacementMapFX->SetWorldViewProj(worldViewProj);
			
			// Note: No world pre-multiply for displacement mapping since the DS computes the world
			// space position, we just need the light view/proj.
			Effects::DisplacementMapFX->SetShadowTransform(shadowTransform);

			Effects::DisplacementMapFX->SetTexTransform(XMMatrixScaling(8.0f, 10.0f, 1.0f));
			Effects::DisplacementMapFX->SetMaterial(m_GridMat);
			Effects::DisplacementMapFX->SetDiffuseMap(m_FloorTexSRV);
			Effects::DisplacementMapFX->SetNormalMap(m_FloorNormalTexSRV);
			break;
		default:
			break;
		}

		activeTech->GetPassByIndex(p)->Apply(0, d3d_context_);
		d3d_context_->DrawIndexed(m_GridIndexCount, m_GridIndexOffset, m_GridVertexOffset);

		// Draw the box.
		world = XMLoadFloat4x4(&m_BoxWorld);
		worldInvTranspose = MathHelper::InverseTranspose(world);
		worldViewProj = world * viewProj;

		switch (m_RenderOption)
		{
		case RenderOptionsBasic:
			Effects::BasicFX->SetWorld(world);
			Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
			Effects::BasicFX->SetWorldViewProj(worldViewProj);
			Effects::BasicFX->SetShadowTransform(world * shadowTransform);
			Effects::BasicFX->SetTexTransform(XMMatrixScaling(2.0f, 1.0f, 1.0f));
			Effects::BasicFX->SetMaterial(m_BoxMat);
			Effects::BasicFX->SetDiffuseMap(m_BrickTexSRV);
			break;
		case RenderOptionsNormalMap:
			Effects::NormalMapFX->SetWorld(world);
			Effects::NormalMapFX->SetWorldInvTranspose(worldInvTranspose);
			Effects::NormalMapFX->SetWorldViewProj(worldViewProj);
			Effects::NormalMapFX->SetShadowTransform(world * shadowTransform);
			Effects::NormalMapFX->SetTexTransform(XMMatrixScaling(2.0f, 1.0f, 1.0f));
			Effects::NormalMapFX->SetMaterial(m_BoxMat);
			Effects::NormalMapFX->SetDiffuseMap(m_BrickTexSRV);
			Effects::NormalMapFX->SetNormalMap(m_BrickNormalTexSRV);
			break;
		case RenderOptionsDisplacementMap:
			Effects::DisplacementMapFX->SetWorld(world);
			Effects::DisplacementMapFX->SetWorldInvTranspose(worldInvTranspose);
			Effects::DisplacementMapFX->SetViewProj(viewProj);
			Effects::DisplacementMapFX->SetWorldViewProj(worldViewProj);
			
			// Note: No world pre-multiply for displacement mapping since the DS computes the world
			// space position, we just need the light view/proj.
			Effects::DisplacementMapFX->SetShadowTransform(shadowTransform);
			
			Effects::DisplacementMapFX->SetTexTransform(XMMatrixScaling(2.0f, 1.0f, 1.0f));
			Effects::DisplacementMapFX->SetMaterial(m_BoxMat);
			Effects::DisplacementMapFX->SetDiffuseMap(m_BrickTexSRV);
			Effects::DisplacementMapFX->SetNormalMap(m_BrickNormalTexSRV);
			break;
		default:
			break;
		}

		activeTech->GetPassByIndex(p)->Apply(0, d3d_context_);
		d3d_context_->DrawIndexed(m_BoxIndexCount, m_BoxIndexOffset, m_BoxVertexOffset);

		// Draw the cylinders.
		for (int i = 0; i < 10; ++i)
		{
			world = XMLoadFloat4x4(&m_CylWorld[i]);
			worldInvTranspose = MathHelper::InverseTranspose(world);
			worldViewProj = world * viewProj;

			switch (m_RenderOption)
			{
			case RenderOptionsBasic:
				Effects::BasicFX->SetWorld(world);
				Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
				Effects::BasicFX->SetWorldViewProj(worldViewProj);
				Effects::BasicFX->SetShadowTransform(world * shadowTransform);
				Effects::BasicFX->SetTexTransform(XMMatrixScaling(1.0f, 2.0f, 1.0f));
				Effects::BasicFX->SetMaterial(m_CylinderMat);
				Effects::BasicFX->SetDiffuseMap(m_BrickTexSRV);
				break;
			case RenderOptionsNormalMap:
				Effects::NormalMapFX->SetWorld(world);
				Effects::NormalMapFX->SetWorldInvTranspose(worldInvTranspose);
				Effects::NormalMapFX->SetWorldViewProj(worldViewProj);
				Effects::NormalMapFX->SetShadowTransform(world * shadowTransform);
				Effects::NormalMapFX->SetTexTransform(XMMatrixScaling(1.0f, 2.0f, 1.0f));
				Effects::NormalMapFX->SetMaterial(m_CylinderMat);
				Effects::NormalMapFX->SetDiffuseMap(m_BrickTexSRV);
				Effects::NormalMapFX->SetNormalMap(m_BrickNormalTexSRV);
				break;
			case RenderOptionsDisplacementMap:
				Effects::DisplacementMapFX->SetWorld(world);
				Effects::DisplacementMapFX->SetWorldInvTranspose(worldInvTranspose);
				Effects::DisplacementMapFX->SetViewProj(viewProj);
				Effects::DisplacementMapFX->SetWorldViewProj(worldViewProj);
				
				// Note: No world pre-multiply for displacement mapping since the DS computes the world
				// space position, we just need the light view/proj.
				Effects::DisplacementMapFX->SetShadowTransform(shadowTransform);

				Effects::DisplacementMapFX->SetTexTransform(XMMatrixScaling(1.0f, 2.0f, 1.0f));
				Effects::DisplacementMapFX->SetMaterial(m_CylinderMat);
				Effects::DisplacementMapFX->SetDiffuseMap(m_BrickTexSRV);
				Effects::DisplacementMapFX->SetNormalMap(m_BrickNormalTexSRV);
				break;
			default:
				break;
			}

			activeTech->GetPassByIndex(p)->Apply(0, d3d_context_);
			d3d_context_->DrawIndexed(m_CylinderIndexCount, m_CylinderIndexOffset, m_CylinderVertexOffset);
		}
	}

	// FX sets tessellation stages, but it does not disable them.  So do that here
	// to turn off tessellation.
	d3d_context_->HSSetShader(nullptr, nullptr, 0);
	d3d_context_->DSSetShader(nullptr, nullptr, 0);

	d3d_context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//
	// Draw the spheres with cubemap reflection.
	//
	activeSphereTech->GetDesc(&techDesc);
	for (int p = 0; p < techDesc.Passes; ++p)
	{
		// Draw the spheres.
		for (int i = 0; i < 10; ++i)
		{
			world = XMLoadFloat4x4(&m_SphereWorld[i]);
			worldInvTranspose = MathHelper::InverseTranspose(world);
			worldViewProj = world * viewProj;

			Effects::BasicFX->SetWorld(world);
			Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
			Effects::BasicFX->SetWorldViewProj(worldViewProj);
			Effects::BasicFX->SetShadowTransform(world * shadowTransform);
			Effects::BasicFX->SetTexTransform(XMMatrixIdentity());
			Effects::BasicFX->SetMaterial(m_SphereMat);
			//Effects::BasicFX->SetDiffuseMap(m_StoneTexSRV);

			activeSphereTech->GetPassByIndex(p)->Apply(0, d3d_context_);
			d3d_context_->DrawIndexed(m_SphereIndexCount, m_SphereIndexOffset, m_SphereVertexOffset);
		}
	}

	//
	// Draw the Skull with cubemap reflection.
	//
	stride = sizeof(Vertex::Basic32);
	offset = 0;

	d3d_context_->RSSetState(nullptr);
	d3d_context_->IASetInputLayout(InputLayouts::Basic32);
	d3d_context_->IASetVertexBuffers(0, 1, &m_SkullVB, &stride, &offset);
	d3d_context_->IASetIndexBuffer(m_SkullIB, DXGI_FORMAT_R32_UINT, 0);

	activeSkullTech->GetDesc(&techDesc);
	for (int p = 0; p < techDesc.Passes; ++p)
	{
		world = XMLoadFloat4x4(&m_SkullWorld);
		worldInvTranspose = MathHelper::InverseTranspose(world);
		worldViewProj = world * viewProj;

		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(worldViewProj);
		Effects::BasicFX->SetShadowTransform(world*shadowTransform);
		Effects::BasicFX->SetTexTransform(XMMatrixIdentity());
		Effects::BasicFX->SetMaterial(m_SkullMat);

		activeSkullTech->GetPassByIndex(p)->Apply(0, d3d_context_);
		d3d_context_->DrawIndexed(m_SkullIndexCount, 0, 0);
	}


	// Debug view depth buffer.
	DrawScreenQuad();

	m_CurrentSky->Draw(d3d_context_, m_Camera);

	// restore default states, as the SkyFX changes them in the effect file.
	d3d_context_->RSSetState(nullptr);
	d3d_context_->OMSetDepthStencilState(nullptr, 0);

	// Unbind shadow map as a shader input because we are going to render to it next frame.
	// The shadow might might be at any slot, so clear all slots.
	ID3D11ShaderResourceView* nullSRV[16] = { nullptr };
	d3d_context_->PSSetShaderResources(0, 16, nullSRV);

	HR(swap_chain_->Present(0, 0));
}

void ShadowsApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	m_LastMousePos.x = x;
	m_LastMousePos.y = y;

	SetCapture(main_wnd_);
}

void ShadowsApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void ShadowsApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f * (float)(x - m_LastMousePos.x));
		float dy = XMConvertToRadians(0.25f * (float)(y - m_LastMousePos.y));

		m_Camera.Pitch(dy);
		m_Camera.RotateY(dx);
	}

	m_LastMousePos.x = x;
	m_LastMousePos.y = y;
}

void ShadowsApp::DrawSceneToShadowMap()
{
	XMMATRIX view = XMLoadFloat4x4(&m_LightView);
	XMMATRIX proj = XMLoadFloat4x4(&m_LightProj);
	XMMATRIX viewProj = view * proj;

	Effects::BuildShadowMapFX->SetEyePosW(m_Camera.GetPosition());
	Effects::BuildShadowMapFX->SetViewProj(viewProj);

	// These properties could be set per object if needed.
	Effects::BuildShadowMapFX->SetHeightScale(0.07f);
	Effects::BuildShadowMapFX->SetMaxTessDistance(1.0f);
	Effects::BuildShadowMapFX->SetMinTessDistance(25.0f);
	Effects::BuildShadowMapFX->SetMinTessFactor(1.0f);
	Effects::BuildShadowMapFX->SetMaxTessFactor(5.0f);

	ID3DX11EffectTechnique* tessShadowTech = Effects::BuildShadowMapFX->BuildShadowMapTech;
	ID3DX11EffectTechnique* shadowTech = Effects::BuildShadowMapFX->BuildShadowMapTech;
	switch (m_RenderOption)
	{
	case RenderOptionsBasic:
	case RenderOptionsNormalMap:
		d3d_context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		break;
	case RenderOptionsDisplacementMap:
		d3d_context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
		tessShadowTech = Effects::BuildShadowMapFX->TessBuildShadowMapTech;
		break;
	default:
		break;
	}

	XMMATRIX world;
	XMMATRIX worldInvTranspose;
	XMMATRIX worldViewProj;

	//
	// Draw the grid, cylinders, and box without any cubemap reflection.
	// 

	UINT stride = sizeof(Vertex::PosNormalTexTan);
	UINT offset = 0;

	d3d_context_->IASetInputLayout(InputLayouts::PosNormalTexTan);
	d3d_context_->IASetVertexBuffers(0, 1, &m_ShapesVB, &stride, &offset);
	d3d_context_->IASetIndexBuffer(m_ShapesIB, DXGI_FORMAT_R32_UINT, 0);

	D3DX11_TECHNIQUE_DESC techDesc;
	tessShadowTech->GetDesc(&techDesc);
	for (UINT p = 0; p < techDesc.Passes; ++p)
	{
		// Draw the grid.
		world = XMLoadFloat4x4(&m_GridWorld);
		worldInvTranspose = MathHelper::InverseTranspose(world);
		worldViewProj = world * view * proj;

		Effects::BuildShadowMapFX->SetWorld(world);
		Effects::BuildShadowMapFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BuildShadowMapFX->SetWorldViewProj(worldViewProj);
		Effects::BuildShadowMapFX->SetTexTransform(XMMatrixScaling(8.0f, 10.0f, 1.0f));

		tessShadowTech->GetPassByIndex(p)->Apply(0, d3d_context_);
		d3d_context_->DrawIndexed(m_GridIndexCount, m_GridIndexOffset, m_GridVertexOffset);

		// Draw the box.
		world = XMLoadFloat4x4(&m_BoxWorld);
		worldInvTranspose = MathHelper::InverseTranspose(world);
		worldViewProj = world * view * proj;

		Effects::BuildShadowMapFX->SetWorld(world);
		Effects::BuildShadowMapFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BuildShadowMapFX->SetWorldViewProj(worldViewProj);
		Effects::BuildShadowMapFX->SetTexTransform(XMMatrixScaling(2.0f, 1.0f, 1.0f));

		tessShadowTech->GetPassByIndex(p)->Apply(0, d3d_context_);
		d3d_context_->DrawIndexed(m_BoxIndexCount, m_BoxIndexOffset, m_BoxVertexOffset);

		// Draw the cylinders.
		for (int i = 0; i < 10; ++i)
		{
			world = XMLoadFloat4x4(&m_CylWorld[i]);
			worldInvTranspose = MathHelper::InverseTranspose(world);
			worldViewProj = world * view * proj;;

			Effects::BuildShadowMapFX->SetWorld(world);
			Effects::BuildShadowMapFX->SetWorldInvTranspose(worldInvTranspose);
			Effects::BuildShadowMapFX->SetWorldViewProj(worldViewProj);
			Effects::BuildShadowMapFX->SetTexTransform(XMMatrixScaling(1.0f, 2.0f, 1.0f));

			tessShadowTech->GetPassByIndex(p)->Apply(0, d3d_context_);
			d3d_context_->DrawIndexed(m_CylinderIndexCount, m_CylinderIndexOffset, m_CylinderVertexOffset);
		}
	}

	// FX sets tessellation stages, but it does not disable them.  So do that here
	// to turn off tessellation.
	d3d_context_->HSSetShader(nullptr, nullptr, 0);
	d3d_context_->DSSetShader(nullptr, nullptr, 0);

	d3d_context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	shadowTech->GetDesc(&techDesc);
	for (UINT p = 0; p < techDesc.Passes; ++p)
	{
		// Draw the spheres.
		for (int i = 0; i < 10; ++i)
		{
			world = XMLoadFloat4x4(&m_SphereWorld[i]);
			worldInvTranspose = MathHelper::InverseTranspose(world);
			worldViewProj = world * view * proj;

			Effects::BuildShadowMapFX->SetWorld(world);
			Effects::BuildShadowMapFX->SetWorldInvTranspose(worldInvTranspose);
			Effects::BuildShadowMapFX->SetWorldViewProj(worldViewProj);
			Effects::BuildShadowMapFX->SetTexTransform(XMMatrixIdentity());

			shadowTech->GetPassByIndex(p)->Apply(0, d3d_context_);
			d3d_context_->DrawIndexed(m_SphereIndexCount, m_SphereIndexOffset, m_SphereVertexOffset);
		}
	}

	stride = sizeof(Vertex::Basic32);
	offset = 0;

	d3d_context_->RSSetState(nullptr);

	d3d_context_->IASetInputLayout(InputLayouts::Basic32);
	d3d_context_->IASetVertexBuffers(0, 1, &m_SkullVB, &stride, &offset);
	d3d_context_->IASetIndexBuffer(m_SkullIB, DXGI_FORMAT_R32_UINT, 0);

	for (UINT p = 0; p < techDesc.Passes; ++p)
	{
		// Draw the skull.
		world = XMLoadFloat4x4(&m_SkullWorld);
		worldInvTranspose = MathHelper::InverseTranspose(world);
		worldViewProj = world*view*proj;

		Effects::BuildShadowMapFX->SetWorld(world);
		Effects::BuildShadowMapFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BuildShadowMapFX->SetWorldViewProj(worldViewProj);
		Effects::BuildShadowMapFX->SetTexTransform(XMMatrixIdentity());

		shadowTech->GetPassByIndex(p)->Apply(0, d3d_context_);
		d3d_context_->DrawIndexed(m_SkullIndexCount, 0, 0);
	}
}

void ShadowsApp::DrawScreenQuad()
{
	UINT stride = sizeof(Vertex::Basic32);
	UINT offset = 0;

	d3d_context_->IASetInputLayout(InputLayouts::Basic32);
	d3d_context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	d3d_context_->IASetVertexBuffers(0, 1, &m_ScreenQuadVB, &stride, &offset);
	d3d_context_->IASetIndexBuffer(m_ScreenQuadIB, DXGI_FORMAT_R32_UINT, 0);

	// Scale and shift quad to lower-right corner.
	XMMATRIX world(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, -0.5f, 0.0f, 1.0f);

	ID3DX11EffectTechnique* tech = Effects::DebugTexFX->ViewRedTech;
	D3DX11_TECHNIQUE_DESC techDesc;
	tech->GetDesc(&techDesc);
	for (int p = 0; p < techDesc.Passes; ++p)
	{
		Effects::DebugTexFX->SetWorldViewProj(world);
		Effects::DebugTexFX->SetTexture(m_ShadowMap->DepthMapSRV());

		tech->GetPassByIndex(p)->Apply(0, d3d_context_);
		d3d_context_->DrawIndexed(6, 0, 0);
	}
}

void ShadowsApp::BuildShadowTransform()
{
	// Only the first "main" light casts a shadow.
	XMVECTOR lightDir = XMLoadFloat3(&m_DirLights[0].Direction);
	XMVECTOR lightPos = -2.0f * m_SceneBounds.m_Radius * lightDir;
	XMVECTOR targetPos = XMLoadFloat3(&m_SceneBounds.m_Center);
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(lightPos, targetPos, up);

	// Transform bounding sphere to light space.
	XMFLOAT3 sphereCenterLS;
	XMStoreFloat3(&sphereCenterLS, XMVector3TransformCoord(targetPos, view));

	// Ortho frustum in light space encloses scene.
	float l = sphereCenterLS.x - m_SceneBounds.m_Radius;
	float b = sphereCenterLS.y - m_SceneBounds.m_Radius;
	float n = sphereCenterLS.z - m_SceneBounds.m_Radius;
	float r = sphereCenterLS.x + m_SceneBounds.m_Radius;
	float t = sphereCenterLS.y + m_SceneBounds.m_Radius;
	float f = sphereCenterLS.z + m_SceneBounds.m_Radius;
	XMMATRIX proj = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);

	// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	XMMATRIX s = view * proj * T;

	XMStoreFloat4x4(&m_LightView, view);
	XMStoreFloat4x4(&m_LightProj, proj);
	XMStoreFloat4x4(&m_ShadowTransform, s);
}

void ShadowsApp::BuildShapeGeometryBuffers()
{
	GeometryGenerator::MeshData box;
	GeometryGenerator::MeshData grid;
	GeometryGenerator::MeshData sphere;
	GeometryGenerator::MeshData cylinder;

	GeometryGenerator geoGen;
	geoGen.CreateBox(1.0f, 1.0f, 1.0f, box);
	geoGen.CreateGrid(20.0f, 30.0f, 50, 40, grid);
	geoGen.CreateSphere(0.5f, 20, 20, sphere);
	geoGen.CreateCylinder(0.5f, 0.5f, 3.0f, 15, 15, cylinder);

	// Cache the vertex offsets to each object in the concatenated vertex buffer.
	m_BoxVertexOffset = 0;
	m_GridVertexOffset = box.Vertices.size();
	m_SphereVertexOffset = m_GridVertexOffset + grid.Vertices.size();
	m_CylinderVertexOffset = m_SphereVertexOffset + sphere.Vertices.size();

	// Cache the index count of each object.
	m_BoxIndexCount = box.Indices.size();
	m_GridIndexCount = grid.Indices.size();
	m_SphereIndexCount = sphere.Indices.size();
	m_CylinderIndexCount = cylinder.Indices.size();

	// Cache the starting index for each object in the concatenated index buffer.
	m_BoxIndexOffset = 0;
	m_GridIndexOffset = m_BoxIndexCount;
	m_SphereIndexOffset = m_GridIndexOffset + m_GridIndexCount;
	m_CylinderIndexOffset = m_SphereIndexOffset + m_SphereIndexCount;

	UINT totalVertexCount =
		box.Vertices.size() +
		grid.Vertices.size() +
		sphere.Vertices.size() +
		cylinder.Vertices.size();

	UINT totalIndexCount =
		m_BoxIndexCount +
		m_GridIndexCount +
		m_SphereIndexCount +
		m_CylinderIndexCount;

	//
	// Extract the vertex elements we are interested in and pack the
	// vertices of all the meshes into one vertex buffer.
	//

	std::vector<Vertex::PosNormalTexTan> vertices(totalVertexCount);

	UINT k = 0;
	for (size_t i = 0; i < box.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = box.Vertices[i].Position;
		vertices[k].Normal = box.Vertices[i].Normal;
		vertices[k].Tex = box.Vertices[i].TexC;
		vertices[k].TangentU = box.Vertices[i].TangentU;
	}

	for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = grid.Vertices[i].Position;
		vertices[k].Normal = grid.Vertices[i].Normal;
		vertices[k].Tex = grid.Vertices[i].TexC;
		vertices[k].TangentU = grid.Vertices[i].TangentU;
	}

	for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = sphere.Vertices[i].Position;
		vertices[k].Normal = sphere.Vertices[i].Normal;
		vertices[k].Tex = sphere.Vertices[i].TexC;
		vertices[k].TangentU = sphere.Vertices[i].TangentU;
	}

	for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = cylinder.Vertices[i].Position;
		vertices[k].Normal = cylinder.Vertices[i].Normal;
		vertices[k].Tex = cylinder.Vertices[i].TexC;
		vertices[k].TangentU = cylinder.Vertices[i].TangentU;
	}

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex::PosNormalTexTan) * totalVertexCount;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &vertices[0];
	HR(d3d_device_->CreateBuffer(&vbd, &vinitData, &m_ShapesVB));

	//
	// Pack the indices of all the meshes into one index buffer.
	//

	std::vector<UINT> indices;
	indices.insert(indices.end(), box.Indices.begin(), box.Indices.end());
	indices.insert(indices.end(), grid.Indices.begin(), grid.Indices.end());
	indices.insert(indices.end(), sphere.Indices.begin(), sphere.Indices.end());
	indices.insert(indices.end(), cylinder.Indices.begin(), cylinder.Indices.end());

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * totalIndexCount;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &indices[0];
	HR(d3d_device_->CreateBuffer(&ibd, &iinitData, &m_ShapesIB));
}

void ShadowsApp::BuildSkullGeometryBuffers()
{
	std::ifstream fin("Models/skull.txt");

	if (!fin)
	{
		MessageBox(0, L"Models/skull.txt not found.", 0, 0);
		return;
	}

	UINT vcount = 0;
	UINT tcount = 0;
	std::string ignore;

	fin >> ignore >> vcount;
	fin >> ignore >> tcount;
	fin >> ignore >> ignore >> ignore >> ignore;

	std::vector<Vertex::Basic32> vertices(vcount);
	for (UINT i = 0; i < vcount; ++i)
	{
		fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
		fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;
	}

	fin >> ignore;
	fin >> ignore;
	fin >> ignore;

	m_SkullIndexCount = 3 * tcount;
	std::vector<UINT> indices(m_SkullIndexCount);
	for (UINT i = 0; i < tcount; ++i)
	{
		fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
	}

	fin.close();

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex::Basic32) * vcount;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &vertices[0];
	HR(d3d_device_->CreateBuffer(&vbd, &vinitData, &m_SkullVB));

	//
	// Pack the indices of all the meshes into one index buffer.
	//

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * m_SkullIndexCount;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &indices[0];
	HR(d3d_device_->CreateBuffer(&ibd, &iinitData, &m_SkullIB));
}

void ShadowsApp::BuildScreenQuadGeometryBuffers()
{
	GeometryGenerator::MeshData quad;
	GeometryGenerator geoGen;
	geoGen.CreateFullscreenQuad(quad);

	//
	// Extract the vertex elements we are interested in and pack the
	// vertices of all the meshes into one vertex buffer.
	//
	std::vector<Vertex::Basic32> vertices(quad.Vertices.size());

	for (int i = 0; i < vertices.size(); ++i)
	{
		vertices[i].Pos = quad.Vertices[i].Position;
		vertices[i].Normal = quad.Vertices[i].Normal;
		vertices[i].Tex = quad.Vertices[i].TexC;
	}

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex::Basic32) * vertices.size();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vData;
	vData.pSysMem = &vertices[0];
	HR(d3d_device_->CreateBuffer(&vbd, &vData, &m_ScreenQuadVB));

	//
	// Pack the indices of all the meshes into one index buffer.
	//
	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * quad.Indices.size();
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	ibd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA iData;
	iData.pSysMem = &quad.Indices[0];
	HR(d3d_device_->CreateBuffer(&ibd, &iData, &m_ScreenQuadIB));
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	ShadowsApp theApp(hInstance);

	if (!theApp.Init())
	{
		return 0;
	}

	return theApp.Run();
}