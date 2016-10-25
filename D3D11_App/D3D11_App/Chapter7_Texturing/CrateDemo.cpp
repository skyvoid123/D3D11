#include "d3dApp.h"
#include "d3dx11effect.h"
#include "GeometryGenerator.h"
#include "MathHelper.h"
#include "LightHelper.h"
#include "DDSTextureLoader.h"
#include "Effects.h"
#include "Vertex.h"


class CrareApp : public D3DApp
{
public:
	CrareApp(HINSTANCE hInstance);
	~CrareApp();

	virtual bool Init() override;
	virtual void OnResize() override;
	virtual void UpdateScene(float dt) override;
	virtual void DrawScene() override;

	virtual void OnMouseDown(WPARAM btn_state, int x, int y) override;
	virtual void OnMouseUp(WPARAM btn_state, int x, int y) override;
	virtual void OnMouseMove(WPARAM btn_state, int x, int y) override;

private:
	void BuildGeometryBuffers();

private:
	ID3D11Buffer* box_vertex_buffer_;
	ID3D11Buffer* box_index_buffer_;

	ID3D11ShaderResourceView* diffuse_map_SRV_;

	DirectionalLight direct_lights_[3];
	Material box_mat_;

	XMFLOAT4X4 tex_transform_;
	XMFLOAT4X4 box_world_;

	XMFLOAT4X4 view_;
	XMFLOAT4X4 proj_;

	int box_vertex_offset_;
	UINT box_index_offset_;
	UINT box_index_count_;

	XMFLOAT3 eye_pos_w_;

	float theta_;
	float phi_;
	float radius_;

	POINT last_mouse_pos_;
};

CrareApp::CrareApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
	, box_vertex_buffer_(nullptr)
	, box_index_buffer_(nullptr)
	, diffuse_map_SRV_(nullptr)
	, eye_pos_w_(0.f, 0.f, 0.f)
	, theta_(1.3f * MathHelper::Pi)
	, phi_(0.4f * MathHelper::Pi)
	, radius_(2.5f)
{
	main_wnd_caption_ = L"Crate Demo";

	last_mouse_pos_.x = 0;
	last_mouse_pos_.y = 0;

	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&box_world_, I);
	XMStoreFloat4x4(&tex_transform_, I);
	XMStoreFloat4x4(&view_, I);
	XMStoreFloat4x4(&proj_, I);

	// Directional lights.
	direct_lights_[0].Ambient = XMFLOAT4(.3f, .3f, .3f, 1.f);
	direct_lights_[0].Diffuse = XMFLOAT4(.8f, .8f, .8f, 1.f);
	direct_lights_[0].Specular = XMFLOAT4(.6f, .6f, .6f, 16.f);
	direct_lights_[0].Direction = XMFLOAT3(0.707f, -0.707f, 0.f);

	direct_lights_[1].Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.f);
	direct_lights_[1].Diffuse = XMFLOAT4(1.4f, 1.4f, 1.4f, 1.f);
	direct_lights_[1].Specular = XMFLOAT4(0.3f, 0.3f, 0.3f, 16.f);
	direct_lights_[1].Direction = XMFLOAT3(-0.707f, 0.f, 0.707f);

	direct_lights_[2].Ambient = XMFLOAT4(0.f, 0.f, 0.f, 1.f);
	direct_lights_[2].Diffuse = XMFLOAT4(.2f, .2f, .2f, 1.f);
	direct_lights_[2].Specular = XMFLOAT4(0.f, 0.f, 0.f, 1.f);
	direct_lights_[2].Direction = XMFLOAT3(0.f, -0.707f, -0.707f);

	box_mat_.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	box_mat_.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	box_mat_.Specular = XMFLOAT4(0.6f, 0.6f, 0.6f, 16.0f);
}

CrareApp::~CrareApp()
{
	ReleaseCOM(box_vertex_buffer_);
	ReleaseCOM(box_index_buffer_);
	ReleaseCOM(diffuse_map_SRV_);

	Effects::DestroyAll();
	InputLayouts::DestroyAll();
}

bool CrareApp::Init()
{
	if (!D3DApp::Init())
	{
		return false;
	}

	// Must init Effects first since InputLayouts depend on shader signatures.
	Effects::InitAll(d3d_device_);
	InputLayouts::InitAll(d3d_device_);

	ID3D11Resource* tex_res = nullptr;
	HR(DirectX::CreateDDSTextureFromFile(d3d_device_,
		L"Textures/WoodCrate01.dds", &tex_res, &diffuse_map_SRV_));
	ReleaseCOM(tex_res);

	BuildGeometryBuffers();

	return true;
}

void CrareApp::BuildGeometryBuffers()
{
	GeometryGenerator::MeshData box;

	GeometryGenerator geoGen;
	geoGen.CreateBox(1.0f, 1.0f, 1.0f, box);

	// Cache the vertex offsets to each object in the concatenated vertex buffer.
	box_vertex_offset_ = 0;

	// Cache the index count of each object.
	box_index_count_ = box.Indices.size();

	// Cache the starting index for each object in the concatenated index buffer.
	box_index_offset_ = 0;

	UINT totalVertexCount = box.Vertices.size();

	UINT totalIndexCount = box_index_count_;

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

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex::Basic32) * totalVertexCount;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &vertices[0];
	HR(d3d_device_->CreateBuffer(&vbd, &vinitData, &box_vertex_buffer_));

	//
	// Pack the indices of all the meshes into one index buffer.
	//

	std::vector<UINT> indices;
	indices.insert(indices.end(), box.Indices.begin(), box.Indices.end());

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * totalIndexCount;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &indices[0];
	HR(d3d_device_->CreateBuffer(&ibd, &iinitData, &box_index_buffer_));
}

void CrareApp::OnResize()
{
	D3DApp::OnResize();

	XMMATRIX p = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&proj_, p);
}

void CrareApp::UpdateScene(float dt)
{
	// Convert Spherical to Cartesian coordinates.
	float x = radius_*sinf(phi_)*cosf(theta_);
	float z = radius_*sinf(phi_)*sinf(theta_);
	float y = radius_*cosf(phi_);

	eye_pos_w_ = XMFLOAT3(x, y, z);

	// Build the view matrix.
	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX v = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&view_, v);
}

void CrareApp::DrawScene()
{
	d3d_context_->ClearRenderTargetView(render_target_view_, (const float*)&Colors::LightSteelBlue);
	d3d_context_->ClearDepthStencilView(depth_stencil_view_, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	d3d_context_->IASetInputLayout(InputLayouts::Basic32);
	d3d_context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT stride = sizeof(Vertex::Basic32);
	UINT offset = 0;

	// Set constants
	XMMATRIX view = XMLoadFloat4x4(&view_);
	XMMATRIX proj = XMLoadFloat4x4(&proj_);
	XMMATRIX view_proj = view * proj;

	// Set per frame constants.
	Effects::BasicFX->SetDirLights(direct_lights_);
	Effects::BasicFX->SetEyePosW(eye_pos_w_);

	// Figure out which technique to use.
	ID3DX11EffectTechnique* activeTech = Effects::BasicFX->Light2TexTech;

	D3DX11_TECHNIQUE_DESC tech_desc;
	activeTech->GetDesc(&tech_desc);
	for (int p = 0; p < tech_desc.Passes; ++p)
	{
		d3d_context_->IASetVertexBuffers(0, 1, &box_vertex_buffer_, &stride, &offset);
		d3d_context_->IASetIndexBuffer(box_index_buffer_, DXGI_FORMAT_R32_UINT, 0);

		// Draw the box.
		XMMATRIX world = XMLoadFloat4x4(&box_world_);
		XMMATRIX worldInvTanspose = MathHelper::InverseTranspose(world);
		XMMATRIX wvp = world * view * proj;

		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTanspose);
		Effects::BasicFX->SetWorldViewProj(wvp);
		Effects::BasicFX->SetTexTransform(XMLoadFloat4x4(&tex_transform_));
		Effects::BasicFX->SetMaterial(box_mat_);
		Effects::BasicFX->SetDiffuseMap(diffuse_map_SRV_);

		activeTech->GetPassByIndex(p)->Apply(0, d3d_context_);
		d3d_context_->DrawIndexed(box_index_count_, box_index_offset_, box_vertex_offset_);
	}

	HR(swap_chain_->Present(0, 0));
}

void CrareApp::OnMouseDown(WPARAM btn_state, int x, int y)
{
	last_mouse_pos_.x = x;
	last_mouse_pos_.y = y;

	SetCapture(main_wnd_);
}

void CrareApp::OnMouseUp(WPARAM btn_state, int x, int y)
{
	ReleaseCapture();
}

void CrareApp::OnMouseMove(WPARAM btn_state, int x, int y)
{
	if ((btn_state & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f * (float)(x - last_mouse_pos_.x));
		float dy = XMConvertToRadians(0.25f * (float)(y - last_mouse_pos_.y));

		// Update angles based on input to orbit camera around box.
		theta_ -= dx;
		phi_ -= dy;

		// Restrict the angle mPhi.
		phi_ = MathHelper::Clamp(phi_, 0.1f, MathHelper::Pi - 0.1f);
	}
	else if ((btn_state & MK_RBUTTON) != 0)
	{
		// Make each pixel correspond to 0.2 unit in the scene.
		float dx = 0.01f * (float)(x - last_mouse_pos_.x);
		float dy = 0.01f * (float)(y - last_mouse_pos_.y);

		// Update the camera radius based on input.
		radius_ += dx - dy;

		// Restrict the radius.
		radius_ = MathHelper::Clamp(radius_, 1.0f, 15.0f);
	}

	last_mouse_pos_.x = x;
	last_mouse_pos_.y = y;
}


int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	CrareApp theApp(hInstance);

	if (!theApp.Init())
	{
		return 0;
	}

	return theApp.Run();
}