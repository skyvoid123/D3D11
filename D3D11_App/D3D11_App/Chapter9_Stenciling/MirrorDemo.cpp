#include "d3dApp.h"
#include "d3dx11Effect.h"
#include "GeometryGenerator.h"
#include "MathHelper.h"
#include "LightHelper.h"
#include "DDSTextureLoader.h"
#include "Effects.h"
#include "Vertex.h"
#include "RenderStates.h"
#include "Waves.h"

enum RenderOptions
{
	Lighting = 0,
	Textures = 1,
	TexturesAndFog = 2
};

class MirrorApp : public D3DApp
{
public:
	MirrorApp(HINSTANCE hInstance);
	~MirrorApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

	void OnMouseDown(WPARAM btnState, int x, int y);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y);

private:
	void BuildRoomGeometryBuffers();
	void BuildSkullGeometryBuffers();

private:
	ID3D11Buffer* m_RoomVB;

	ID3D11Buffer* m_SkullVB;
	ID3D11Buffer* m_SkullIB;

	ID3D11ShaderResourceView* m_FloorMapSRV;
	ID3D11ShaderResourceView* m_WallMapSRV;
	ID3D11ShaderResourceView* m_MirrorMapSRV;

	DirectionalLight m_DirLights[3];
	Material m_RoomMat;
	Material m_SkullMat;
	Material m_MirrorMat;
	Material m_ShadowMat;
	
	XMFLOAT4X4 m_RoomWorld;
	XMFLOAT4X4 m_SkullWorld;

	UINT m_SkullIndexCount;
	XMFLOAT3 m_SkullTranslation;

	XMFLOAT4X4 m_View;
	XMFLOAT4X4 m_Proj;

	RenderOptions m_RenderOptions;

	XMFLOAT3 m_EyePosW;

	float m_Radius;
	float m_Phi;
	float m_Theta;

	POINT m_LastMousePos;
};

MirrorApp::MirrorApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
	, m_RoomVB(nullptr)
	, m_SkullVB(nullptr)
	, m_SkullIB(nullptr)
	, m_FloorMapSRV(nullptr)
	, m_WallMapSRV(nullptr)
	, m_MirrorMapSRV(nullptr)
	, m_SkullIndexCount(0)
	, m_SkullTranslation(0.0f, 1.0f, -5.0f)
	, m_RenderOptions(RenderOptions::Textures)
	, m_EyePosW(0.f, 0.f, 0.f)
	, m_Radius(12.f)
	, m_Phi(.4f * MathHelper::Pi)
	, m_Theta(1.3f * MathHelper::Pi)
{
	main_wnd_caption_ = L"Mirror Demo";
	enable_4x_msaa_ = false;

	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&m_RoomWorld, I);

	m_DirLights[0].Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_DirLights[0].Diffuse = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_DirLights[0].Specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_DirLights[0].Direction = XMFLOAT3(0.57735f, -0.57735f, 0.57735f);

	m_DirLights[1].Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	m_DirLights[1].Diffuse = XMFLOAT4(0.20f, 0.20f, 0.20f, 1.0f);
	m_DirLights[1].Specular = XMFLOAT4(0.25f, 0.25f, 0.25f, 1.0f);
	m_DirLights[1].Direction = XMFLOAT3(-0.57735f, -0.57735f, 0.57735f);

	m_DirLights[2].Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	m_DirLights[2].Diffuse = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_DirLights[2].Specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	m_DirLights[2].Direction = XMFLOAT3(0.0f, -0.707f, -0.707f);

	m_RoomMat.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_RoomMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_RoomMat.Specular = XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);

	m_SkullMat.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_SkullMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_SkullMat.Specular = XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);

	// Reflected material is transparent so it blends into mirror.
	m_MirrorMat.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_MirrorMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
	m_MirrorMat.Specular = XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);

	m_ShadowMat.Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	m_ShadowMat.Diffuse = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.5f);
	m_ShadowMat.Specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 16.0f);
}

MirrorApp::~MirrorApp()
{
	d3d_context_->ClearState();
	ReleaseCOM(m_RoomVB);
	ReleaseCOM(m_SkullVB);
	ReleaseCOM(m_SkullIB);
	ReleaseCOM(m_FloorMapSRV);
	ReleaseCOM(m_WallMapSRV);
	ReleaseCOM(m_MirrorMapSRV);

	Effects::DestroyAll();
	InputLayouts::DestroyAll();
	RenderStates::DestroyAll();
}

bool MirrorApp::Init()
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
		L"Textures/checkboard.dds", &texRes, &m_FloorMapSRV));
	ReleaseCOM(texRes);

	HR(DirectX::CreateDDSTextureFromFile(d3d_device_,
		L"Textures/brick01.dds", &texRes, &m_WallMapSRV));
	ReleaseCOM(texRes);

	HR(DirectX::CreateDDSTextureFromFile(d3d_device_,
		L"Textures/ice.dds", &texRes, &m_MirrorMapSRV));
	ReleaseCOM(texRes);

	BuildRoomGeometryBuffers();
	BuildSkullGeometryBuffers();

	return true;
}

void MirrorApp::OnResize()
{
	D3DApp::OnResize();

	XMMATRIX P = XMMatrixPerspectiveFovLH(.25f * MathHelper::Pi, AspectRatio(), 1.f, 1000.f);
	XMStoreFloat4x4(&m_Proj, P);
}

void MirrorApp::UpdateScene(float dt)
{
	float x = m_Radius * sinf(m_Phi) * cosf(m_Theta);
	float z = m_Radius * sinf(m_Phi) * sinf(m_Theta);
	float y = m_Radius * cosf(m_Phi);

	m_EyePosW = XMFLOAT3(x, y, z);

	auto pos = XMVectorSet(x, y, z, 1.f);
	auto target = XMVectorZero();
	auto up = XMVectorSet(0.f, 1.f, 0.f, 0.f);

	auto V = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&m_View, V);

	//
	// Switch the render mode based in key input.
	//
	if (GetAsyncKeyState('1') & 0x8000)
		m_RenderOptions = RenderOptions::Lighting;

	if (GetAsyncKeyState('2') & 0x8000)
		m_RenderOptions = RenderOptions::Textures;

	if (GetAsyncKeyState('3') & 0x8000)
		m_RenderOptions = RenderOptions::TexturesAndFog;

	//
	// Allow user to move box.
	//
	if (GetAsyncKeyState('A') & 0x8000)
		m_SkullTranslation.x -= 1.f * dt;

	if (GetAsyncKeyState('D') & 0x8000)
		m_SkullTranslation.x += 1.f * dt;
	
	if (GetAsyncKeyState('W') & 0x8000)
		m_SkullTranslation.y += 1.f * dt;

	if (GetAsyncKeyState('S') & 0x8000)
		m_SkullTranslation.y -= 1.f * dt;

	// Don't let user move below ground plane.
	m_SkullTranslation.y = MathHelper::Max(m_SkullTranslation.y, 0.0f);

	// Update the new world matrix.
	XMMATRIX skullRotation = XMMatrixRotationY(0.5f * MathHelper::Pi);
	XMMATRIX skullScale = XMMatrixScaling(.45f, .45f, .45f);
	XMMATRIX skullOffset = XMMatrixTranslation(m_SkullTranslation.x, m_SkullTranslation.y, m_SkullTranslation.z);
	XMStoreFloat4x4(&m_SkullWorld, skullRotation * skullScale * skullOffset);
}

void MirrorApp::DrawScene()
{
	d3d_context_->ClearRenderTargetView(render_target_view_, (const float*)&Colors::Silver);
	d3d_context_->ClearDepthStencilView(depth_stencil_view_, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

	d3d_context_->IASetInputLayout(InputLayouts::Basic32);
	d3d_context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	float blendFactor[] = { 0.f, 0.f, 0.f, 0.f };

	UINT stride = sizeof(Vertex::Basic32);
	UINT offset = 0;

	XMMATRIX view = XMLoadFloat4x4(&m_View);
	XMMATRIX proj = XMLoadFloat4x4(&m_Proj);
	XMMATRIX viewProj = view * proj;

	// Set per frame constants.
	Effects::BasicFX->SetDirLights(m_DirLights);
	Effects::BasicFX->SetEyePosW(m_EyePosW);
	Effects::BasicFX->SetFogColor(Colors::Silver);
	Effects::BasicFX->SetFogStart(2.f);
	Effects::BasicFX->SetFogRange(40.f);

	// Skull doesn't have texture coordinates, so we can't texture it.
	ID3DX11EffectTechnique* activeTech;
	ID3DX11EffectTechnique* activeSkullTech;

	switch (m_RenderOptions)
	{
	case Lighting:
		activeTech = Effects::BasicFX->Light3Tech;
		activeSkullTech = Effects::BasicFX->Light3Tech;
		break;
	case Textures:
		activeTech = Effects::BasicFX->Light3TexTech;
		activeSkullTech = Effects::BasicFX->Light3Tech;
		break;
	case TexturesAndFog:
		activeTech = Effects::BasicFX->Light3TexFogTech;
		activeSkullTech = Effects::BasicFX->Light3FogTech;
		break;
	default:
		break;
	}

	D3DX11_TECHNIQUE_DESC techDesc;

	// Draw the floor and walls to the back buffer as normal.
	activeTech->GetDesc(&techDesc);
	for (int p = 0; p < techDesc.Passes; ++p)
	{
		d3d_context_->IASetVertexBuffers(0, 1, &m_RoomVB, &stride, &offset);

		// Set per object constants.
		XMMATRIX world = XMLoadFloat4x4(&m_RoomWorld);
		XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
		XMMATRIX wvp = world * view * proj;

		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(wvp);
		Effects::BasicFX->SetTexTransform(XMMatrixIdentity());
		Effects::BasicFX->SetMaterial(m_RoomMat);

		ID3DX11EffectPass* pass = activeTech->GetPassByIndex(p);
		// Floor
		Effects::BasicFX->SetDiffuseMap(m_FloorMapSRV);
		pass->Apply(0, d3d_context_);
		d3d_context_->Draw(6, 0);
		
		// Wall
		Effects::BasicFX->SetDiffuseMap(m_WallMapSRV);
		pass->Apply(0, d3d_context_);
		d3d_context_->Draw(18, 6);
	}

	// Draw the skull to the back buffer as normal.
	activeSkullTech->GetDesc(&techDesc);
	for (int p = 0; p < techDesc.Passes; ++p)
	{
		d3d_context_->IASetVertexBuffers(0, 1, &m_SkullVB, &stride, &offset);
		d3d_context_->IASetIndexBuffer(m_SkullIB, DXGI_FORMAT_R32_UINT, 0);

		// Set per object constants.
		XMMATRIX world = XMLoadFloat4x4(&m_SkullWorld);
		XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
		XMMATRIX wvp = world * view * proj;

		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(wvp);
		Effects::BasicFX->SetMaterial(m_SkullMat);

		activeSkullTech->GetPassByIndex(p)->Apply(0, d3d_context_);
		d3d_context_->DrawIndexed(m_SkullIndexCount, 0, 0);
	}

	// Draw the mirror to stencil buffer only.
	activeTech->GetDesc(&techDesc);
	for (int p = 0; p < techDesc.Passes; ++p)
	{
		d3d_context_->IASetVertexBuffers(0, 1, &m_RoomVB, &stride, &offset);

		// Set per object constants.
		XMMATRIX world = XMLoadFloat4x4(&m_RoomWorld);
		XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
		XMMATRIX wvp = world * view * proj;

		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(wvp);
		Effects::BasicFX->SetTexTransform(XMMatrixIdentity());

		// Render visible mirror pixels to stencil buffer.
		// Do not write mirror depth to depth buffer at this point, otherwise it will occlude the reflection.
		d3d_context_->OMSetDepthStencilState(RenderStates::MarkMirrorDSS, 1);

		// Do not write to render target.
		d3d_context_->OMSetBlendState(RenderStates::NoRenderTargetWritesBS, blendFactor, 0xffffffff);


		activeTech->GetPassByIndex(p)->Apply(0, d3d_context_);
		d3d_context_->Draw(6, 24);

		// Restore states.
		d3d_context_->OMSetDepthStencilState(nullptr, 0);
		d3d_context_->OMSetBlendState(nullptr, blendFactor, 0xffffffff);
	}

	// Draw the skull reflection.
	activeSkullTech->GetDesc(&techDesc);
	for (int p = 0; p < techDesc.Passes; ++p)
	{
		d3d_context_->IASetVertexBuffers(0, 1, &m_SkullVB, &stride, &offset);
		d3d_context_->IASetIndexBuffer(m_SkullIB, DXGI_FORMAT_R32_UINT, 0);

		// Set per object constants.
		XMVECTOR mirrorPlane = XMVectorSet(0.f, 0.f, -1.f, 0.f);	// xy plane
		XMMATRIX R = XMMatrixReflect(mirrorPlane);
		XMMATRIX world = XMLoadFloat4x4(&m_SkullWorld) * R;
		XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
		XMMATRIX wvp = world * view * proj;

		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(wvp);
		Effects::BasicFX->SetMaterial(m_SkullMat);

		// Cache the old light directions, and reflect the light directions.
		XMFLOAT3 oldLightDirections[3];
		for (int i = 0; i < 3; ++i)
		{
			oldLightDirections[i] = m_DirLights[i].Direction;

			XMVECTOR lightDir = XMLoadFloat3(&m_DirLights[i].Direction);
			XMVECTOR reflectedLightDir = XMVector3TransformNormal(lightDir, R);
			XMStoreFloat3(&m_DirLights[i].Direction, reflectedLightDir);
		}
		Effects::BasicFX->SetDirLights(m_DirLights);

		// Cull clockwise triangles for reflection.
		d3d_context_->RSSetState(RenderStates::CullClockwiseRS);

		// Only draw reflection into visible mirror pixels as marked by the stencil buffer. 
		d3d_context_->OMSetDepthStencilState(RenderStates::DrawReflectionDSS, 1);

		activeSkullTech->GetPassByIndex(p)->Apply(0, d3d_context_);
		d3d_context_->DrawIndexed(m_SkullIndexCount, 0, 0);

		// Restore default states.
		d3d_context_->RSSetState(nullptr);
		d3d_context_->OMSetDepthStencilState(nullptr, 0);

		// Restore light directions.
		for (int i = 0; i < 3; ++i)
		{
			m_DirLights[i].Direction = oldLightDirections[i];
		}
		Effects::BasicFX->SetDirLights(m_DirLights);
	}

	// Draw the mirror to the back buffer as usual but with transparency
	// blending so the reflection shows through.
	activeTech->GetDesc(&techDesc);
	for (int p = 0; p < techDesc.Passes; ++p)
	{
		d3d_context_->IASetVertexBuffers(0, 1, &m_RoomVB, &stride, &offset);

		// Set per object constants.
		XMMATRIX world = XMLoadFloat4x4(&m_RoomWorld);
		XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
		XMMATRIX wvp = world * view * proj;

		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(wvp);
		Effects::BasicFX->SetTexTransform(XMMatrixIdentity());
		Effects::BasicFX->SetMaterial(m_MirrorMat);
		Effects::BasicFX->SetDiffuseMap(m_MirrorMapSRV);

		// Mirror
		d3d_context_->OMSetBlendState(RenderStates::TransparentBS, blendFactor, 0xffffffff);
		activeTech->GetPassByIndex(p)->Apply(0, d3d_context_);
		d3d_context_->Draw(6, 24);

		// No need to restore the blend state. Blow we will also use the TransparentBS to draw shadow.
	}

	// Draw the skull shadow.
	activeSkullTech->GetDesc(&techDesc);
	for (int p = 0; p < techDesc.Passes; ++p)
	{
		d3d_context_->IASetVertexBuffers(0, 1, &m_SkullVB, &stride, &offset);
		d3d_context_->IASetIndexBuffer(m_SkullIB, DXGI_FORMAT_R32_UINT, 0);

		// Set per object constants.
		XMVECTOR shadowPlane = XMVectorSet(0.f, 1.f, 0.f, 0.f);	// xz plane
		XMVECTOR toMainLight = -XMLoadFloat3(&m_DirLights[0].Direction);
		XMMATRIX S = XMMatrixShadow(shadowPlane, toMainLight);
		XMMATRIX shadowOffsetY = XMMatrixTranslation(0.f, 0.001f, 0.f);
		XMMATRIX world = XMLoadFloat4x4(&m_SkullWorld) * S * shadowOffsetY;
		XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
		XMMATRIX wvp = world * view * proj;

		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(wvp);
		Effects::BasicFX->SetMaterial(m_ShadowMat);

		d3d_context_->OMSetDepthStencilState(RenderStates::NoDoubleBlendDSS, 0);
		activeSkullTech->GetPassByIndex(p)->Apply(0, d3d_context_);
		d3d_context_->DrawIndexed(m_SkullIndexCount, 0, 0);

		// Restore default states.
		d3d_context_->OMSetDepthStencilState(nullptr, 0);
		d3d_context_->OMSetBlendState(nullptr, blendFactor, 0xffffffff);
	}

	HR(swap_chain_->Present(0, 0));
}

void MirrorApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	m_LastMousePos.x = x;
	m_LastMousePos.y = y;

	SetCapture(main_wnd_);
}

void MirrorApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void MirrorApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f * (float)(x - m_LastMousePos.x));
		float dy = XMConvertToRadians(0.25f * (float)(y - m_LastMousePos.y));

		// Update angles based on input to orbit camera around box.
		m_Theta -= dx;
		m_Phi -= dy;

		// Restrict the angle mPhi.
		m_Phi = MathHelper::Clamp(m_Phi, 0.1f, MathHelper::Pi - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		// Make each pixel correspond to 0.2 unit in the scene.
		float dx = 0.2f * (float)(x - m_LastMousePos.x);
		float dy = 0.2f * (float)(y - m_LastMousePos.y);

		// Update the camera radius based on input.
		m_Radius += dx - dy;

		// Restrict the radius.
		m_Radius = MathHelper::Clamp(m_Radius, 3.0f, 50.0f);
	}

	m_LastMousePos.x = x;
	m_LastMousePos.y = y;
}

void MirrorApp::BuildRoomGeometryBuffers()
{
	// Create and specify geometry.  For this sample we draw a floor
	// and a wall with a mirror on it.  We put the floor, wall, and
	// mirror geometry in one vertex buffer.
	Vertex::Basic32 v[30];

	// Floor: Observe we tile texture coordinates.
	v[0] = Vertex::Basic32(-3.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 0.0f, 4.0f);
	v[1] = Vertex::Basic32(-3.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	v[2] = Vertex::Basic32(7.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 4.0f, 0.0f);

	v[3] = Vertex::Basic32(-3.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 0.0f, 4.0f);
	v[4] = Vertex::Basic32(7.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 4.0f, 0.0f);
	v[5] = Vertex::Basic32(7.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 4.0f, 4.0f);

	// Wall: Observe we tile texture coordinates, and that we
	// leave a gap in the middle for the mirror.
	v[6] = Vertex::Basic32(-3.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f);
	v[7] = Vertex::Basic32(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[8] = Vertex::Basic32(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 0.0f);

	v[9] = Vertex::Basic32(-3.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f);
	v[10] = Vertex::Basic32(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 0.0f);
	v[11] = Vertex::Basic32(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 2.0f);

	v[12] = Vertex::Basic32(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f);
	v[13] = Vertex::Basic32(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[14] = Vertex::Basic32(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 0.0f);

	v[15] = Vertex::Basic32(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f);
	v[16] = Vertex::Basic32(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 0.0f);
	v[17] = Vertex::Basic32(7.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 2.0f);

	v[18] = Vertex::Basic32(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[19] = Vertex::Basic32(-3.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[20] = Vertex::Basic32(7.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 0.0f);

	v[21] = Vertex::Basic32(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[22] = Vertex::Basic32(7.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 0.0f);
	v[23] = Vertex::Basic32(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 1.0f);

	// Mirror
	v[24] = Vertex::Basic32(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[25] = Vertex::Basic32(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[26] = Vertex::Basic32(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);

	v[27] = Vertex::Basic32(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[28] = Vertex::Basic32(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	v[29] = Vertex::Basic32(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.ByteWidth = sizeof(Vertex::Basic32) * 30;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vData;
	vData.pSysMem = v;
	HR(d3d_device_->CreateBuffer(&vbd, &vData, &m_RoomVB));
}

void MirrorApp::BuildSkullGeometryBuffers()
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

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	MirrorApp theApp(hInstance);

	if (!theApp.Init())
	{
		return 0;
	}

	return theApp.Run();
}