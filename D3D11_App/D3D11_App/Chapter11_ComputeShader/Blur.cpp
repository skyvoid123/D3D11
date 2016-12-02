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
#include "BlurFilter.h"
#include "Camera.h"

enum RenderOptions
{
	Lighting = 0,
	Textures = 1,
	TexturesAndFog = 2
};

class BlurApp : public D3DApp
{
public:
	BlurApp(HINSTANCE hInstance);
	~BlurApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

	void OnMouseDown(WPARAM btnState, int x, int y);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y);

private:
	float GetLandHeight(float x, float z) const;
	XMFLOAT3 GetLandNormal(float x, float z) const;
	void BuildLandGeometryBuffers();
	void BuildWaveGeometryBuffers();
	void BuildCrateGeometryBuffers();
	void BuildTreeSpritesBuffers();
	void DrawTreeSprites(CXMMATRIX viewProj);
	void DrawScreenQuad();
	void DrawWrapper();

	void BuildScreenQuadGeometryBuffers();
	void BuildOffscreenViews();
	

private:
	ID3D11Buffer* m_LandVB;
	ID3D11Buffer* m_LandIB;

	ID3D11Buffer* m_WaveVB;
	ID3D11Buffer* m_WaveIB;

	ID3D11Buffer* m_CrateVB;
	ID3D11Buffer* m_CrateIB;

	ID3D11Buffer* m_TreeSpritesVB;

	ID3D11Buffer* m_ScreenQuadVB;
	ID3D11Buffer* m_ScreenQuadIB;

	ID3D11ShaderResourceView* m_GrassMapSRV;
	ID3D11ShaderResourceView* m_WaveMapSRV;
	ID3D11ShaderResourceView* m_CrateMapSRV;
	ID3D11ShaderResourceView* m_TreeTextureMapArraySRV;

	ID3D11RenderTargetView* m_OffscreenRTV;
	ID3D11ShaderResourceView* m_OffscreenSRV;
	ID3D11UnorderedAccessView* m_OffscreenUAV;

	Waves m_Wave;
	BlurFilter m_BlurFilter;

	DirectionalLight m_DirLights[3];
	Material m_LandMat;
	Material m_WaveMat;
	Material m_CrateMat;
	Material m_TreeMat;

	XMFLOAT4X4 m_GrassTexTransform;
	XMFLOAT4X4 m_WaveTexTransform;
	
	XMFLOAT4X4 m_LandWorld;
	XMFLOAT4X4 m_WaveWorld;
	XMFLOAT4X4 m_CrateWorld;

	Camera m_Camera;

	UINT m_LandIndexCount;

	static const UINT TreeCount = 16;

	bool m_AlphaToCoverageOn;

	XMFLOAT2 m_WaterTexOffset;

	RenderOptions m_RenderOptions;

	float m_Radius;
	float m_Phi;
	float m_Theta;

	POINT m_LastMousePos;
};

BlurApp::BlurApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
	, m_LandVB(nullptr)
	, m_LandIB(nullptr)
	, m_WaveVB(nullptr)
	, m_WaveIB(nullptr)
	, m_TreeSpritesVB(nullptr)
	, m_ScreenQuadVB(nullptr)
	, m_ScreenQuadIB(nullptr)
	, m_GrassMapSRV(nullptr)
	, m_WaveMapSRV(nullptr)
	, m_CrateMapSRV(nullptr)
	, m_TreeTextureMapArraySRV(nullptr)
	, m_OffscreenRTV(nullptr)
	, m_OffscreenSRV(nullptr)
	, m_OffscreenUAV(nullptr)
	, m_LandIndexCount(0)
	, m_AlphaToCoverageOn(true)
	, m_WaterTexOffset(0.f, 0.f)
	, m_RenderOptions(RenderOptions::TexturesAndFog)
	, m_Radius(80.f)
	, m_Phi(.4f * MathHelper::Pi)
	, m_Theta(1.3f * MathHelper::Pi)
{
	main_wnd_caption_ = L"Blur Demo";
	enable_4x_msaa_ = false;

	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&m_LandWorld, I);
	XMStoreFloat4x4(&m_WaveWorld, I);

	XMMATRIX crateScale = XMMatrixScaling(15.f, 15.f, 15.f);
	XMMATRIX crateOffset = XMMatrixTranslation(8.f, 5.f, -15.f);
	XMStoreFloat4x4(&m_CrateWorld, crateScale * crateOffset);

	XMMATRIX grassTexScale = XMMatrixScaling(5.f, 5.f, 0.f);
	XMStoreFloat4x4(&m_GrassTexTransform, grassTexScale);

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

	m_LandMat.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_LandMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_LandMat.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);

	m_WaveMat.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_WaveMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
	m_WaveMat.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 32.0f);

	m_CrateMat.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_CrateMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_CrateMat.Specular = XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);

	m_TreeMat.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_TreeMat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_TreeMat.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
}

BlurApp::~BlurApp()
{
	d3d_context_->ClearState();
	ReleaseCOM(m_LandVB);
	ReleaseCOM(m_LandIB);
	ReleaseCOM(m_WaveVB);
	ReleaseCOM(m_WaveIB);
	ReleaseCOM(m_CrateVB);
	ReleaseCOM(m_CrateIB);
	ReleaseCOM(m_TreeSpritesVB);
	ReleaseCOM(m_ScreenQuadVB);
	ReleaseCOM(m_ScreenQuadIB);
	ReleaseCOM(m_GrassMapSRV);
	ReleaseCOM(m_WaveMapSRV);
	ReleaseCOM(m_CrateMapSRV);
	ReleaseCOM(m_TreeTextureMapArraySRV);
	ReleaseCOM(m_OffscreenRTV);
	ReleaseCOM(m_OffscreenSRV);
	ReleaseCOM(m_OffscreenUAV);

	Effects::DestroyAll();
	InputLayouts::DestroyAll();
	RenderStates::DestroyAll();
}

bool BlurApp::Init()
{
	if (!D3DApp::Init())
	{
		return false;
	}

	m_Wave.Init(160, 160, 1.f, 0.03f, 5.f, 0.3f);

	Effects::InitAll(d3d_device_);
	InputLayouts::InitAll(d3d_device_);
	RenderStates::InitAll(d3d_device_);

	ID3D11Resource* texRes = nullptr;
	HR(DirectX::CreateDDSTextureFromFile(d3d_device_,
		L"Textures/grass.dds", &texRes, &m_GrassMapSRV));
	ReleaseCOM(texRes);

	HR(DirectX::CreateDDSTextureFromFile(d3d_device_,
		L"Textures/water2.dds", &texRes, &m_WaveMapSRV));
	ReleaseCOM(texRes);

	HR(DirectX::CreateDDSTextureFromFile(d3d_device_,
		L"Textures/WireFence.dds", &texRes, &m_CrateMapSRV));
	ReleaseCOM(texRes);

	HR(DirectX::CreateDDSTextureFromFile(d3d_device_,
		L"Textures/TreeArray4.dds", &texRes, &m_TreeTextureMapArraySRV));
	ReleaseCOM(texRes);

	BuildLandGeometryBuffers();
	BuildWaveGeometryBuffers();
	BuildCrateGeometryBuffers();
	BuildTreeSpritesBuffers();
	BuildScreenQuadGeometryBuffers();

	return true;
}

void BlurApp::OnResize()
{
	D3DApp::OnResize();

	// Recreate the resources that depend on the client area size.
	BuildOffscreenViews();
	m_BlurFilter.Init(d3d_device_, client_width_, client_height_, DXGI_FORMAT_R8G8B8A8_UNORM);


	m_Camera.SetLens(.25f * MathHelper::Pi, AspectRatio(), 1.f, 1000.f);
}

void BlurApp::UpdateScene(float dt)
{
	static float t_base = 0.0f;
	if ((timer_.TotalTime() - t_base) >= 0.1f)
	{
		t_base += 0.1f;

		DWORD i = 5 + rand() % (m_Wave.RowCount() - 10);
		DWORD j = 5 + rand() % (m_Wave.ColumnCount() - 10);

		float r = MathHelper::RandF(0.5f, 1.0f);

		m_Wave.Disturb(i, j, r);
	}

	m_Wave.Update(dt);

	D3D11_MAPPED_SUBRESOURCE mappedData;
	HR(d3d_context_->Map(m_WaveVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));

	auto v = (Vertex::Basic32*)mappedData.pData;
	for (int i = 0; i < m_Wave.VertexCount(); ++i)
	{
		v[i].Pos = m_Wave[i];
		v[i].Normal = m_Wave.Normal(i);

		// Derive tex-coords in [0,1] from position.
		v[i].Tex.x = 0.5f + m_Wave[i].x / m_Wave.Width();
		v[i].Tex.y = 0.5f - m_Wave[i].z / m_Wave.Depth();
	}

	d3d_context_->Unmap(m_WaveVB, 0);

	auto waveScale = XMMatrixScaling(5.f, 5.f, 0.f);
	m_WaterTexOffset.x += .1f * dt;
	m_WaterTexOffset.y += .05f * dt;
	auto waveOffset = XMMatrixTranslation(m_WaterTexOffset.x, m_WaterTexOffset.y, 0.f);
	// Combine scale and translation.
	XMStoreFloat4x4(&m_WaveTexTransform, waveScale * waveOffset);

	//
	// Switch the render mode based in key input.
	//
	if (GetAsyncKeyState('1') & 0x8000)
		m_RenderOptions = RenderOptions::Lighting;

	if (GetAsyncKeyState('2') & 0x8000)
		m_RenderOptions = RenderOptions::Textures;

	if (GetAsyncKeyState('3') & 0x8000)
		m_RenderOptions = RenderOptions::TexturesAndFog;

	if (GetAsyncKeyState('R') & 0x8000)
		m_AlphaToCoverageOn = true;

	if (GetAsyncKeyState('T') & 0x8000)
		m_AlphaToCoverageOn = false;

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
}

void BlurApp::DrawScene()
{
	// Render to our offscreen texture.  Note that we can use the same depth/stencil buffer
	// we normally use since our offscreen texture matches the dimensions.  
	d3d_context_->OMSetRenderTargets(1, &m_OffscreenRTV, depth_stencil_view_);
	d3d_context_->ClearRenderTargetView(m_OffscreenRTV, (const float*)&Colors::Silver);
	d3d_context_->ClearDepthStencilView(depth_stencil_view_, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

	//
	// Draw the scene to the offscreen texture
	//
	DrawWrapper();

	//
	// Restore the back buffer.  The offscreen render target will serve as an input into
	// the compute shader for blurring, so we must unbind it from the OM stage before we
	// can use it as an input into the compute shader.
	//

	d3d_context_->OMSetRenderTargets(1, &render_target_view_, depth_stencil_view_);
	m_BlurFilter.SetGaussianWeights(4.f);
	m_BlurFilter.BlurInPlace(d3d_context_, m_OffscreenSRV, m_OffscreenUAV, 4);

	//
	// Draw fullscreen quad with texture of blurred scene on it.
	//

	d3d_context_->ClearRenderTargetView(render_target_view_, (const float*)&Colors::Silver);
	d3d_context_->ClearDepthStencilView(depth_stencil_view_, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

	DrawScreenQuad();

	HR(swap_chain_->Present(0, 0));
}

void BlurApp::DrawTreeSprites(CXMMATRIX viewProj)
{
	Effects::TreeSpriteFX->SetDirLights(m_DirLights);
	Effects::TreeSpriteFX->SetEyePosW(m_Camera.GetPosition());
	Effects::TreeSpriteFX->SetFogColor(Colors::Silver);
	Effects::TreeSpriteFX->SetFogStart(15.f);
	Effects::TreeSpriteFX->SetFogRange(175.f);
	Effects::TreeSpriteFX->SetViewProj(viewProj);
	Effects::TreeSpriteFX->SetMaterial(m_TreeMat);
	Effects::TreeSpriteFX->SetTreeTextureMapArray(m_TreeTextureMapArraySRV);

	d3d_context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	d3d_context_->IASetInputLayout(InputLayouts::TreePointSprite);

	UINT stride = sizeof(Vertex::TreePointSprite);
	UINT offset = 0;

	d3d_context_->IASetVertexBuffers(0, 1, &m_TreeSpritesVB, &stride, &offset);


	ID3DX11EffectTechnique* treeTech;
	switch (m_RenderOptions)
	{
	case Lighting:
		treeTech = Effects::TreeSpriteFX->Light3Tech;
		break;
	case Textures:
		treeTech = Effects::TreeSpriteFX->Light3TexAlphaClipTech;
		break;
	case TexturesAndFog:
		treeTech = Effects::TreeSpriteFX->Light3TexAlphaClipFogTech;
		break;
	default:
		break;
	}

	D3DX11_TECHNIQUE_DESC desc;
	treeTech->GetDesc(&desc);
	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	if (m_AlphaToCoverageOn)
	{
		d3d_context_->OMSetBlendState(RenderStates::AlphaToCoverageBS, blendFactor, 0xffffffff);
	}
	for (int p = 0; p < desc.Passes; ++p)
	{
		treeTech->GetPassByIndex(p)->Apply(0, d3d_context_);
		d3d_context_->Draw(TreeCount, 0);
	}
	d3d_context_->OMSetBlendState(nullptr, blendFactor, 0xffffffff);

}

void BlurApp::DrawScreenQuad()
{
	d3d_context_->IASetInputLayout(InputLayouts::Basic32);
	d3d_context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT stride = sizeof(Vertex::Basic32);
	UINT offset = 0;

	XMMATRIX identity = XMMatrixIdentity();

	d3d_context_->IASetVertexBuffers(0, 1, &m_ScreenQuadVB, &stride, &offset);
	d3d_context_->IASetIndexBuffer(m_ScreenQuadIB, DXGI_FORMAT_R32_UINT, 0);

	auto texTech = Effects::BasicFX->Light0TexTech;
	D3DX11_TECHNIQUE_DESC desc;
	texTech->GetDesc(&desc);
	for (int p = 0; p < desc.Passes; ++p)
	{
		Effects::BasicFX->SetWorld(identity);
		Effects::BasicFX->SetWorldInvTranspose(identity);
		Effects::BasicFX->SetWorldViewProj(identity);
		Effects::BasicFX->SetTexTransform(identity);
		Effects::BasicFX->SetDiffuseMap(m_OffscreenSRV);

		texTech->GetPassByIndex(p)->Apply(0, d3d_context_);
		d3d_context_->DrawIndexed(6, 0, 0);
	}

}

void BlurApp::DrawWrapper()
{
	m_Camera.UpdateViewMatrix();
	XMMATRIX view = m_Camera.View();
	XMMATRIX proj = m_Camera.Proj();
	XMMATRIX viewProj = view * proj;

	// Draw the tree sprites
	DrawTreeSprites(viewProj);

	d3d_context_->IASetInputLayout(InputLayouts::Basic32);
	d3d_context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	float blendFactor[] = { 0.f, 0.f, 0.f, 0.f };

	UINT stride = sizeof(Vertex::Basic32);
	UINT offset = 0;

	// Set per frame constants.
	Effects::BasicFX->SetDirLights(m_DirLights);
	Effects::BasicFX->SetEyePosW(m_Camera.GetPosition());
	Effects::BasicFX->SetFogColor(Colors::Silver);
	Effects::BasicFX->SetFogStart(15.f);
	Effects::BasicFX->SetFogRange(175.f);

	ID3DX11EffectTechnique* boxTech;
	ID3DX11EffectTechnique* landAndWaveTech;

	switch (m_RenderOptions)
	{
	case Lighting:
		boxTech = Effects::BasicFX->Light3Tech;
		landAndWaveTech = Effects::BasicFX->Light3Tech;
		break;
	case Textures:
		boxTech = Effects::BasicFX->Light3TexAlphaClipTech;
		landAndWaveTech = Effects::BasicFX->Light3TexTech;
		break;
	case TexturesAndFog:
		boxTech = Effects::BasicFX->Light3TexAlphaClipFogTech;
		landAndWaveTech = Effects::BasicFX->Light3TexFogTech;
		break;
	default:
		break;
	}

	D3DX11_TECHNIQUE_DESC techDesc;

	// Draw the box with alpha clipping.
	boxTech->GetDesc(&techDesc);
	for (int p = 0; p < techDesc.Passes; ++p)
	{
		d3d_context_->IASetVertexBuffers(0, 1, &m_CrateVB, &stride, &offset);
		d3d_context_->IASetIndexBuffer(m_CrateIB, DXGI_FORMAT_R32_UINT, 0);

		// Set per object constants.
		auto world = XMLoadFloat4x4(&m_CrateWorld);
		auto worldInvTranspose = MathHelper::InverseTranspose(world);
		auto wvp = world * viewProj;

		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(wvp);
		Effects::BasicFX->SetTexTransform(XMMatrixIdentity());
		Effects::BasicFX->SetMaterial(m_CrateMat);
		Effects::BasicFX->SetDiffuseMap(m_CrateMapSRV);

		d3d_context_->RSSetState(RenderStates::NoCullRS);
		boxTech->GetPassByIndex(p)->Apply(0, d3d_context_);
		d3d_context_->DrawIndexed(36, 0, 0);

		d3d_context_->RSSetState(nullptr);
	}

	// Draw the land and water with texture and fog (no alpha clipping needed).
	landAndWaveTech->GetDesc(&techDesc);
	for (int p = 0; p < techDesc.Passes; ++p)
	{
		// Draw the land.
		d3d_context_->IASetVertexBuffers(0, 1, &m_LandVB, &stride, &offset);
		d3d_context_->IASetIndexBuffer(m_LandIB, DXGI_FORMAT_R32_UINT, 0);

		// Set per object constants.
		auto world = XMLoadFloat4x4(&m_LandWorld);
		auto worldInvTranspose = MathHelper::InverseTranspose(world);
		auto wvp = world * viewProj;

		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(wvp);
		Effects::BasicFX->SetTexTransform(XMLoadFloat4x4(&m_GrassTexTransform));
		Effects::BasicFX->SetMaterial(m_LandMat);
		Effects::BasicFX->SetDiffuseMap(m_GrassMapSRV);

		landAndWaveTech->GetPassByIndex(p)->Apply(0, d3d_context_);
		d3d_context_->DrawIndexed(m_LandIndexCount, 0, 0);

		// Draw the waves.
		d3d_context_->IASetVertexBuffers(0, 1, &m_WaveVB, &stride, &offset);
		d3d_context_->IASetIndexBuffer(m_WaveIB, DXGI_FORMAT_R32_UINT, 0);

		world = XMLoadFloat4x4(&m_WaveWorld);
		worldInvTranspose = MathHelper::InverseTranspose(world);
		wvp = world * viewProj;

		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(wvp);
		Effects::BasicFX->SetTexTransform(XMLoadFloat4x4(&m_WaveTexTransform));
		Effects::BasicFX->SetMaterial(m_WaveMat);
		Effects::BasicFX->SetDiffuseMap(m_WaveMapSRV);

		d3d_context_->OMSetBlendState(RenderStates::TransparentBS, blendFactor, 0xffffffff);
		landAndWaveTech->GetPassByIndex(p)->Apply(0, d3d_context_);
		d3d_context_->DrawIndexed(3 * m_Wave.TriangleCount(), 0, 0);

		d3d_context_->OMSetBlendState(nullptr, blendFactor, 0xffffffff);
	}
}

void BlurApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	m_LastMousePos.x = x;
	m_LastMousePos.y = y;

	SetCapture(main_wnd_);
}

void BlurApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void BlurApp::OnMouseMove(WPARAM btnState, int x, int y)
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

float BlurApp::GetLandHeight(float x, float z) const
{
	return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
}

XMFLOAT3 BlurApp::GetLandNormal(float x, float z) const
{
	// n = (-df/dx, 1, -df/dz)
	XMFLOAT3 n(
		-0.03f*z*cosf(0.1f*x) - 0.3f*cosf(0.1f*z),
		1.0f,
		-0.3f*sinf(0.1f*x) + 0.03f*x*sinf(0.1f*z));

	XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&n));
	XMStoreFloat3(&n, unitNormal);

	return n;
}

void BlurApp::BuildLandGeometryBuffers()
{
	GeometryGenerator::MeshData grid;
	GeometryGenerator geoGen;
	geoGen.CreateGrid(160.f, 160.f, 50, 50, grid);
	m_LandIndexCount = grid.Indices.size();

	std::vector<Vertex::Basic32> vertices(grid.Vertices.size());
	for (int i = 0; i < vertices.size(); ++i)
	{
		auto p = grid.Vertices[i].Position;
		p.y = GetLandHeight(p.x, p.z);

		vertices[i].Pos = p;
		vertices[i].Normal = GetLandNormal(p.x, p.z);
		vertices[i].Tex = grid.Vertices[i].TexC;
	}

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.ByteWidth = sizeof(Vertex::Basic32) * vertices.size();
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vData;
	vData.pSysMem = &vertices[0];
	HR(d3d_device_->CreateBuffer(&vbd, &vData, &m_LandVB));

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.ByteWidth = sizeof(UINT) * grid.Indices.size();
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA iData;
	iData.pSysMem = &grid.Indices[0];
	HR(d3d_device_->CreateBuffer(&ibd, &iData, &m_LandIB));
}

void BlurApp::BuildWaveGeometryBuffers()
{
	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_DYNAMIC;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.ByteWidth = sizeof(Vertex::Basic32) * m_Wave.VertexCount();
	vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vbd.MiscFlags = 0;
	HR(d3d_device_->CreateBuffer(&vbd, 0, &m_WaveVB));

	std::vector<UINT> indices(3 * m_Wave.TriangleCount());
	UINT rows = m_Wave.RowCount();
	UINT cols = m_Wave.ColumnCount();
	int k = 0;
	for (UINT i = 0; i < rows - 1; ++i)
	{
		for (UINT j = 0; j < cols - 1; ++j)
		{
			indices[k] = i * cols + j;
			indices[k + 1] = i * cols + j + 1;
			indices[k + 2] = (i + 1) * cols + j;

			indices[k + 3] = (i + 1) * cols + j;
			indices[k + 4] = i * cols + j + 1;
			indices[k + 5] = (i + 1) * cols + j + 1;

			k += 6;
		}
	}

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * indices.size();
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA iData;
	iData.pSysMem = &indices[0];
	HR(d3d_device_->CreateBuffer(&ibd, &iData, &m_WaveIB));
}

void BlurApp::BuildCrateGeometryBuffers()
{
	GeometryGenerator::MeshData box;

	GeometryGenerator geoGen;
	geoGen.CreateBox(1.0f, 1.0f, 1.0f, box);

	std::vector<Vertex::Basic32> vertices(box.Vertices.size());
	for (UINT i = 0; i < vertices.size(); ++i)
	{
		vertices[i].Pos = box.Vertices[i].Position;
		vertices[i].Normal = box.Vertices[i].Normal;
		vertices[i].Tex = box.Vertices[i].TexC;
	}

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.ByteWidth = sizeof(Vertex::Basic32) * vertices.size();
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vData;
	vData.pSysMem = &vertices[0];
	HR(d3d_device_->CreateBuffer(&vbd, &vData, &m_CrateVB));

	//
	// Pack the indices of all the meshes into one index buffer.
	//

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.ByteWidth = sizeof(UINT) * box.Indices.size();
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA iData;
	iData.pSysMem = &box.Indices[0];
	HR(d3d_device_->CreateBuffer(&ibd, &iData, &m_CrateIB));
}

void BlurApp::BuildTreeSpritesBuffers()
{
	Vertex::TreePointSprite v[TreeCount];
	for (int i = 0; i < TreeCount; ++i)
	{
		float x = MathHelper::RandF(-35.f, 35.f);
		float z = MathHelper::RandF(-35.f, 35.f);
		float y = GetLandHeight(x, z);
		// Move tree slightly above land height.
		y += 10.f;

		v[i].Pos = XMFLOAT3(x, y, z);
		v[i].Size = XMFLOAT2(24.f, 24.f);
	}

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.ByteWidth = sizeof(Vertex::TreePointSprite) * TreeCount;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vData;
	vData.pSysMem = v;
	HR(d3d_device_->CreateBuffer(&vbd, &vData, &m_TreeSpritesVB));
}

void BlurApp::BuildScreenQuadGeometryBuffers()
{
	GeometryGenerator::MeshData quad;
	GeometryGenerator geoGen;
	geoGen.CreateFullscreenQuad(quad);

	// Extract the vertex elements we are interested in and pack the
	// vertices of all the meshes into one vertex buffer.

	std::vector<Vertex::Basic32> vertices(quad.Vertices.size());
	for (int i = 0; i < vertices.size(); ++i)
	{
		vertices[i].Pos = quad.Vertices[i].Position;
		vertices[i].Normal = quad.Vertices[i].Normal;
		vertices[i].Tex = quad.Vertices[i].TexC;
	}

	D3D11_BUFFER_DESC vbd;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.ByteWidth = sizeof(Vertex::Basic32) * vertices.size();
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;

	D3D11_SUBRESOURCE_DATA vData;
	vData.pSysMem = &vertices[0];
	
	HR(d3d_device_->CreateBuffer(&vbd, &vData, &m_ScreenQuadVB));

	D3D11_BUFFER_DESC ibd;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.ByteWidth = sizeof(UINT) * quad.Indices.size();
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;

	D3D11_SUBRESOURCE_DATA iData;
	iData.pSysMem = &quad.Indices[0];

	HR(d3d_device_->CreateBuffer(&ibd, &iData, &m_ScreenQuadIB));
}

void BlurApp::BuildOffscreenViews()
{
	// We call this function everytime the window is resized so that the render target is a quarter
	// the client area dimensions.  So Release the previous views before we create new ones.
	ReleaseCOM(m_OffscreenRTV);
	ReleaseCOM(m_OffscreenSRV);
	ReleaseCOM(m_OffscreenUAV);

	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.ArraySize = 1;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	texDesc.CPUAccessFlags = 0;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.Width = client_width_;
	texDesc.Height = client_height_;
	texDesc.MipLevels = 1;
	texDesc.MiscFlags = 0;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	
	ID3D11Texture2D* offscreenTex = nullptr;
	HR(d3d_device_->CreateTexture2D(&texDesc, nullptr, &offscreenTex));

	// Null description means to create a view to all mipmap levels using 
	// the format the texture was created with.
	HR(d3d_device_->CreateRenderTargetView(offscreenTex, nullptr, &m_OffscreenRTV));
	HR(d3d_device_->CreateShaderResourceView(offscreenTex, nullptr, &m_OffscreenSRV));
	HR(d3d_device_->CreateUnorderedAccessView(offscreenTex, nullptr, &m_OffscreenUAV));

	ReleaseCOM(offscreenTex);
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	BlurApp theApp(hInstance);

	if (!theApp.Init())
	{
		return 0;
	}

	return theApp.Run();
}