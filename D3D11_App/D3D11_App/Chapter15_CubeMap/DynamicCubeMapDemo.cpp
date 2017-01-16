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

#include "Camera.h"

class DynamicCubeMapApp : public D3DApp
{
public:
	DynamicCubeMapApp(HINSTANCE hInstance);
	~DynamicCubeMapApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

	void OnMouseDown(WPARAM btnState, int x, int y);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y);

private:
	void DrawScene(const Camera& camera, bool drawCenterSphere);
	void BuildCubeFaceCameras(float x, float y, float z);
	void BuildDynamicCubeMapViews();
	void BuildShapeGeometryBuffers();
	void BuildSkullGeometryBuffers();

private:
	ID3D11Buffer* m_ShapesVB;
	ID3D11Buffer* m_ShapesIB;

	ID3D11Buffer* m_SkullVB;
	ID3D11Buffer* m_SkullIB;

	ID3D11ShaderResourceView* m_FloorTexSRV;
	ID3D11ShaderResourceView* m_StoneTexSRV;
	ID3D11ShaderResourceView* m_BrickTexSRV;

	ID3D11DepthStencilView* m_DynamicCubeMapDSV;
	ID3D11RenderTargetView* m_DynamicCubeMapRTV[6];
	ID3D11ShaderResourceView* m_DynamicCubeMapSRV;
	D3D11_VIEWPORT m_CubeMapViewport;

	static const int CubeMapSize = 256;

	Sky* m_Skys[3];
	Sky* m_CurrentSky;

	DirectionalLight m_DirLights[3];
	Material m_GridMat;
	Material m_BoxMat;
	Material m_CylinderMat;
	Material m_SphereMat;
	Material m_SkullMat;
	Material m_CenterSphereMat;

	// Define transformations from local spaces to world space.
	XMFLOAT4X4 m_SphereWorld[10];
	XMFLOAT4X4 m_CylWorld[10];
	XMFLOAT4X4 m_BoxWorld;
	XMFLOAT4X4 m_GridWorld;
	XMFLOAT4X4 m_SkullWorld;
	XMFLOAT4X4 m_CenterSphereWorld;

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

	Camera m_Camera;
	Camera m_CubeMapCamera[6];

	POINT m_LastMousePos;
};

DynamicCubeMapApp::DynamicCubeMapApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
	, m_ShapesVB(nullptr)
	, m_ShapesIB(nullptr)
	, m_SkullVB(nullptr)
	, m_SkullIB(nullptr)
	, m_FloorTexSRV(nullptr)
	, m_StoneTexSRV(nullptr)
	, m_BrickTexSRV(nullptr)
	, m_DynamicCubeMapDSV(nullptr)
	, m_DynamicCubeMapSRV(nullptr)
	, m_CurrentSky(nullptr)
{
	main_wnd_caption_ = L"Dynamic Cube Map Demo";

	for (int i = 0; i < 6; ++i)
	{
		m_DynamicCubeMapRTV[i] = nullptr;
	}

	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&m_GridWorld, I);

	XMMATRIX boxScale = XMMatrixScaling(3.0f, 1.0f, 3.0f);
	XMMATRIX boxOffset = XMMatrixTranslation(0.0f, 0.5f, 0.0f);
	XMStoreFloat4x4(&m_BoxWorld, XMMatrixMultiply(boxScale, boxOffset));

	XMMATRIX centerSphereScale = XMMatrixScaling(2.0f, 2.0f, 2.0f);
	XMMATRIX centerSphereOffset = XMMatrixTranslation(0.0f, 2.0f, 0.0f);
	XMStoreFloat4x4(&m_CenterSphereWorld, XMMatrixMultiply(centerSphereScale, centerSphereOffset));

	for (int i = 0; i < 5; ++i)
	{
		XMStoreFloat4x4(&m_CylWorld[i * 2 + 0], XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i*5.0f));
		XMStoreFloat4x4(&m_CylWorld[i * 2 + 1], XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i*5.0f));

		XMStoreFloat4x4(&m_SphereWorld[i * 2 + 0], XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i*5.0f));
		XMStoreFloat4x4(&m_SphereWorld[i * 2 + 1], XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i*5.0f));
	}

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

	m_GridMat.Ambient = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	m_GridMat.Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	m_GridMat.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
	m_GridMat.Reflect = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

	m_CylinderMat.Ambient = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_CylinderMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_CylinderMat.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
	m_CylinderMat.Reflect = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

	m_SphereMat.Ambient = XMFLOAT4(0.2f, 0.3f, 0.4f, 1.0f);
	m_SphereMat.Diffuse = XMFLOAT4(0.2f, 0.3f, 0.4f, 1.0f);
	m_SphereMat.Specular = XMFLOAT4(0.9f, 0.9f, 0.9f, 16.0f);
	m_SphereMat.Reflect = XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);

	m_BoxMat.Ambient = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_BoxMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_BoxMat.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
	m_BoxMat.Reflect = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

	m_SkullMat.Ambient = XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);
	m_SkullMat.Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	m_SkullMat.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
	m_SkullMat.Reflect = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

	m_CenterSphereMat.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_CenterSphereMat.Diffuse = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_CenterSphereMat.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
	m_CenterSphereMat.Reflect = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);

}

DynamicCubeMapApp::~DynamicCubeMapApp()
{
	d3d_context_->ClearState();
	ReleaseCOM(m_ShapesVB);
	ReleaseCOM(m_ShapesIB);
	ReleaseCOM(m_SkullVB);
	ReleaseCOM(m_SkullIB);
	ReleaseCOM(m_FloorTexSRV);
	ReleaseCOM(m_StoneTexSRV);
	ReleaseCOM(m_BrickTexSRV);
	ReleaseCOM(m_DynamicCubeMapDSV);
	ReleaseCOM(m_DynamicCubeMapSRV);

	for (int i = 0; i < 6; ++i)
	{
		ReleaseCOM(m_DynamicCubeMapRTV[i]);
	}

	Effects::DestroyAll();
	InputLayouts::DestroyAll();
}

bool DynamicCubeMapApp::Init()
{
	if (!D3DApp::Init())
	{
		return false;
	}

	Effects::InitAll(d3d_device_);
	InputLayouts::InitAll(d3d_device_);

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

	BuildCubeFaceCameras(0.f, 2.f, 0.f);
	BuildDynamicCubeMapViews();

	BuildShapeGeometryBuffers();
	BuildSkullGeometryBuffers();

	m_Skys[0] = new Sky(d3d_device_, L"Textures/grasscube1024.dds", 5000.f);
	m_Skys[1] = new Sky(d3d_device_, L"Textures/snowcube1024.dds", 5000.f);
	m_Skys[2] = new Sky(d3d_device_, L"Textures/sunsetcube1024.dds", 5000.f);
	m_CurrentSky = m_Skys[0];

	return true;
}

void DynamicCubeMapApp::OnResize()
{
	D3DApp::OnResize();

	m_Camera.SetLens(.25f * MathHelper::Pi, AspectRatio(), 1.f, 1000.f);
}

void DynamicCubeMapApp::UpdateScene(float dt)
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
	// Animate the skull around the center sphere.
	//
	XMMATRIX skullScale = XMMatrixScaling(0.2f, 0.2f, 0.2f);
	XMMATRIX skullOffset = XMMatrixTranslation(3.0f, 2.0f, 0.0f);
	XMMATRIX skullLocalRotate = XMMatrixRotationY(2.0f * timer_.TotalTime());
	XMMATRIX skullGlobalRotate = XMMatrixRotationY(0.5f * timer_.TotalTime());
	XMStoreFloat4x4(&m_SkullWorld, skullScale * skullLocalRotate * skullOffset * skullGlobalRotate);

	m_Camera.UpdateViewMatrix();
}

void DynamicCubeMapApp::DrawScene()
{
	ID3D11RenderTargetView* renderTarget = nullptr;
	
	// Generate the cube map.
	d3d_context_->RSSetViewports(1, &m_CubeMapViewport);
	for (int i = 0; i < 6; ++i)
	{
		// Clear cube map face and depth buffer.
		d3d_context_->ClearRenderTargetView(m_DynamicCubeMapRTV[i], (const float*)&Colors::Silver);
		d3d_context_->ClearDepthStencilView(m_DynamicCubeMapDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);
		
		// Bind cube map face as render target.
		renderTarget = m_DynamicCubeMapRTV[i];
		d3d_context_->OMSetRenderTargets(1, &renderTarget, m_DynamicCubeMapDSV);

		DrawScene(m_CubeMapCamera[i], false);
	}

	// Restore old viewport and render targets.
	d3d_context_->RSSetViewports(1, &screen_viewport_);
	renderTarget = render_target_view_;
	d3d_context_->OMSetRenderTargets(1, &renderTarget, depth_stencil_view_);

	// Have hardware generate lower mipmap levels of cube map.
	d3d_context_->GenerateMips(m_DynamicCubeMapSRV);

	// Now draw the scene as normal, but with the center sphere.
	d3d_context_->ClearRenderTargetView(render_target_view_, (const float*)&Colors::Silver);
	d3d_context_->ClearDepthStencilView(depth_stencil_view_, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

	DrawScene(m_Camera, true);

	HR(swap_chain_->Present(0, 0));
}

void DynamicCubeMapApp::DrawScene(const Camera& camera, bool drawCenterSphere)
{
	d3d_context_->IASetInputLayout(InputLayouts::Basic32);
	d3d_context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	XMMATRIX viewProj = camera.ViewProj();

	UINT stride = sizeof(Vertex::Basic32);
	UINT offset = 0;

	// Set per frame constants.
	Effects::BasicFX->SetDirLights(m_DirLights);
	Effects::BasicFX->SetEyePosW(camera.GetPosition());

	ID3DX11EffectTechnique* texTech = Effects::BasicFX->Light3TexTech;
	ID3DX11EffectTechnique* reflectTech = Effects::BasicFX->Light3TexReflectTech;
	ID3DX11EffectTechnique* skullTech = Effects::BasicFX->Light3TexTech;

	XMMATRIX world;
	XMMATRIX worldInvTranspose;
	XMMATRIX worldViewProj;

	D3DX11_TECHNIQUE_DESC techDesc;

	//
	// Draw the Skull without any cubemap reflection.
	//
	skullTech->GetDesc(&techDesc);
	for (int p = 0; p < techDesc.Passes; ++p)
	{
		d3d_context_->IASetVertexBuffers(0, 1, &m_SkullVB, &stride, &offset);
		d3d_context_->IASetIndexBuffer(m_SkullIB, DXGI_FORMAT_R32_UINT, 0);

		world = XMLoadFloat4x4(&m_SkullWorld);
		worldInvTranspose = MathHelper::InverseTranspose(world);
		worldViewProj = world * viewProj;

		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(worldViewProj);
		Effects::BasicFX->SetTexTransform(XMMatrixIdentity());
		Effects::BasicFX->SetMaterial(m_SkullMat);

		skullTech->GetPassByIndex(p)->Apply(0, d3d_context_);
		d3d_context_->DrawIndexed(m_SkullIndexCount, 0, 0);
	}

	//
	// Draw the grid, cylinders, spheres and box without any cubemap reflection.
	// 
	texTech->GetDesc(&techDesc);
	for (int p = 0; p < techDesc.Passes; ++p)
	{
		d3d_context_->IASetVertexBuffers(0, 1, &m_ShapesVB, &stride, &offset);
		d3d_context_->IASetIndexBuffer(m_ShapesIB, DXGI_FORMAT_R32_UINT, 0);

		// Draw the grid.
		world = XMLoadFloat4x4(&m_GridWorld);
		worldInvTranspose = MathHelper::InverseTranspose(world);
		worldViewProj = world * viewProj;

		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(worldViewProj);
		Effects::BasicFX->SetTexTransform(XMMatrixScaling(6.0f, 8.0f, 1.0f));
		Effects::BasicFX->SetMaterial(m_GridMat);
		Effects::BasicFX->SetDiffuseMap(m_FloorTexSRV);

		texTech->GetPassByIndex(p)->Apply(0, d3d_context_);
		d3d_context_->DrawIndexed(m_GridIndexCount, m_GridIndexOffset, m_GridVertexOffset);

		// Draw the box.
		world = XMLoadFloat4x4(&m_BoxWorld);
		worldInvTranspose = MathHelper::InverseTranspose(world);
		worldViewProj = world * viewProj;

		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(worldViewProj);
		Effects::BasicFX->SetTexTransform(XMMatrixIdentity());
		Effects::BasicFX->SetMaterial(m_BoxMat);
		Effects::BasicFX->SetDiffuseMap(m_StoneTexSRV);

		texTech->GetPassByIndex(p)->Apply(0, d3d_context_);
		d3d_context_->DrawIndexed(m_BoxIndexCount, m_BoxIndexOffset, m_BoxVertexOffset);

		// Draw the cylinders.
		for (int i = 0; i < 10; ++i)
		{
			world = XMLoadFloat4x4(&m_CylWorld[i]);
			worldInvTranspose = MathHelper::InverseTranspose(world);
			worldViewProj = world * viewProj;

			Effects::BasicFX->SetWorld(world);
			Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
			Effects::BasicFX->SetWorldViewProj(worldViewProj);
			Effects::BasicFX->SetTexTransform(XMMatrixIdentity());
			Effects::BasicFX->SetMaterial(m_CylinderMat);
			Effects::BasicFX->SetDiffuseMap(m_BrickTexSRV);

			texTech->GetPassByIndex(p)->Apply(0, d3d_context_);
			d3d_context_->DrawIndexed(m_CylinderIndexCount, m_CylinderIndexOffset, m_CylinderVertexOffset);
		}

		// Draw the spheres.
		for (int i = 0; i < 10; ++i)
		{
			world = XMLoadFloat4x4(&m_SphereWorld[i]);
			worldInvTranspose = MathHelper::InverseTranspose(world);
			worldViewProj = world * viewProj;

			Effects::BasicFX->SetWorld(world);
			Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
			Effects::BasicFX->SetWorldViewProj(worldViewProj);
			Effects::BasicFX->SetTexTransform(XMMatrixIdentity());
			Effects::BasicFX->SetMaterial(m_SphereMat);
			Effects::BasicFX->SetDiffuseMap(m_StoneTexSRV);

			texTech->GetPassByIndex(p)->Apply(0, d3d_context_);
			d3d_context_->DrawIndexed(m_SphereIndexCount, m_SphereIndexOffset, m_SphereVertexOffset);
		}
	}

	//
	// Draw the center sphere with the dynamic cube map.
	//
	if (drawCenterSphere)
	{
		reflectTech->GetDesc(&techDesc);
		for (int p = 0; p < techDesc.Passes; ++p)
		{
			// Draw the spheres.

			world = XMLoadFloat4x4(&m_CenterSphereWorld);
			worldInvTranspose = MathHelper::InverseTranspose(world);
			worldViewProj = world * viewProj;

			Effects::BasicFX->SetWorld(world);
			Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
			Effects::BasicFX->SetWorldViewProj(worldViewProj);
			Effects::BasicFX->SetTexTransform(XMMatrixIdentity());
			Effects::BasicFX->SetMaterial(m_CenterSphereMat);
			Effects::BasicFX->SetDiffuseMap(m_StoneTexSRV);
			Effects::BasicFX->SetCubeMap(m_DynamicCubeMapSRV);

			reflectTech->GetPassByIndex(p)->Apply(0, d3d_context_);
			d3d_context_->DrawIndexed(m_SphereIndexCount, m_SphereIndexOffset, m_SphereVertexOffset);
		}
	}
	
	m_CurrentSky->Draw(d3d_context_, camera);

	// restore default states, as the SkyFX changes them in the effect file.
	d3d_context_->RSSetState(nullptr);
	d3d_context_->OMSetDepthStencilState(nullptr, 0);
}

void DynamicCubeMapApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	m_LastMousePos.x = x;
	m_LastMousePos.y = y;

	SetCapture(main_wnd_);
}

void DynamicCubeMapApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void DynamicCubeMapApp::OnMouseMove(WPARAM btnState, int x, int y)
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

void DynamicCubeMapApp::BuildCubeFaceCameras(float x, float y, float z)
{
	// Generate the cube map about the given position.
	XMFLOAT3 center(x, y, z);
	
	// Look along each coordinate axis.
	XMFLOAT3 targets[6] =
	{
		XMFLOAT3(x + 1.0f, y, z), // +X
		XMFLOAT3(x - 1.0f, y, z), // -X
		XMFLOAT3(x, y + 1.0f, z), // +Y
		XMFLOAT3(x, y - 1.0f, z), // -Y
		XMFLOAT3(x, y, z + 1.0f), // +Z
		XMFLOAT3(x, y, z - 1.0f)  // -Z
	};

	// Use world up vector (0,1,0) for all directions except +Y/-Y.  In these cases, we
	// are looking down +Y or -Y, so we need a different "up" vector.
	XMFLOAT3 ups[6] =
	{
		XMFLOAT3(0.0f, 1.0f, 0.0f),  // +X
		XMFLOAT3(0.0f, 1.0f, 0.0f),  // -X
		XMFLOAT3(0.0f, 0.0f, -1.0f), // +Y
		XMFLOAT3(0.0f, 0.0f, +1.0f), // -Y
		XMFLOAT3(0.0f, 1.0f, 0.0f),	 // +Z
		XMFLOAT3(0.0f, 1.0f, 0.0f)	 // -Z
	};

	for (int i = 0; i < 6; ++i)
	{
		m_CubeMapCamera[i].LookAt(center, targets[i], ups[i]);
		m_CubeMapCamera[i].SetLens(0.5f * MathHelper::Pi, 1.f, 0.1f, 1000.f);
		m_CubeMapCamera[i].UpdateViewMatrix();
	}
}

void DynamicCubeMapApp::BuildDynamicCubeMapViews()
{
	//
	// Cubemap is a special texture array with 6 elements.
	//
	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = CubeMapSize;
	texDesc.Height = CubeMapSize;
	texDesc.MipLevels = 0;
	texDesc.ArraySize = 6;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS | D3D11_RESOURCE_MISC_TEXTURECUBE;

	ID3D11Texture2D* cubeTex;
	HR(d3d_device_->CreateTexture2D(&texDesc, nullptr, &cubeTex));

	//
	// Create a render target view to each cube map face 
	// (i.e., each element in the texture array).
	// 
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
	rtvDesc.Format = texDesc.Format;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
	rtvDesc.Texture2DArray.ArraySize = 1;
	rtvDesc.Texture2DArray.MipSlice = 0;

	for (int i = 0; i < 6; ++i)
	{
		rtvDesc.Texture2DArray.FirstArraySlice = i;
		HR(d3d_device_->CreateRenderTargetView(cubeTex, &rtvDesc, &m_DynamicCubeMapRTV[i]));
	}

	//
	// Create a shader resource view to the cube map.
	//
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = -1;

	HR(d3d_device_->CreateShaderResourceView(cubeTex, &srvDesc, &m_DynamicCubeMapSRV));

	ReleaseCOM(cubeTex);

	//
	// We need a depth texture for rendering the scene into the cubemap
	// that has the same resolution as the cubemap faces.  
	//

	D3D11_TEXTURE2D_DESC depthTexDesc;
	depthTexDesc.Width = CubeMapSize;
	depthTexDesc.Height = CubeMapSize;
	depthTexDesc.MipLevels = 1;
	depthTexDesc.ArraySize = 1;
	depthTexDesc.SampleDesc.Count = 1; 
	depthTexDesc.SampleDesc.Quality = 0;
	depthTexDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthTexDesc.Usage = D3D11_USAGE_DEFAULT;
	depthTexDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthTexDesc.CPUAccessFlags = 0;
	depthTexDesc.MiscFlags = 0;

	ID3D11Texture2D* depthTex = nullptr;
	HR(d3d_device_->CreateTexture2D(&depthTexDesc, nullptr, &depthTex));
	
	// Create the depth stencil view for the entire cube
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Format = depthTexDesc.Format;
	dsvDesc.Flags = 0;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;
	HR(d3d_device_->CreateDepthStencilView(depthTex, &dsvDesc, &m_DynamicCubeMapDSV));

	ReleaseCOM(depthTex);

	//
	// Viewport for drawing into cubemap.
	// 
	m_CubeMapViewport.TopLeftX = 0.0f;
	m_CubeMapViewport.TopLeftY = 0.0f;
	m_CubeMapViewport.Width = (float)CubeMapSize;
	m_CubeMapViewport.Height = (float)CubeMapSize;
	m_CubeMapViewport.MinDepth = 0.0f;
	m_CubeMapViewport.MaxDepth = 1.0f;
}

void DynamicCubeMapApp::BuildShapeGeometryBuffers()
{
	GeometryGenerator::MeshData box;
	GeometryGenerator::MeshData grid;
	GeometryGenerator::MeshData sphere;
	GeometryGenerator::MeshData cylinder;

	GeometryGenerator geoGen;
	geoGen.CreateBox(1.0f, 1.0f, 1.0f, box);
	geoGen.CreateGrid(20.0f, 30.0f, 60, 40, grid);
	geoGen.CreateSphere(0.5f, 20, 20, sphere);
	geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20, cylinder);

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

	std::vector<Vertex::Basic32> vertices(totalVertexCount);

	UINT k = 0;
	for (size_t i = 0; i < box.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = box.Vertices[i].Position;
		vertices[k].Normal = box.Vertices[i].Normal;
		vertices[k].Tex = box.Vertices[i].TexC;
	}

	for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = grid.Vertices[i].Position;
		vertices[k].Normal = grid.Vertices[i].Normal;
		vertices[k].Tex = grid.Vertices[i].TexC;
	}

	for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = sphere.Vertices[i].Position;
		vertices[k].Normal = sphere.Vertices[i].Normal;
		vertices[k].Tex = sphere.Vertices[i].TexC;
	}

	for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = cylinder.Vertices[i].Position;
		vertices[k].Normal = cylinder.Vertices[i].Normal;
		vertices[k].Tex = cylinder.Vertices[i].TexC;
	}

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex::Basic32) * totalVertexCount;
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

void DynamicCubeMapApp::BuildSkullGeometryBuffers()
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

	DynamicCubeMapApp theApp(hInstance);

	if (!theApp.Init())
	{
		return 0;
	}

	return theApp.Run();
}