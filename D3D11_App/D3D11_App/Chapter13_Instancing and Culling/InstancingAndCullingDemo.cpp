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

#include "Camera.h"


struct InstanceData
{
	XMFLOAT4X4 World;
	XMFLOAT4 Color;
};

class InstancingAndCullingApp : public D3DApp
{
public:
	InstancingAndCullingApp(HINSTANCE hInstance);
	~InstancingAndCullingApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

	void OnMouseDown(WPARAM btnState, int x, int y);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y);

private:
	void BuildSkullGeometryBuffers();
	void BuildInstancedBuffer();

	void DrawLocalAABB(const Box& box);

private:
	ID3D11Buffer* m_SkullVB;
	ID3D11Buffer* m_SkullIB;

	ID3D11Buffer* m_InstancedBuffer;

	ID3D11Buffer* m_BoxVB;
	ID3D11Buffer* m_BoxIB;

	// Bounding box of the skull.
	Box m_SkullBox;
	
	UINT m_VisibleObjectCount;

	// Keep a system memory copy of the world matrices for culling.
	std::vector<InstanceData> m_InstancedData;

	bool m_IsFrustumCullingEnabled;

	DirectionalLight m_DirLights[3];
	Material m_SkullMat;

	UINT m_SkullIndexCount;

	Camera m_Camera;

	POINT m_LastMousePos;
};

InstancingAndCullingApp::InstancingAndCullingApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
	, m_SkullVB(nullptr)
	, m_SkullIB(nullptr)
	, m_InstancedBuffer(nullptr)
	, m_VisibleObjectCount(0)
	, m_IsFrustumCullingEnabled(true)
	, m_SkullIndexCount(0)
{
	main_wnd_caption_ = L"Instancing and Culling Demo";
	enable_4x_msaa_ = true;

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

	m_SkullMat.Ambient = XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);
	m_SkullMat.Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	m_SkullMat.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
}

InstancingAndCullingApp::~InstancingAndCullingApp()
{
	d3d_context_->ClearState();
	ReleaseCOM(m_SkullVB);
	ReleaseCOM(m_SkullIB);
	ReleaseCOM(m_InstancedBuffer);

	Effects::DestroyAll();
	InputLayouts::DestroyAll();
}

bool InstancingAndCullingApp::Init()
{
	if (!D3DApp::Init())
	{
		return false;
	}

	Effects::InitAll(d3d_device_);
	InputLayouts::InitAll(d3d_device_);
	RenderStates::InitAll(d3d_device_);

	BuildSkullGeometryBuffers();
	BuildInstancedBuffer();

	return true;
}

void InstancingAndCullingApp::OnResize()
{
	D3DApp::OnResize();

	m_Camera.SetLens(.25f * MathHelper::Pi, AspectRatio(), 1.f, 1000.f);
}

void InstancingAndCullingApp::UpdateScene(float dt)
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

	if (GetAsyncKeyState('C') & 0x8000)
		m_IsFrustumCullingEnabled = true;

	if (GetAsyncKeyState('N') & 0x8000)
		m_IsFrustumCullingEnabled = false;

	//
	// Perform frustum culling.
	//
	m_Camera.UpdateViewMatrix();
	m_VisibleObjectCount = 0;

	D3D11_MAPPED_SUBRESOURCE mappedData;
	d3d_context_->Map(m_InstancedBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData);

	InstanceData* data = (InstanceData*)mappedData.pData;

	if (m_IsFrustumCullingEnabled)
	{
		for (int i = 0; i < m_InstancedData.size(); ++i)
		{
			XMMATRIX W = XMLoadFloat4x4(&m_InstancedData[i].World);
			m_Camera.CalLocalFrustum(W);
			if (m_Camera.GetFrustum().IsIntersected(m_SkullBox))
			{
				data[m_VisibleObjectCount++] = m_InstancedData[i];
			}
		}
	}
	else  // No culling enabled, draw all objects.
	{
		for (int i = 0; i < m_InstancedData.size(); ++i)
		{

			data[m_VisibleObjectCount++] = m_InstancedData[i];
		}
	}

	d3d_context_->Unmap(m_InstancedBuffer, 0);

	std::wostringstream outs;
	outs.precision(6);
	outs << L"Instancing and Culling Demo" <<
		L"    " << m_VisibleObjectCount <<
		L" objects visible out of " << m_InstancedData.size();
	main_wnd_caption_ = outs.str();
}

void InstancingAndCullingApp::DrawScene()
{
	d3d_context_->ClearRenderTargetView(render_target_view_, (const float*)&Colors::Silver);
	d3d_context_->ClearDepthStencilView(depth_stencil_view_, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

	d3d_context_->IASetInputLayout(InputLayouts::InstancedBasic32);
	d3d_context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	XMMATRIX viewProj = m_Camera.ViewProj();

	UINT stride[2] = { sizeof(Vertex::Basic32), sizeof(InstanceData) };
	UINT offset[2] = { 0, 0 };
	ID3D11Buffer* vbs[2] = { m_SkullVB, m_InstancedBuffer };

	// Set per frame constants.
	Effects::InstancedBasicFX->SetDirLights(m_DirLights);
	Effects::InstancedBasicFX->SetEyePosW(m_Camera.GetPosition());

	ID3DX11EffectTechnique* tech = Effects::InstancedBasicFX->Light3Tech;

	D3DX11_TECHNIQUE_DESC techDesc;

	tech->GetDesc(&techDesc);
	for (int p = 0; p < techDesc.Passes; ++p)
	{
		d3d_context_->IASetVertexBuffers(0, 2, vbs, stride, offset);
		d3d_context_->IASetIndexBuffer(m_SkullIB, DXGI_FORMAT_R32_UINT, 0);

		XMMATRIX world = XMLoadFloat4x4(&m_InstancedData[0].World);
		XMMATRIX invWorld = MathHelper::InverseTranspose(world);

		Effects::InstancedBasicFX->SetViewProj(viewProj);
		Effects::InstancedBasicFX->SetTexTransform(XMMatrixIdentity());
		Effects::InstancedBasicFX->SetMaterial(m_SkullMat);

		tech->GetPassByIndex(p)->Apply(0, d3d_context_);
		d3d_context_->DrawIndexedInstanced(m_SkullIndexCount, m_VisibleObjectCount, 0, 0, 0);
	}

	// Draw Bounding Box
	DrawLocalAABB(m_SkullBox);

	HR(swap_chain_->Present(0, 0));
}

void InstancingAndCullingApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	m_LastMousePos.x = x;
	m_LastMousePos.y = y;

	SetCapture(main_wnd_);
}

void InstancingAndCullingApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void InstancingAndCullingApp::OnMouseMove(WPARAM btnState, int x, int y)
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

void InstancingAndCullingApp::BuildSkullGeometryBuffers()
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

	XMFLOAT3 vMinf3(+MathHelper::Infinity, +MathHelper::Infinity, +MathHelper::Infinity);
	XMFLOAT3 vMaxf3(-MathHelper::Infinity, -MathHelper::Infinity, -MathHelper::Infinity);

	XMVECTOR vMin = XMLoadFloat3(&vMinf3);
	XMVECTOR vMax = XMLoadFloat3(&vMaxf3);
	std::vector<Vertex::Basic32> vertices(vcount);
	for (UINT i = 0; i < vcount; ++i)
	{
		fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
		fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;

		XMVECTOR P = XMLoadFloat3(&vertices[i].Pos);

		vMin = XMVectorMin(vMin, P);
		vMax = XMVectorMax(vMax, P);
	}

	XMStoreFloat3(&m_SkullBox.center, 0.5f*(vMin + vMax));
	XMStoreFloat3(&m_SkullBox.extent, 0.5f*(vMax - vMin));

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

	//
	// Create Local Box Buffer
	//

	GeometryGenerator::MeshData box;

	GeometryGenerator geoGen;
	geoGen.CreateBox(m_SkullBox.center, m_SkullBox.extent.x * 2.f, m_SkullBox.extent.y * 2.f, m_SkullBox.extent.z * 2.f, box);

	std::vector<Vertex::Basic32> v(box.Vertices.size());
	for (UINT i = 0; i < v.size(); ++i)
	{
		v[i].Pos = box.Vertices[i].Position;
		v[i].Normal = box.Vertices[i].Normal;
		v[i].Tex = box.Vertices[i].TexC;
	}

	D3D11_BUFFER_DESC bvbd;
	bvbd.Usage = D3D11_USAGE_IMMUTABLE;
	bvbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bvbd.ByteWidth = sizeof(Vertex::Basic32) * v.size();
	bvbd.CPUAccessFlags = 0;
	bvbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vData;
	vData.pSysMem = &v[0];
	HR(d3d_device_->CreateBuffer(&bvbd, &vData, &m_BoxVB));

	//
	// Pack the indices of all the meshes into one index buffer.
	//

	D3D11_BUFFER_DESC bibd;
	bibd.Usage = D3D11_USAGE_IMMUTABLE;
	bibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bibd.ByteWidth = sizeof(UINT) * box.Indices.size();
	bibd.CPUAccessFlags = 0;
	bibd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA iData;
	iData.pSysMem = &box.Indices[0];
	HR(d3d_device_->CreateBuffer(&bibd, &iData, &m_BoxIB));
}

void InstancingAndCullingApp::BuildInstancedBuffer()
{
	const int n = 5;
	m_InstancedData.resize(n*n*n);

	float width = 200.0f;
	float height = 200.0f;
	float depth = 200.0f;

	float x = -0.5f*width;
	float y = -0.5f*height;
	float z = -0.5f*depth;
	float dx = width / (n - 1);
	float dy = height / (n - 1);
	float dz = depth / (n - 1);
	for (int k = 0; k < n; ++k)
	{
		for (int i = 0; i < n; ++i)
		{
			for (int j = 0; j < n; ++j)
			{
				// Position instanced along a 3D grid.
				m_InstancedData[k*n*n + i*n + j].World = XMFLOAT4X4(
					1.0f, 0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f, 0.0f,
					x + j*dx, y + i*dy, z + k*dz, 1.0f);

				// Random color.
				m_InstancedData[k*n*n + i*n + j].Color.x = MathHelper::RandF(0.0f, 1.0f);
				m_InstancedData[k*n*n + i*n + j].Color.y = MathHelper::RandF(0.0f, 1.0f);
				m_InstancedData[k*n*n + i*n + j].Color.z = MathHelper::RandF(0.0f, 1.0f);
				m_InstancedData[k*n*n + i*n + j].Color.w = 1.0f;
			}
		}
	}

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_DYNAMIC;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.ByteWidth = sizeof(InstanceData) * m_InstancedData.size();
	vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;

	HR(d3d_device_->CreateBuffer(&vbd, nullptr, &m_InstancedBuffer));
}

void InstancingAndCullingApp::DrawLocalAABB(const Box& box)
{
	d3d_context_->IASetInputLayout(InputLayouts::Basic32);
	
	XMMATRIX viewProj = m_Camera.ViewProj();

	UINT stride = sizeof(Vertex::Basic32);
	UINT offset = 0;

	ID3DX11EffectTechnique* boxTech = Effects::BasicFX->Light0TexTech;

	D3DX11_TECHNIQUE_DESC techDesc;
	boxTech->GetDesc(&techDesc);
	for (int i = 0; i < m_InstancedData.size(); ++i)
	{
		for (int p = 0; p < techDesc.Passes; ++p)
		{
			d3d_context_->IASetVertexBuffers(0, 1, &m_BoxVB, &stride, &offset);
			d3d_context_->IASetIndexBuffer(m_BoxIB, DXGI_FORMAT_R32_UINT, 0);

			// Set per object constants.
			auto world = XMLoadFloat4x4(&m_InstancedData[i].World);
			auto worldInvTranspose = MathHelper::InverseTranspose(world);
			auto wvp = world * viewProj;

			Effects::BasicFX->SetWorld(world);
			Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
			Effects::BasicFX->SetWorldViewProj(wvp);
			Effects::BasicFX->SetTexTransform(XMMatrixIdentity());

			d3d_context_->RSSetState(RenderStates::WireframeRS);
			boxTech->GetPassByIndex(p)->Apply(0, d3d_context_);
			d3d_context_->DrawIndexed(36, 0, 0);

			d3d_context_->RSSetState(nullptr);
		}
	}
	
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	InstancingAndCullingApp theApp(hInstance);

	if (!theApp.Init())
	{
		return 0;
	}

	return theApp.Run();
}