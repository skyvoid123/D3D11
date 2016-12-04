#include "d3dApp.h"
#include "d3dx11Effect.h"
#include "GeometryGenerator.h"
#include "MathHelper.h"
#include "LightHelper.h"
#include "../Effects.h"
#include "../Vertex.h"
#include "../RenderStates.h"

#include "Camera.h"

class BezierPatchApp : public D3DApp
{
public:
	BezierPatchApp(HINSTANCE hInstance);
	~BezierPatchApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

	void OnMouseDown(WPARAM btnState, int x, int y);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y);

private:
	void BuildQuadPatchBuffer();

private:
	ID3D11Buffer* m_QuadPatchVB;

	Camera m_Camera;

	POINT m_LastMousePos;
};

BezierPatchApp::BezierPatchApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
	, m_QuadPatchVB(nullptr)
{
	main_wnd_caption_ = L"Bezier Surface Demo";
	enable_4x_msaa_ = false;
}

BezierPatchApp::~BezierPatchApp()
{
	d3d_context_->ClearState();
	ReleaseCOM(m_QuadPatchVB);

	Effects::DestroyAll();
	InputLayouts::DestroyAll();
	RenderStates::DestroyAll();
}

bool BezierPatchApp::Init()
{
	if (!D3DApp::Init())
	{
		return false;
	}

	Effects::InitAll(d3d_device_);
	InputLayouts::InitAll(d3d_device_);
	RenderStates::InitAll(d3d_device_);

	d3d_context_->IASetInputLayout(InputLayouts::Pos);
	d3d_context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST);

	d3d_context_->RSSetState(RenderStates::WireframeRS);

	BuildQuadPatchBuffer();
	UINT stride = sizeof(Vertex::Pos);
	UINT offset = 0;
	d3d_context_->IASetVertexBuffers(0, 1, &m_QuadPatchVB, &stride, &offset);

	return true;
}

void BezierPatchApp::OnResize()
{
	D3DApp::OnResize();

	m_Camera.SetLens(.25f * MathHelper::Pi, AspectRatio(), 1.f, 1000.f);
}

void BezierPatchApp::UpdateScene(float dt)
{
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

void BezierPatchApp::DrawScene()
{
	d3d_context_->ClearRenderTargetView(render_target_view_, (const float*)&Colors::Silver);
	d3d_context_->ClearDepthStencilView(depth_stencil_view_, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

	m_Camera.UpdateViewMatrix();
	XMMATRIX view = m_Camera.View();
	XMMATRIX proj = m_Camera.Proj();
	XMMATRIX world = XMMatrixIdentity();
	XMMATRIX wvp = world* view * proj;

	auto fx = Effects::TessellationFX;
	fx->SetEyePosW(m_Camera.GetPosition());
	fx->SetWorld(world);
	fx->SetWorldViewProj(wvp);
	auto tech = fx->BezierTessellationTech;
	
	D3DX11_TECHNIQUE_DESC desc;
	tech->GetDesc(&desc);
	for (int p = 0; p < desc.Passes; ++p)
	{
		tech->GetPassByIndex(p)->Apply(0, d3d_context_);
		d3d_context_->Draw(16, 0);
	}

	HR(swap_chain_->Present(0, 0));
}

void BezierPatchApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	m_LastMousePos.x = x;
	m_LastMousePos.y = y;

	SetCapture(main_wnd_);
}

void BezierPatchApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void BezierPatchApp::OnMouseMove(WPARAM btnState, int x, int y)
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

void BezierPatchApp::BuildQuadPatchBuffer()
{
	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.ByteWidth = sizeof(Vertex::Pos) * 16;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;

	Vertex::Pos vertices[16] =
	{
		// Row 0
		Vertex::Pos(-10.0f, -10.0f, +15.0f),
		Vertex::Pos(-5.0f,  0.0f, +15.0f),
		Vertex::Pos(+5.0f,  0.0f, +15.0f),
		Vertex::Pos(+10.0f, 0.0f, +15.0f),

		// Row 1
		Vertex::Pos(-15.0f, 0.0f, +5.0f),
		Vertex::Pos(-5.0f,  0.0f, +5.0f),
		Vertex::Pos(+5.0f,  20.0f, +5.0f),
		Vertex::Pos(+15.0f, 0.0f, +5.0f),

		// Row 2
		Vertex::Pos(-15.0f, 0.0f, -5.0f),
		Vertex::Pos(-5.0f,  0.0f, -5.0f),
		Vertex::Pos(+5.0f,  0.0f, -5.0f),
		Vertex::Pos(+15.0f, 0.0f, -5.0f),

		// Row 3
		Vertex::Pos(-10.0f, 10.0f, -15.0f),
		Vertex::Pos(-5.0f,  0.0f, -15.0f),
		Vertex::Pos(+5.0f,  0.0f, -15.0f),
		Vertex::Pos(+25.0f, 10.0f, -15.0f)
	};

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = vertices;

	HR(d3d_device_->CreateBuffer(&vbd, &data, &m_QuadPatchVB));
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	BezierPatchApp theApp(hInstance);

	if (!theApp.Init())
	{
		return 0;
	}

	return theApp.Run();
}