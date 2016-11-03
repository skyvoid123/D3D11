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

class BlendApp : public D3DApp
{
public:
	BlendApp(HINSTANCE hInstance);
	~BlendApp();

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

private:
	ID3D11Buffer* m_LandVB;
	ID3D11Buffer* m_LandIB;

	ID3D11Buffer* m_WaveVB;
	ID3D11Buffer* m_WaveIB;

	ID3D11Buffer* m_CrateVB;
	ID3D11Buffer* m_CrateIB;

	ID3D11ShaderResourceView* m_GrassMapSRV;
	ID3D11ShaderResourceView* m_WaveMapSRV;
	ID3D11ShaderResourceView* m_CrateMapSRV;

	Waves m_Wave;

	DirectionalLight m_DirLights[3];
	Material m_LandMat;
	Material m_WaveMat;
	Material m_CrateMat;

	XMMATRIX m_GrassTexTransform;
	XMMATRIX m_WaveTexTransform;
	
	XMMATRIX m_LandWorld;
	XMMATRIX m_WaveWorld;
	XMMATRIX m_CrateWorld;

	XMMATRIX m_View;
	XMMATRIX m_Proj;

	UINT m_LandIndexCount;

	XMFLOAT2 m_WaterTexOffset;

	RenderOptions m_RenderOptions;

	XMFLOAT3 m_EyePosW;

	float m_Radius;
	float m_Phi;
	float m_Theta;

	POINT m_LastMousePos;
};

BlendApp::BlendApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
	, m_LandVB(nullptr)
	, m_LandIB(nullptr)
	, m_WaveVB(nullptr)
	, m_WaveIB(nullptr)
	, m_GrassMapSRV(nullptr)
	, m_WaveMapSRV(nullptr)
	, m_CrateMapSRV(nullptr)
	, m_LandIndexCount(0)
	, m_WaterTexOffset(0.f, 0.f)
	, m_RenderOptions(RenderOptions::TexturesAndFog)
	, m_EyePosW(0.f, 0.f, 0.f)
	, m_Radius(80.f)
	, m_Phi(.4f * MathHelper::Pi)
	, m_Theta(1.3f * MathHelper::Pi)
{
	main_wnd_caption_ = L"Blend Demo";
	enable_4x_msaa_ = false;

	XMMATRIX I = XMMatrixIdentity();
	m_LandWorld = I;
	m_WaveWorld = I;
	m_View = I;
	m_Proj = I;

	XMMATRIX crateScale = XMMatrixScaling(15.f, 15.f, 15.f);
	XMMATRIX crateOffset = XMMatrixTranslation(8.f, 5.f, -15.f);
	m_CrateWorld = crateScale * crateOffset;

	m_GrassTexTransform = XMMatrixScaling(5.f, 5.f, 0.f);

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
}

BlendApp::~BlendApp()
{
	d3d_context_->ClearState();
	ReleaseCOM(m_LandVB);
	ReleaseCOM(m_LandIB);
	ReleaseCOM(m_WaveVB);
	ReleaseCOM(m_WaveIB);
	ReleaseCOM(m_CrateVB);
	ReleaseCOM(m_CrateIB);
	ReleaseCOM(m_GrassMapSRV);
	ReleaseCOM(m_WaveMapSRV);
	ReleaseCOM(m_CrateMapSRV);

	Effects::DestroyAll();
	InputLayouts::DestroyAll();
	RenderStates::DestroyAll();
}

bool BlendApp::Init()
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

	BuildLandGeometryBuffers();
	BuildWaveGeometryBuffers();
	BuildCrateGeometryBuffers();

	return true;
}

void BlendApp::OnResize()
{
	D3DApp::OnResize();

	XMMATRIX P = XMMatrixPerspectiveFovLH(.25f * MathHelper::Pi, AspectRatio(), 1.f, 1000.f);
	m_Proj = P;
}

void BlendApp::UpdateScene(float dt)
{
	float x = m_Radius * sinf(m_Phi) * cosf(m_Theta);
	float z = m_Radius * sinf(m_Phi) * sinf(m_Theta);
	float y = m_Radius * cosf(m_Phi);

	m_EyePosW = XMFLOAT3(x, y, z);

	auto pos = XMVectorSet(x, y, z, 1.f);
	auto target = XMVectorZero();
	auto up = XMVectorSet(0.f, 1.f, 0.f, 0.f);

	auto V = XMMatrixLookAtLH(pos, target, up);
	m_View = V;

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
	m_WaveTexTransform = waveScale * waveOffset;

	//
	// Switch the render mode based in key input.
	//
	if (GetAsyncKeyState('1') & 0x8000)
		m_RenderOptions = RenderOptions::Lighting;

	if (GetAsyncKeyState('2') & 0x8000)
		m_RenderOptions = RenderOptions::Textures;

	if (GetAsyncKeyState('3') & 0x8000)
		m_RenderOptions = RenderOptions::TexturesAndFog;
}

void BlendApp::DrawScene()
{
	d3d_context_->ClearRenderTargetView(render_target_view_, (const float*)&Colors::Silver);
	d3d_context_->ClearDepthStencilView(depth_stencil_view_, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

	d3d_context_->IASetInputLayout(InputLayouts::Basic32);
	d3d_context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	float blendFactor[] = { 0.f, 0.f, 0.f, 0.f };

	UINT stride = sizeof(Vertex::Basic32);
	UINT offset = 0;

	auto viewProj = m_View * m_Proj;

	// Set per frame constants.
	Effects::BasicFX->SetDirLights(m_DirLights);
	Effects::BasicFX->SetEyePosW(m_EyePosW);
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
		auto worldInvTranspose = MathHelper::InverseTranspose(m_CrateWorld);
		auto wvp = m_CrateWorld * m_View * m_Proj;

		Effects::BasicFX->SetWorld(m_CrateWorld);
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
		auto worldInvTranspose = MathHelper::InverseTranspose(m_LandWorld);
		auto wvp = m_LandWorld * m_View * m_Proj;

		Effects::BasicFX->SetWorld(m_LandWorld);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(wvp);
		Effects::BasicFX->SetTexTransform(m_GrassTexTransform);
		Effects::BasicFX->SetMaterial(m_LandMat);
		Effects::BasicFX->SetDiffuseMap(m_GrassMapSRV);

		landAndWaveTech->GetPassByIndex(p)->Apply(0, d3d_context_);
		d3d_context_->DrawIndexed(m_LandIndexCount, 0, 0);

		// Draw the waves.
		d3d_context_->IASetVertexBuffers(0, 1, &m_WaveVB, &stride, &offset);
		d3d_context_->IASetIndexBuffer(m_WaveIB, DXGI_FORMAT_R32_UINT, 0);

		worldInvTranspose = MathHelper::InverseTranspose(m_WaveWorld);
		wvp = m_WaveWorld * m_View * m_Proj;

		Effects::BasicFX->SetWorld(m_WaveWorld);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(wvp);
		Effects::BasicFX->SetTexTransform(m_WaveTexTransform);
		Effects::BasicFX->SetMaterial(m_WaveMat);
		Effects::BasicFX->SetDiffuseMap(m_WaveMapSRV);

		d3d_context_->OMSetBlendState(RenderStates::TransparentBS, blendFactor, 0xffffffff);
		landAndWaveTech->GetPassByIndex(p)->Apply(0, d3d_context_);
		d3d_context_->DrawIndexed(3 * m_Wave.TriangleCount(), 0, 0);

		d3d_context_->OMSetBlendState(nullptr, blendFactor, 0xffffffff);
	}

	HR(swap_chain_->Present(0, 0));
}

void BlendApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	m_LastMousePos.x = x;
	m_LastMousePos.y = y;

	SetCapture(main_wnd_);
}

void BlendApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void BlendApp::OnMouseMove(WPARAM btnState, int x, int y)
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
		m_Radius = MathHelper::Clamp(m_Radius, 50.0f, 500.0f);
	}

	m_LastMousePos.x = x;
	m_LastMousePos.y = y;
}

float BlendApp::GetLandHeight(float x, float z) const
{
	return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
}

XMFLOAT3 BlendApp::GetLandNormal(float x, float z) const
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

void BlendApp::BuildLandGeometryBuffers()
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

void BlendApp::BuildWaveGeometryBuffers()
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

void BlendApp::BuildCrateGeometryBuffers()
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

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	BlendApp theApp(hInstance);

	if (!theApp.Init())
	{
		return 0;
	}

	return theApp.Run();
}