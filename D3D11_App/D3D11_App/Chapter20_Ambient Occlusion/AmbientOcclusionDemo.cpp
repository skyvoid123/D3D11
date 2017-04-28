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
#include "Octree.h"
#include "Camera.h"

struct BoundingSphere
{
	BoundingSphere() : m_Center(0.0f, 0.0f, 0.0f), m_Radius(0.0f) {}
	XMFLOAT3 m_Center;
	float m_Radius;
};

class AmbientOcclusionApp : public D3DApp
{
public:
	AmbientOcclusionApp(HINSTANCE hInstance);
	~AmbientOcclusionApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

	void OnMouseDown(WPARAM btnState, int x, int y);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y);

private:
	void BuildVertexAmbientOcclusion(std::vector<Vertex::AmbientOcclusion>& vertices,
		const std::vector<UINT>& indices);
	void BuildSkullGeometryBuffers();

private:
	ID3D11Buffer* m_SkullVB;
	ID3D11Buffer* m_SkullIB;

	XMFLOAT4X4 m_SkullWorld;

	UINT m_SkullIndexCount;

	Camera m_Camera;

	POINT m_LastMousePos;
};

AmbientOcclusionApp::AmbientOcclusionApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
	, m_SkullVB(nullptr)
	, m_SkullIB(nullptr)
	, m_SkullIndexCount(0)

{
	main_wnd_caption_ = L"Ambient Occlusion";
	
	XMMATRIX skullScale = XMMatrixScaling(0.5f, 0.5f, 0.5f);
	XMMATRIX skullOffset = XMMatrixTranslation(0.0f, 1.0f, 0.0f);
	XMStoreFloat4x4(&m_SkullWorld, XMMatrixMultiply(skullScale, skullOffset));
}

AmbientOcclusionApp::~AmbientOcclusionApp()
{
	d3d_context_->ClearState();
	ReleaseCOM(m_SkullVB);
	ReleaseCOM(m_SkullIB);

	Effects::DestroyAll();
	InputLayouts::DestroyAll();
}

bool AmbientOcclusionApp::Init()
{
	if (!D3DApp::Init())
	{
		return false;
	}

	Effects::InitAll(d3d_device_);
	InputLayouts::InitAll(d3d_device_);

	BuildSkullGeometryBuffers();

	return true;
}

void AmbientOcclusionApp::OnResize()
{
	D3DApp::OnResize();

	m_Camera.SetLens(.25f * MathHelper::Pi, AspectRatio(), 1.f, 1000.f);
}

void AmbientOcclusionApp::UpdateScene(float dt)
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

void AmbientOcclusionApp::DrawScene()
{
	d3d_context_->ClearRenderTargetView(render_target_view_, (const float*)&Colors::Silver);
	d3d_context_->ClearDepthStencilView(depth_stencil_view_, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

	m_Camera.UpdateViewMatrix();
	XMMATRIX viewProj = m_Camera.ViewProj();

	UINT stride = sizeof(Vertex::AmbientOcclusion);
	UINT offset = 0;

	d3d_context_->IASetInputLayout(InputLayouts::AmbientOcclusion);
	d3d_context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	d3d_context_->IASetVertexBuffers(0, 1, &m_SkullVB, &stride, &offset);
	d3d_context_->IASetIndexBuffer(m_SkullIB, DXGI_FORMAT_R32_UINT, 0);

	ID3DX11EffectTechnique* tech = Effects::AmbientOcclusionFX->AmbientOcclusionTech;

	D3DX11_TECHNIQUE_DESC techDesc;
	tech->GetDesc(&techDesc);
	for (int p = 0; p < techDesc.Passes; ++p)
	{
		XMMATRIX world = XMLoadFloat4x4(&m_SkullWorld);
		XMMATRIX worldViewProj = world * viewProj;

		Effects::AmbientOcclusionFX->SetWorldViewProj(worldViewProj);

		tech->GetPassByIndex(p)->Apply(0, d3d_context_);
		d3d_context_->DrawIndexed(m_SkullIndexCount, 0, 0);
	}

	HR(swap_chain_->Present(0, 0));
}

void AmbientOcclusionApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	m_LastMousePos.x = x;
	m_LastMousePos.y = y;

	SetCapture(main_wnd_);
}

void AmbientOcclusionApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void AmbientOcclusionApp::OnMouseMove(WPARAM btnState, int x, int y)
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

void AmbientOcclusionApp::BuildVertexAmbientOcclusion(std::vector<Vertex::AmbientOcclusion>& vertices, const std::vector<UINT>& indices)
{
	int vcount = vertices.size();
	int tcount = indices.size() / 3;

	std::vector<XMFLOAT3> pos(vcount);
	for (int i = 0; i < vcount; ++i)
	{
		pos[i] = vertices[i].Pos;
	}

	Octree octree;
	octree.Build(pos, indices);

	// For each vertex, count how many triangles contain the vertex.
	std::vector<int> vertexSharedCount(vcount);
	for (int i = 0; i < tcount; ++i)
	{
		UINT i0 = indices[i * 3 + 0];
		UINT i1 = indices[i * 3 + 1];
		UINT i2 = indices[i * 3 + 2];

		XMVECTOR v0 = XMLoadFloat3(&vertices[i0].Pos);
		XMVECTOR v1 = XMLoadFloat3(&vertices[i1].Pos);
		XMVECTOR v2 = XMLoadFloat3(&vertices[i2].Pos);

		XMVECTOR edge0 = v1 - v0;
		XMVECTOR edge1 = v2 - v0;

		XMVECTOR normal = XMVector3Normalize(XMVector3Cross(edge0, edge1));

		XMVECTOR centroid = (v0 + v1 + v2) / 3.0f;

		// Offset to avoid self intersection.
		centroid += 0.001f * normal;

		const int numSampleRays = 32;
		float numUnoccluded = 0;
		for (int j = 0; j < numSampleRays; ++j)
		{
			XMVECTOR randomDir = MathHelper::RandHemisphereUnitVec3(normal);

			// TODO: Technically we should not count intersections that are far 
			// away as occluding the triangle, but this is OK for demo.
			if (!octree.RayOctreeIntersect(centroid, randomDir))
			{
				++numUnoccluded;
			}
		}

		float ambientAccess = numUnoccluded / numSampleRays;
		
		// Average with vertices that share this face.
		vertices[i0].AmbientAccess += ambientAccess;
		vertices[i1].AmbientAccess += ambientAccess;
		vertices[i2].AmbientAccess += ambientAccess;

		++vertexSharedCount[i0];
		++vertexSharedCount[i1];
		++vertexSharedCount[i2];
	}

	// Finish average by dividing by the number of samples we added.
	for (int i = 0; i < vcount; ++i)
	{
		vertices[i].AmbientAccess /= vertexSharedCount[i];
	}
}

void AmbientOcclusionApp::BuildSkullGeometryBuffers()
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

	std::vector<Vertex::AmbientOcclusion> vertices(vcount);
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

	BuildVertexAmbientOcclusion(vertices, indices);

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

	AmbientOcclusionApp theApp(hInstance);

	if (!theApp.Init())
	{
		return 0;
	}

	return theApp.Run();
}