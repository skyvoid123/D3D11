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
#include "Terrain.h"
#include "ParticleSystem.h"
#include "Camera.h"

class ParticlesApp : public D3DApp
{
public:
	ParticlesApp(HINSTANCE hInstance);
	~ParticlesApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

	void OnMouseDown(WPARAM btnState, int x, int y);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y);

private:
	Sky* m_Skys[3];
	Sky* m_CurrentSky;

	Terrain m_Terrain;
	
	ParticleSystem m_Fire;
	ParticleSystem m_Rain;

	ID3D11ShaderResourceView* m_FlareTexSRV;
	ID3D11ShaderResourceView* m_RainTexSRV;
	ID3D11ShaderResourceView* m_RandomTexSRV;

	DirectionalLight m_DirLights[3];

	Camera m_Camera;

	bool m_IsWalkCamMode;

	POINT m_LastMousePos;
};

ParticlesApp::ParticlesApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
	, m_CurrentSky(nullptr)
	, m_IsWalkCamMode(false)
{
	main_wnd_caption_ = L"Particles Demo";

	m_DirLights[0].Ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	m_DirLights[0].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_DirLights[0].Specular = XMFLOAT4(0.8f, 0.8f, 0.7f, 1.0f);
	m_DirLights[0].Direction = XMFLOAT3(0.707f, -0.707f, 0.0f);

	m_DirLights[1].Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	m_DirLights[1].Diffuse = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_DirLights[1].Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_DirLights[1].Direction = XMFLOAT3(0.57735f, -0.57735f, 0.57735f);

	m_DirLights[2].Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	m_DirLights[2].Diffuse = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_DirLights[2].Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_DirLights[2].Direction = XMFLOAT3(-0.57735f, -0.57735f, -0.57735f);
}

ParticlesApp::~ParticlesApp()
{
	d3d_context_->ClearState();

	for (int i = 0; i < 3; ++i)
	{
		delete m_Skys[i];
	}

	Effects::DestroyAll();
	InputLayouts::DestroyAll();
	RenderStates::DestroyAll();
}

bool ParticlesApp::Init()
{
	if (!D3DApp::Init())
	{
		return false;
	}

	Effects::InitAll(d3d_device_);
	InputLayouts::InitAll(d3d_device_);
	RenderStates::InitAll(d3d_device_);

	m_Skys[0] = new Sky(d3d_device_, L"Textures/grasscube1024.dds", 5000.f);
	m_Skys[1] = new Sky(d3d_device_, L"Textures/snowcube1024.dds", 5000.f);
	m_Skys[2] = new Sky(d3d_device_, L"Textures/sunsetcube1024.dds", 5000.f);
	m_CurrentSky = m_Skys[0];

	Terrain::InitInfo tii;
	tii.HeightMapFilename = L"Textures/terrain.raw";
	tii.LayerMapArrayFilename = L"Textures/LayerArray5.dds";
	tii.BlendMapFilename = L"Textures/blend.dds";
	tii.HeightScale = 50.0f;
	tii.HeightmapWidth = 2049;
	tii.HeightmapHeight = 2049;
	tii.CellSpacing = 0.5f;

	m_Terrain.Init(d3d_device_, d3d_context_, tii);

	m_RandomTexSRV = D3DHelper::CreateRandomTexture1DSRV(d3d_device_);

	ID3D11Resource* tex_res = nullptr;
	HR(DirectX::CreateDDSTextureFromFile(d3d_device_, L"Textures/flare0.dds", &tex_res, &m_FlareTexSRV));
	ReleaseCOM(tex_res);
	m_Fire.Init(d3d_device_, Effects::FireFX, m_FlareTexSRV, m_RandomTexSRV, 500);
	m_Fire.SetEmitPos(XMFLOAT3(0.0f, 1.0f, 120.0f));

	HR(DirectX::CreateDDSTextureFromFile(d3d_device_, L"Textures/raindrop.dds", &tex_res, &m_RainTexSRV));
	ReleaseCOM(tex_res);
	m_Rain.Init(d3d_device_, Effects::RainFX, m_RainTexSRV, m_RandomTexSRV, 10000);

	return true;
}

void ParticlesApp::OnResize()
{
	D3DApp::OnResize();

	m_Camera.SetLens(.25f * MathHelper::Pi, AspectRatio(), 1.f, 3000.f);
}

void ParticlesApp::UpdateScene(float dt)
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
		m_Camera.Walk(10.0f * dt);

	if (GetAsyncKeyState('S') & 0x8000)
		m_Camera.Walk(-10.0f * dt);

	if (GetAsyncKeyState('A') & 0x8000)
		m_Camera.Strafe(-10.0f * dt);

	if (GetAsyncKeyState('D') & 0x8000)
		m_Camera.Strafe(10.0f * dt);

	//
	// Walk/fly mode
	//
	if (GetAsyncKeyState('Q') & 0x8000)
		m_IsWalkCamMode = true;
	if (GetAsyncKeyState('E') & 0x8000)
		m_IsWalkCamMode = false;

	// 
	// Clamp camera to terrain surface in walk mode.
	//
	if (m_IsWalkCamMode)
	{
		XMFLOAT3 camPos = m_Camera.GetPosition();
		float y = m_Terrain.GetHeight(camPos.x, camPos.z);
		m_Camera.SetPosition(camPos.x, y + 2, camPos.z);
	}

	//
	// Reset particle systems.
	//
	if (GetAsyncKeyState('R') & 0x8000)
	{
		m_Fire.Reset();
		m_Rain.Reset();
	}

	m_Fire.Update(dt, timer_.TotalTime());
	m_Rain.Update(dt, timer_.TotalTime());
}

void ParticlesApp::DrawScene()
{
	d3d_context_->ClearRenderTargetView(render_target_view_, (const float*)&Colors::Silver);
	d3d_context_->ClearDepthStencilView(depth_stencil_view_, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

	float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };

	m_Camera.UpdateViewMatrix();

	if (GetAsyncKeyState('F') & 0x8000)
		d3d_context_->RSSetState(RenderStates::WireframeRS);

	m_Terrain.Draw(d3d_context_, m_Camera, m_DirLights);

	d3d_context_->RSSetState(nullptr);

	m_CurrentSky->Draw(d3d_context_, m_Camera);

	// restore default states, as the SkyFX changes them in the effect file.
	d3d_context_->RSSetState(nullptr);
	d3d_context_->OMSetDepthStencilState(nullptr, 0);

	// Draw particle systems last so it is blended with scene.
	m_Fire.SetEyePos(m_Camera.GetPosition());
	m_Fire.Draw(d3d_context_, m_Camera);
	d3d_context_->OMSetBlendState(nullptr, blendFactor, 0xffffffff); // restore default

	m_Rain.SetEyePos(m_Camera.GetPosition());
	m_Rain.SetEmitPos(m_Camera.GetPosition());
	m_Rain.Draw(d3d_context_, m_Camera);

	// restore default states.
	d3d_context_->RSSetState(nullptr);
	d3d_context_->OMSetDepthStencilState(nullptr, 0);
	d3d_context_->OMSetBlendState(nullptr, blendFactor, 0xffffffff);

	HR(swap_chain_->Present(0, 0));
}

void ParticlesApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	m_LastMousePos.x = x;
	m_LastMousePos.y = y;

	SetCapture(main_wnd_);
}

void ParticlesApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void ParticlesApp::OnMouseMove(WPARAM btnState, int x, int y)
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

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	ParticlesApp theApp(hInstance);

	if (!theApp.Init())
	{
		return 0;
	}

	return theApp.Run();
}