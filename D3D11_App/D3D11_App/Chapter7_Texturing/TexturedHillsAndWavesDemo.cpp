#include "d3dApp.h"
#include "GeometryGenerator.h"
#include "MathHelper.h"
#include "LightHelper.h"
#include "DDSTextureLoader.h"
#include "Waves.h"
#include "Vertex.h"
#include "Effects.h"


class TexturedHillsAndWavesApp : public D3DApp
{
public:
	TexturedHillsAndWavesApp(HINSTANCE hInstance);
	~TexturedHillsAndWavesApp();

	virtual bool Init() override;
	virtual void OnResize() override;
	virtual void UpdateScene(float dt) override;
	virtual void DrawScene() override;

	virtual void OnMouseDown(WPARAM btn_state, int x, int y) override;
	virtual void OnMouseUp(WPARAM btn_state, int x, int y) override;
	virtual void OnMouseMove(WPARAM btn_state, int x, int y) override;

private:
	float GetLandHeight(float x, float z) const;
	XMFLOAT3 GetLandNormal(float x, float z) const;
	void BuildLandGeometryBuffers();
	void BuildWavesGeometryBuffers();

private:
	ID3D11Buffer* land_vertex_buffer_;
	ID3D11Buffer* land_index_buffer_;

	ID3D11Buffer* waves_vertex_buffer_;
	ID3D11Buffer* waves_index_buffer_;

	ID3D11ShaderResourceView* grass_map_SRV_;
	ID3D11ShaderResourceView* waves_map_SRV_;

	Waves waves_;

	DirectionalLight direct_lights_[3];
	Material land_material_;
	Material waves_material_;

	XMFLOAT4X4 grass_tex_transform_;
	XMFLOAT4X4 waves_tex_transform_;
	XMFLOAT4X4 land_world_;
	XMFLOAT4X4 waves_world_;

	XMFLOAT4X4 view_;
	XMFLOAT4X4 proj_;

	XMFLOAT3 eye_pos_w_;

	UINT land_index_count_;

	XMFLOAT2 waves_tex_offset_;

	float theta_;
	float phi_;
	float radius_;

	POINT last_mouse_pos_;
};

TexturedHillsAndWavesApp::TexturedHillsAndWavesApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
	, land_vertex_buffer_(nullptr)
	, land_index_buffer_(nullptr)
	, waves_vertex_buffer_(nullptr)
	, waves_index_buffer_(nullptr)
	, grass_map_SRV_(nullptr)
	, waves_map_SRV_(nullptr)
	, waves_()
	, eye_pos_w_(0.f, 0.f, 0.f)
	, land_index_count_(0)
	, waves_tex_offset_(0.f, 0.f)
	, theta_(1.3f * MathHelper::Pi)
	, phi_(0.4f * MathHelper::Pi)
	, radius_(80.0f)
{
	main_wnd_caption_ = L"TexturedHillsAndWaves Demo";

	last_mouse_pos_.x = 0;
	last_mouse_pos_.y = 0;

	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&land_world_, I);
	XMStoreFloat4x4(&waves_world_, I);
	XMStoreFloat4x4(&view_, I);
	XMStoreFloat4x4(&proj_, I);

	XMMATRIX grass_tex_scale = XMMatrixScaling(5.f, 5.f, 0.f);
	XMStoreFloat4x4(&grass_tex_transform_, grass_tex_scale);

	// Directional light.
	direct_lights_[0].Ambient = XMFLOAT4(.2f, .2f, .2f, 1.f);
	direct_lights_[0].Diffuse = XMFLOAT4(.5f, .5f, .5f, 1.f);
	direct_lights_[0].Specular = XMFLOAT4(.5f, .5f, .5f, 1.f);
	direct_lights_[0].Direction = XMFLOAT3(0.57735f, -0.57735f, 0.57735f);

	direct_lights_[1].Ambient = XMFLOAT4(0.f, 0.f, 0.f, 1.f);
	direct_lights_[1].Diffuse = XMFLOAT4(.2f, .2f, .2f, 1.f);
	direct_lights_[1].Specular = XMFLOAT4(.25f, .25f, .25f, 1.f);
	direct_lights_[1].Direction = XMFLOAT3(-0.57735f, -0.57735f, 0.57735f);

	direct_lights_[2].Ambient = XMFLOAT4(0.f, 0.f, 0.f, 1.f);
	direct_lights_[2].Diffuse = XMFLOAT4(.2f, .2f, .2f, 1.f);
	direct_lights_[2].Specular = XMFLOAT4(0.f, 0.f, 0.f, 1.f);
	direct_lights_[2].Direction = XMFLOAT3(0.f, -0.707f, -0.707f);

	land_material_.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	land_material_.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	land_material_.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);

	waves_material_.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	waves_material_.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	waves_material_.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 32.0f);
}

TexturedHillsAndWavesApp::~TexturedHillsAndWavesApp()
{
	ReleaseCOM(land_vertex_buffer_);
	ReleaseCOM(land_index_buffer_);
	ReleaseCOM(waves_vertex_buffer_);
	ReleaseCOM(waves_index_buffer_);
	ReleaseCOM(grass_map_SRV_);
	ReleaseCOM(waves_map_SRV_);

	Effects::DestroyAll();
	InputLayouts::DestroyAll();
}

bool TexturedHillsAndWavesApp::Init()
{
	if (!D3DApp::Init())
	{
		return false;
	}

	waves_.Init(160, 160, 1.f, .03f, 3.25f, .4f);
	
	Effects::InitAll(d3d_device_);
	InputLayouts::InitAll(d3d_device_);

	d3d_context_->IASetInputLayout(InputLayouts::Basic32);
	d3d_context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	ID3D11Resource* tex_res = nullptr;
	HR(DirectX::CreateDDSTextureFromFile(d3d_device_,
		L"Textures/grass.dds", &tex_res, &grass_map_SRV_));
	ReleaseCOM(tex_res);

	HR(DirectX::CreateDDSTextureFromFile(d3d_device_,
		L"Textures/water2.dds", &tex_res, &waves_map_SRV_));
	ReleaseCOM(tex_res);

	BuildLandGeometryBuffers();
	BuildWavesGeometryBuffers();

	return true;
}

void TexturedHillsAndWavesApp::BuildLandGeometryBuffers()
{
	GeometryGenerator::MeshData grid;
	GeometryGenerator geometry_generator;

	geometry_generator.CreateGrid(160.0f, 160.0f, 50, 50, grid);

	land_index_count_ = grid.Indices.size();

	// Extract the vertex elements we are interested and apply the height function to
	// each vertex.  In addition, color the vertices based on their height so we have
	// sandy looking beaches, grassy low hills, and snow mountain peaks.
	std::vector<Vertex::Basic32> v(grid.Vertices.size());

	for (int i = 0; i < grid.Vertices.size(); ++i)
	{
		XMFLOAT3 pos = grid.Vertices[i].Position;
		pos.y = GetLandHeight(pos.x, pos.z);

		v[i].Pos = pos;
		v[i].Normal = GetLandNormal(pos.x, pos.z);
		v[i].Tex = grid.Vertices[i].TexC;
	}

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex::Basic32) * grid.Vertices.size();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &v[0];

	HR(d3d_device_->CreateBuffer(&vbd, &vinitData, &land_vertex_buffer_));

	// Pack the indices of all the meshes into one index buffer.
	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * land_index_count_;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &grid.Indices[0];

	HR(d3d_device_->CreateBuffer(&ibd, &iinitData, &land_index_buffer_));
}

void TexturedHillsAndWavesApp::BuildWavesGeometryBuffers()
{
	// Create the vertex buffer.  Note that we allocate space only, as
	// we will be updating the data every time step of the simulation.

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_DYNAMIC;
	vbd.ByteWidth = sizeof(Vertex::Basic32) * waves_.VertexCount();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vbd.MiscFlags = 0;
	HR(d3d_device_->CreateBuffer(&vbd, nullptr, &waves_vertex_buffer_));

	// Create the index buffer.  The index buffer is fixed, so we only 
	// need to create and set once.

	std::vector<UINT> indices(3 * waves_.TriangleCount());
	UINT rows = waves_.RowCount();
	UINT cols = waves_.ColumnCount();
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

	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &indices[0];
	HR(d3d_device_->CreateBuffer(&ibd, &iinitData, &waves_index_buffer_));

}

void TexturedHillsAndWavesApp::OnResize()
{
	D3DApp::OnResize();

	XMMATRIX p = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&proj_, p);
}

void TexturedHillsAndWavesApp::UpdateScene(float dt)
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

	//
	// Every quarter second, generate a random wave.
	//
	static float t_base = 0.f;
	if ((timer_.TotalTime() - t_base) >= .25f)
	{
		t_base += .25f;

		UINT i = 5 + rand() % (waves_.RowCount() - 10);
		UINT j = 5 + rand() % (waves_.ColumnCount() - 10);

		float r = MathHelper::RandF(1.f, 2.f);

		waves_.Disturb(i, j, r);
	}

	waves_.Update(dt);

	//
	// Update the wave vertex buffer with the new solution.
	//
	D3D11_MAPPED_SUBRESOURCE mapData;
	HR(d3d_context_->Map(waves_vertex_buffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapData));

	Vertex::Basic32* vertex = (Vertex::Basic32*)mapData.pData;
	for (int i = 0; i < waves_.VertexCount(); ++i)
	{
		vertex[i].Pos = waves_[i];
		vertex[i].Normal = waves_.Normal(i);

		vertex[i].Tex.x = 0.5f + waves_[i].x / waves_.Width();
		vertex[i].Tex.y = 0.5f - waves_[i].z / waves_.Depth();
	}

	d3d_context_->Unmap(waves_vertex_buffer_, 0);

	//
	// Animate water texture coordinates.
	//

	// Tile water texture.
	XMMATRIX waves_scale = XMMatrixScaling(5.f, 5.f, 0.f);

	// Translate texture over time.
	waves_tex_offset_.x += .1f * dt;
	waves_tex_offset_.y += .05f * dt;
	XMMATRIX waves_offset = XMMatrixTranslation(waves_tex_offset_.x, waves_tex_offset_.y, 0.f);

	// Combine scale and translation.
	XMStoreFloat4x4(&waves_tex_transform_, waves_scale * waves_offset);
}

void TexturedHillsAndWavesApp::DrawScene()
{
	d3d_context_->ClearRenderTargetView(render_target_view_, (const float*)&Colors::LightSteelBlue);
	d3d_context_->ClearDepthStencilView(depth_stencil_view_, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	UINT stride = sizeof(Vertex::Basic32);
	UINT offset = 0;

	// Set constants
	XMMATRIX view = XMLoadFloat4x4(&view_);
	XMMATRIX proj = XMLoadFloat4x4(&proj_);
	XMMATRIX view_proj = view * proj;

	// Set per frame constants.
	Effects::BasicFX->SetDirLights(direct_lights_);
	Effects::BasicFX->SetEyePosW(eye_pos_w_);

	ID3DX11EffectTechnique* tech = Effects::BasicFX->Light3TexTech;

	D3DX11_TECHNIQUE_DESC tech_desc;
	tech->GetDesc(&tech_desc);
	for (int p = 0; p < tech_desc.Passes; ++p)
	{
		// Draw the land.
		d3d_context_->IASetVertexBuffers(0, 1, &land_vertex_buffer_, &stride, &offset);
		d3d_context_->IASetIndexBuffer(land_index_buffer_, DXGI_FORMAT_R32_UINT, 0);

		// Set per object constants.
		XMMATRIX world = XMLoadFloat4x4(&land_world_);
		XMMATRIX worldInvTanspose = MathHelper::InverseTranspose(world);
		XMMATRIX wvp = world * view * proj;
		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTanspose);
		Effects::BasicFX->SetWorldViewProj(wvp);
		Effects::BasicFX->SetTexTransform(XMLoadFloat4x4(&grass_tex_transform_));
		Effects::BasicFX->SetMaterial(land_material_);
		Effects::BasicFX->SetDiffuseMap(grass_map_SRV_);

		tech->GetPassByIndex(p)->Apply(0, d3d_context_);
		d3d_context_->DrawIndexed(land_index_count_, 0, 0);

		// Draw the waves.
		d3d_context_->IASetVertexBuffers(0, 1, &waves_vertex_buffer_, &stride, &offset);
		d3d_context_->IASetIndexBuffer(waves_index_buffer_, DXGI_FORMAT_R32_UINT, 0);

		// Set per object constants.
		world = XMLoadFloat4x4(&waves_world_);
		worldInvTanspose = MathHelper::InverseTranspose(world);
		wvp = world * view * proj;
		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTanspose);
		Effects::BasicFX->SetWorldViewProj(wvp);
		Effects::BasicFX->SetTexTransform(XMLoadFloat4x4(&waves_tex_transform_));
		Effects::BasicFX->SetMaterial(waves_material_);
		Effects::BasicFX->SetDiffuseMap(waves_map_SRV_);

		tech->GetPassByIndex(p)->Apply(0, d3d_context_);
		d3d_context_->DrawIndexed(3 * waves_.TriangleCount(), 0, 0);
	}

	HR(swap_chain_->Present(0, 0));
}

void TexturedHillsAndWavesApp::OnMouseDown(WPARAM btn_state, int x, int y)
{
	last_mouse_pos_.x = x;
	last_mouse_pos_.y = y;

	SetCapture(main_wnd_);
}

void TexturedHillsAndWavesApp::OnMouseUp(WPARAM btn_state, int x, int y)
{
	ReleaseCapture();
}

void TexturedHillsAndWavesApp::OnMouseMove(WPARAM btn_state, int x, int y)
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
		float dx = 0.05f * (float)(x - last_mouse_pos_.x);
		float dy = 0.05f * (float)(y - last_mouse_pos_.y);

		// Update the camera radius based on input.
		radius_ += dx - dy;

		// Restrict the radius.
		radius_ = MathHelper::Clamp(radius_, 50.0f, 500.0f);
	}

	last_mouse_pos_.x = x;
	last_mouse_pos_.y = y;
}

float TexturedHillsAndWavesApp::GetLandHeight(float x, float z) const
{
	return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
}

XMFLOAT3 TexturedHillsAndWavesApp::GetLandNormal(float x, float z) const
{
	// n = (-df/dx, 1, -df/dz)
	XMFLOAT3 n(
		-0.03f*z*cosf(0.1f*x) - 0.3f*cosf(0.1f*z),
		1.0f,
		-0.3f*sinf(0.1f*x) + 0.03f*x*sinf(0.1f*z));

	XMVECTOR unitNor = XMVector3Normalize(XMLoadFloat3(&n));
	XMStoreFloat3(&n, unitNor);

	return n;
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	TexturedHillsAndWavesApp theApp(hInstance);

	if (!theApp.Init())
	{
		return 0;
	}

	return theApp.Run();
}