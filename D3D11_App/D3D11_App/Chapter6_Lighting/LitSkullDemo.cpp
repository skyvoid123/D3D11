#include "d3dApp.h"
#include "d3dx11effect.h"
#include "GeometryGenerator.h"
#include "MathHelper.h"
#include "LightHelper.h"
#include "Effects.h"
#include "Vertex.h"


class LitSkullApp : public D3DApp
{
public:
	LitSkullApp(HINSTANCE hInstance);
	~LitSkullApp();

	virtual bool Init() override;
	virtual void OnResize() override;
	virtual void UpdateScene(float dt) override;
	virtual void DrawScene() override;

	virtual void OnMouseDown(WPARAM btn_state, int x, int y) override;
	virtual void OnMouseUp(WPARAM btn_state, int x, int y) override;
	virtual void OnMouseMove(WPARAM btn_state, int x, int y) override;

private:
	void BuildShapeGeometryBuffers();
	void BuildSkullGeometryBuffers();

private:
	ID3D11Buffer* shapes_vertex_buffer_;
	ID3D11Buffer* shapes_index_buffer_;

	ID3D11Buffer* skull_vertex_buffer_;
	ID3D11Buffer* skull_index_buffer_;

	DirectionalLight direct_lights_[3];
	Material grid_mat_;
	Material box_mat_;
	Material cylinder_mat_;
	Material sphere_mat_;
	Material skull_mat_;

	// Define transformations from local spaces to world space.
	XMFLOAT4X4 grid_world_;
	XMFLOAT4X4 box_world_;
	XMFLOAT4X4 cylinder_world_[10];
	XMFLOAT4X4 sphere_world_[10];
	XMFLOAT4X4 skull_world_;

	XMFLOAT4X4 view_;
	XMFLOAT4X4 proj_;

	int grid_vertex_offset_;
	int box_vertex_offset_;
	int cylinder_vertex_offset_;
	int sphere_vertex_offset_;

	UINT grid_index_offset_;
	UINT box_index_offset_;
	UINT cylinder_index_offset_;
	UINT sphere_index_offset_;

	UINT grid_index_count_;
	UINT box_index_count_;
	UINT cylinder_index_count_;
	UINT sphere_index_count_;

	UINT skull_index_count_;

	UINT light_count_;

	XMFLOAT3 eye_pos_w_;

	float theta_;
	float phi_;
	float radius_;

	POINT last_mouse_pos_;
};

LitSkullApp::LitSkullApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
	, shapes_vertex_buffer_(nullptr)
	, shapes_index_buffer_(nullptr)
	, skull_vertex_buffer_(nullptr)
	, skull_index_buffer_(nullptr)
	, skull_index_count_(0)
	, light_count_(1)
	, eye_pos_w_(0.f, 0.f, 0.f)
	, theta_(1.5f * MathHelper::Pi)
	, phi_(0.1f * MathHelper::Pi)
	, radius_(15.0f)
{
	main_wnd_caption_ = L"LitSkull Demo";

	last_mouse_pos_.x = 0;
	last_mouse_pos_.y = 0;

	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&grid_world_, I);
	XMStoreFloat4x4(&view_, I);
	XMStoreFloat4x4(&proj_, I);

	XMMATRIX boxScale = XMMatrixScaling(3.0f, 1.0f, 3.0f);
	XMMATRIX boxOffset = XMMatrixTranslation(0.0f, 0.5f, 0.0f);
	XMStoreFloat4x4(&box_world_, XMMatrixMultiply(boxScale, boxOffset));

	XMMATRIX skullScale = XMMatrixScaling(0.5f, 0.5f, 0.5f);
	XMMATRIX skullOffset = XMMatrixTranslation(0.0f, 1.0f, 0.0f);
	XMStoreFloat4x4(&skull_world_, XMMatrixMultiply(skullScale, skullOffset));

	for (int i = 0; i < 5; ++i)
	{
		XMStoreFloat4x4(&cylinder_world_[i * 2 + 0], XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i*5.0f));
		XMStoreFloat4x4(&cylinder_world_[i * 2 + 1], XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i*5.0f));

		XMStoreFloat4x4(&sphere_world_[i * 2 + 0], XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i*5.0f));
		XMStoreFloat4x4(&sphere_world_[i * 2 + 1], XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i*5.0f));
	}

	// Directional lights.
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


	grid_mat_.Ambient = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
	grid_mat_.Diffuse = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
	grid_mat_.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);

	cylinder_mat_.Ambient = XMFLOAT4(0.7f, 0.85f, 0.7f, 1.0f);
	cylinder_mat_.Diffuse = XMFLOAT4(0.7f, 0.85f, 0.7f, 1.0f);
	cylinder_mat_.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);

	sphere_mat_.Ambient = XMFLOAT4(0.1f, 0.2f, 0.3f, 1.0f);
	sphere_mat_.Diffuse = XMFLOAT4(0.2f, 0.4f, 0.6f, 1.0f);
	sphere_mat_.Specular = XMFLOAT4(0.9f, 0.9f, 0.9f, 16.0f);

	box_mat_.Ambient = XMFLOAT4(0.651f, 0.5f, 0.392f, 1.0f);
	box_mat_.Diffuse = XMFLOAT4(0.651f, 0.5f, 0.392f, 1.0f);
	box_mat_.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);

	skull_mat_.Ambient = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	skull_mat_.Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	skull_mat_.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
}

LitSkullApp::~LitSkullApp()
{
	ReleaseCOM(shapes_vertex_buffer_);
	ReleaseCOM(shapes_index_buffer_);
	ReleaseCOM(skull_vertex_buffer_);
	ReleaseCOM(skull_index_buffer_);
	
	Effects::DestroyAll();
	InputLayouts::DestroyAll();
}

bool LitSkullApp::Init()
{
	if (!D3DApp::Init())
	{
		return false;
	}
	
	// Must init Effects first since InputLayouts depend on shader signatures.
	Effects::InitAll(d3d_device_);
	InputLayouts::InitAll(d3d_device_);

	BuildShapeGeometryBuffers();
	BuildSkullGeometryBuffers();

	return true;
}

void LitSkullApp::BuildShapeGeometryBuffers()
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
	box_vertex_offset_ = 0;
	grid_vertex_offset_ = box.Vertices.size();
	sphere_vertex_offset_ = grid_vertex_offset_ + grid.Vertices.size();
	cylinder_vertex_offset_ = sphere_vertex_offset_ + sphere.Vertices.size();

	// Cache the index count of each object.
	box_index_count_ = box.Indices.size();
	grid_index_count_ = grid.Indices.size();
	sphere_index_count_ = sphere.Indices.size();
	cylinder_index_count_ = cylinder.Indices.size();

	// Cache the starting index for each object in the concatenated index buffer.
	box_index_offset_ = 0;
	grid_index_offset_ = box_index_count_;
	sphere_index_offset_ = grid_index_offset_ + grid_index_count_;
	cylinder_index_offset_ = sphere_index_offset_ + sphere_index_count_;

	UINT totalVertexCount =
		box.Vertices.size() +
		grid.Vertices.size() +
		sphere.Vertices.size() +
		cylinder.Vertices.size();

	UINT totalIndexCount =
		box_index_count_ +
		grid_index_count_ +
		sphere_index_count_ +
		cylinder_index_count_;

	//
	// Extract the vertex elements we are interested in and pack the
	// vertices of all the meshes into one vertex buffer.
	//

	std::vector<Vertex::PosNormal> vertices(totalVertexCount);

	UINT k = 0;
	for (size_t i = 0; i < box.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = box.Vertices[i].Position;
		vertices[k].Normal = box.Vertices[i].Normal;
	}

	for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = grid.Vertices[i].Position;
		vertices[k].Normal = grid.Vertices[i].Normal;
	}

	for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = sphere.Vertices[i].Position;
		vertices[k].Normal = sphere.Vertices[i].Normal;
	}

	for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = cylinder.Vertices[i].Position;
		vertices[k].Normal = cylinder.Vertices[i].Normal;
	}

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex::PosNormal) * totalVertexCount;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &vertices[0];
	HR(d3d_device_->CreateBuffer(&vbd, &vinitData, &shapes_vertex_buffer_));

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
	HR(d3d_device_->CreateBuffer(&ibd, &iinitData, &shapes_index_buffer_));
}

void LitSkullApp::BuildSkullGeometryBuffers()
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

	std::vector<Vertex::PosNormal> vertices(vcount);
	for (UINT i = 0; i < vcount; ++i)
	{
		fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
		fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;
	}

	fin >> ignore;
	fin >> ignore;
	fin >> ignore;

	skull_index_count_ = 3 * tcount;
	std::vector<UINT> indices(skull_index_count_);
	for (UINT i = 0; i < tcount; ++i)
	{
		fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
	}

	fin.close();

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex::PosNormal) * vcount;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &vertices[0];
	HR(d3d_device_->CreateBuffer(&vbd, &vinitData, &skull_vertex_buffer_));

	//
	// Pack the indices of all the meshes into one index buffer.
	//

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * skull_index_count_;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &indices[0];
	HR(d3d_device_->CreateBuffer(&ibd, &iinitData, &skull_index_buffer_));
}


void LitSkullApp::OnResize()
{
	D3DApp::OnResize();

	XMMATRIX p = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&proj_, p);
}

void LitSkullApp::UpdateScene(float dt)
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
	// Switch the number of lights based on key presses.
	//
	if (GetAsyncKeyState('0') & 0x8000)
	{
		light_count_ = 0;
	}
	
	if (GetAsyncKeyState('1') & 0x8000)
	{
		light_count_ = 1;
	}

	if (GetAsyncKeyState('2') & 0x8000)
	{
		light_count_ = 2;
	}

	if (GetAsyncKeyState('3') & 0x8000)
	{
		light_count_ = 3;
	}

}

void LitSkullApp::DrawScene()
{
	d3d_context_->ClearRenderTargetView(render_target_view_, (const float*)&Colors::LightSteelBlue);
	d3d_context_->ClearDepthStencilView(depth_stencil_view_, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	d3d_context_->IASetInputLayout(InputLayouts::PosNormal);
	d3d_context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT stride = sizeof(Vertex::PosNormal);
	UINT offset = 0;

	// Set constants
	XMMATRIX view = XMLoadFloat4x4(&view_);
	XMMATRIX proj = XMLoadFloat4x4(&proj_);
	XMMATRIX view_proj = view * proj;

	// Set per frame constants.
	Effects::BasicFX->SetDirLights(direct_lights_);
	Effects::BasicFX->SetEyePosW(eye_pos_w_);

	// Figure out which technique to use.
	ID3DX11EffectTechnique* activeTech = Effects::BasicFX->Light1Tech;
	switch (light_count_)
	{
	case 1:
		activeTech = Effects::BasicFX->Light1Tech;
		break;
	case 2:
		activeTech = Effects::BasicFX->Light2Tech;
		break;
	case 3:
		activeTech = Effects::BasicFX->Light3Tech;
		break;
	default:
		break;
	}

	D3DX11_TECHNIQUE_DESC tech_desc;
	activeTech->GetDesc(&tech_desc);
	for (int p = 0; p < tech_desc.Passes; ++p)
	{
		d3d_context_->IASetVertexBuffers(0, 1, &shapes_vertex_buffer_, &stride, &offset);
		d3d_context_->IASetIndexBuffer(shapes_index_buffer_, DXGI_FORMAT_R32_UINT, 0);

		// Draw the grid
		XMMATRIX world = XMLoadFloat4x4(&grid_world_);
		XMMATRIX worldInvTanspose = MathHelper::InverseTranspose(world);
		XMMATRIX wvp = world * view * proj;

		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTanspose);
		Effects::BasicFX->SetWorldViewProj(wvp);
		Effects::BasicFX->SetMaterial(grid_mat_);

		activeTech->GetPassByIndex(p)->Apply(0, d3d_context_);
		d3d_context_->DrawIndexed(grid_index_count_, grid_index_offset_, grid_vertex_offset_);
		
		// Draw the box.
		world = XMLoadFloat4x4(&box_world_);
		worldInvTanspose = MathHelper::InverseTranspose(world);
		wvp = world * view * proj;

		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTanspose);
		Effects::BasicFX->SetWorldViewProj(wvp);
		Effects::BasicFX->SetMaterial(box_mat_);

		activeTech->GetPassByIndex(p)->Apply(0, d3d_context_);
		d3d_context_->DrawIndexed(box_index_count_, box_index_offset_, box_vertex_offset_);
	
		// Draw the cylinders.
		for (int i = 0; i < 10; ++i)
		{
			world = XMLoadFloat4x4(&cylinder_world_[i]);
			worldInvTanspose = MathHelper::InverseTranspose(world);
			wvp = world * view * proj;

			Effects::BasicFX->SetWorld(world);
			Effects::BasicFX->SetWorldInvTranspose(worldInvTanspose);
			Effects::BasicFX->SetWorldViewProj(wvp);
			Effects::BasicFX->SetMaterial(cylinder_mat_);

			activeTech->GetPassByIndex(p)->Apply(0, d3d_context_);
			d3d_context_->DrawIndexed(cylinder_index_count_, cylinder_index_offset_, cylinder_vertex_offset_);
		}

		// Draw the spheres.
		for (int i = 0; i < 10; ++i)
		{
			world = XMLoadFloat4x4(&sphere_world_[i]);
			worldInvTanspose = MathHelper::InverseTranspose(world);
			wvp = world * view * proj;

			Effects::BasicFX->SetWorld(world);
			Effects::BasicFX->SetWorldInvTranspose(worldInvTanspose);
			Effects::BasicFX->SetWorldViewProj(wvp);
			Effects::BasicFX->SetMaterial(sphere_mat_);

			activeTech->GetPassByIndex(p)->Apply(0, d3d_context_);
			d3d_context_->DrawIndexed(sphere_index_count_, sphere_index_offset_, sphere_vertex_offset_);
		}

		// Draw the skull.
		d3d_context_->IASetVertexBuffers(0, 1, &skull_vertex_buffer_, &stride, &offset);
		d3d_context_->IASetIndexBuffer(skull_index_buffer_, DXGI_FORMAT_R32_UINT, 0);

		world = XMLoadFloat4x4(&skull_world_);
		worldInvTanspose = MathHelper::InverseTranspose(world);
		wvp = world * view * proj;

		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTanspose);
		Effects::BasicFX->SetWorldViewProj(wvp);
		Effects::BasicFX->SetMaterial(skull_mat_);

		activeTech->GetPassByIndex(p)->Apply(0, d3d_context_);
		d3d_context_->DrawIndexed(skull_index_count_, 0, 0);

	}

	HR(swap_chain_->Present(0, 0));
}

void LitSkullApp::OnMouseDown(WPARAM btn_state, int x, int y)
{
	last_mouse_pos_.x = x;
	last_mouse_pos_.y = y;

	SetCapture(main_wnd_);
}

void LitSkullApp::OnMouseUp(WPARAM btn_state, int x, int y)
{
	ReleaseCapture();
}

void LitSkullApp::OnMouseMove(WPARAM btn_state, int x, int y)
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
		radius_ = MathHelper::Clamp(radius_, 3.0f, 200.0f);
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

	LitSkullApp theApp(hInstance);

	if (!theApp.Init())
	{
		return 0;
	}

	return theApp.Run();
}