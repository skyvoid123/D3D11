#include "d3dApp.h"
#include "GeometryGenerator.h"
#include "MathHelper.h"
#include "LightHelper.h"
#include "Waves.h"

struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT3 Normal;
};

class LightingApp : public D3DApp
{
public:
	LightingApp(HINSTANCE hInstance);
	~LightingApp();

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
	void BuildFX();
	void BuildVertexLayout();

private:
	ID3D11Buffer* land_vertex_buffer_;
	ID3D11Buffer* land_index_buffer_;

	ID3D11Buffer* waves_vertex_buffer_;
	ID3D11Buffer* waves_index_buffer_;

	ID3DX11Effect* fx_;
	ID3DX11EffectTechnique* technique_;
	ID3DX11EffectMatrixVariable* fx_WVP_;
	ID3DX11EffectMatrixVariable* fx_world_;
	ID3DX11EffectMatrixVariable* fx_world_inv_transpose_;
	ID3DX11EffectVectorVariable* fx_eye_posW_;
	ID3DX11EffectVariable* fx_direct_light_;
	ID3DX11EffectVariable* fx_point_light_;
	ID3DX11EffectVariable* fx_spot_light_;
	ID3DX11EffectVariable* fx_material_;

	ID3D11InputLayout* input_layout_;

	ID3D11RasterizerState* wireframe_RS_;

	// Define transformations from local spaces to world space.
	XMFLOAT4X4 grid_world_;
	XMFLOAT4X4 waves_world_;
	XMFLOAT4X4 view_;
	XMFLOAT4X4 proj_;

	XMFLOAT3 eye_pos_w_;

	UINT grid_index_count_;

	Waves waves_;

	DirectionalLight direct_light_;
	PointLight point_light_;
	SpotLight spot_light_;
	Material land_material_;
	Material waves_material_;

	float theta_;
	float phi_;
	float radius_;

	POINT last_mouse_pos_;
};

LightingApp::LightingApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
	, land_vertex_buffer_(nullptr)
	, land_index_buffer_(nullptr)
	, waves_vertex_buffer_(nullptr)
	, waves_index_buffer_(nullptr)
	, fx_(nullptr)
	, technique_(nullptr)
	, fx_WVP_(nullptr)
	, fx_world_(nullptr)
	, fx_world_inv_transpose_(nullptr)
	, fx_eye_posW_(nullptr)
	, fx_direct_light_(nullptr)
	, fx_point_light_(nullptr)
	, fx_spot_light_(nullptr)
	, fx_material_(nullptr)
	, input_layout_(nullptr)
	, wireframe_RS_(nullptr)
	, eye_pos_w_(0.f, 0.f, 0.f)
	, grid_index_count_(0)
	, waves_()
	, theta_(1.5f * MathHelper::Pi)
	, phi_(0.1f * MathHelper::Pi)
	, radius_(200.0f)
{
	main_wnd_caption_ = L"Lighting Demo";

	last_mouse_pos_.x = 0;
	last_mouse_pos_.y = 0;

	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&grid_world_, I);
	XMStoreFloat4x4(&waves_world_, I);
	XMStoreFloat4x4(&view_, I);
	XMStoreFloat4x4(&proj_, I);

	XMMATRIX waves_offset = XMMatrixTranslation(0.f, -3.f, 0.f);
	XMStoreFloat4x4(&waves_world_, waves_offset);

	// Directional light.
	direct_light_.Ambient = XMFLOAT4(.2f, .2f, .2f, 1.f);
	direct_light_.Diffuse = XMFLOAT4(.5f, .5f, .5f, 1.f);
	direct_light_.Specular = XMFLOAT4(.5f, .5f, .5f, 1.f);
	direct_light_.Direction = XMFLOAT3(0.57735f, -0.57735f, 0.57735f);

	// Point light--position is changed every frame to animate in UpdateScene function.
	point_light_.Ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	point_light_.Diffuse = XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f);
	point_light_.Specular = XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f);
	point_light_.Att = XMFLOAT3(0.0f, 0.1f, 0.0f);
	point_light_.Range = 25.0f;

	// Spot light--position and direction changed every frame to animate in UpdateScene function.
	spot_light_.Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	spot_light_.Diffuse = XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f);
	spot_light_.Specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	spot_light_.Att = XMFLOAT3(1.0f, 0.0f, 0.0f);
	spot_light_.Spot = 96.0f;
	spot_light_.Range = 10000.0f;

	land_material_.Ambient = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
	land_material_.Diffuse = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
	land_material_.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);

	waves_material_.Ambient = XMFLOAT4(0.137f, 0.42f, 0.556f, 1.0f);
	waves_material_.Diffuse = XMFLOAT4(0.137f, 0.42f, 0.556f, 1.0f);
	waves_material_.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 96.0f);
}

LightingApp::~LightingApp()
{
	ReleaseCOM(land_vertex_buffer_);
	ReleaseCOM(land_index_buffer_);
	ReleaseCOM(waves_vertex_buffer_);
	ReleaseCOM(waves_index_buffer_);
	ReleaseCOM(fx_);
	ReleaseCOM(input_layout_);
	ReleaseCOM(wireframe_RS_);
}

bool LightingApp::Init()
{
	if (!D3DApp::Init())
	{
		return false;
	}

	waves_.Init(160, 160, 1.f, .03f, 3.25f, .4f);

	BuildLandGeometryBuffers();
	BuildWavesGeometryBuffers();
	BuildFX();
	BuildVertexLayout();

	D3D11_RASTERIZER_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	//desc.FillMode = D3D11_FILL_WIREFRAME;
	desc.FillMode = D3D11_FILL_SOLID;
	desc.CullMode = D3D11_CULL_BACK;
	desc.FrontCounterClockwise = false;
	desc.DepthClipEnable = true;

	HR(d3d_device_->CreateRasterizerState(&desc, &wireframe_RS_));

	return true;
}

void LightingApp::BuildLandGeometryBuffers()
{
	GeometryGenerator::MeshData grid;
	GeometryGenerator geometry_generator;

	geometry_generator.CreateGrid(160.0f, 160.0f, 50, 50, grid);

	grid_index_count_ = grid.Indices.size();

	// Extract the vertex elements we are interested and apply the height function to
	// each vertex.  In addition, color the vertices based on their height so we have
	// sandy looking beaches, grassy low hills, and snow mountain peaks.
	std::vector<Vertex> v(grid.Vertices.size());

	for (int i = 0; i < grid.Vertices.size(); ++i)
	{
		XMFLOAT3 pos = grid.Vertices[i].Position;
		pos.y = GetLandHeight(pos.x, pos.z);
		
		v[i].Pos = pos;
		v[i].Normal = GetLandNormal(pos.x, pos.z);
	}

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex) * grid.Vertices.size();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &v[0];

	HR(d3d_device_->CreateBuffer(&vbd, &vinitData, &land_vertex_buffer_));

	// Pack the indices of all the meshes into one index buffer.
	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * grid_index_count_;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &grid.Indices[0];

	HR(d3d_device_->CreateBuffer(&ibd, &iinitData, &land_index_buffer_));
}

void LightingApp::BuildWavesGeometryBuffers()
{
	// Create the vertex buffer.  Note that we allocate space only, as
	// we will be updating the data every time step of the simulation.

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_DYNAMIC;
	vbd.ByteWidth = sizeof(Vertex) * waves_.VertexCount();
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

void LightingApp::BuildFX()
{
	std::ifstream fin("fx/Lighting.fxo", std::ios::binary);

	fin.seekg(0, std::ios_base::end);
	int size = (int)fin.tellg();
	fin.seekg(0, std::ios_base::beg);

	std::vector<char> compiled_shader(size);

	fin.read(&compiled_shader[0], size);
	fin.close();

	HR(D3DX11CreateEffectFromMemory(&compiled_shader[0], size,
		0, d3d_device_, &fx_));

	technique_ = fx_->GetTechniqueByName("LightTech");
	fx_WVP_ = fx_->GetVariableByName("gWorldViewProj")->AsMatrix();
	fx_world_ = fx_->GetVariableByName("gWorld")->AsMatrix();
	fx_world_inv_transpose_ = fx_->GetVariableByName("gWorldInvTranspose")->AsMatrix();
	fx_eye_posW_ = fx_->GetVariableByName("gEyePosW")->AsVector();
	fx_direct_light_ = fx_->GetVariableByName("gDirLight");
	fx_point_light_ = fx_->GetVariableByName("gPointLight");
	fx_spot_light_ = fx_->GetVariableByName("gSpotLight");
	fx_material_ = fx_->GetVariableByName("gMaterial");
}

void LightingApp::BuildVertexLayout()
{
	// Create the vertex input layout.
	D3D11_INPUT_ELEMENT_DESC vertex_desc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	// Create the input layout
	D3DX11_PASS_DESC pass_desc;
	technique_->GetPassByIndex(0)->GetDesc(&pass_desc);
	HR(d3d_device_->CreateInputLayout(vertex_desc, 2, pass_desc.pIAInputSignature,
		pass_desc.IAInputSignatureSize, &input_layout_));
}

void LightingApp::OnResize()
{
	D3DApp::OnResize();

	XMMATRIX p = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&proj_, p);
}

void LightingApp::UpdateScene(float dt)
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

	Vertex* vertex = (Vertex*)mapData.pData;
	for (int i = 0; i < waves_.VertexCount(); ++i)
	{
		vertex[i].Pos = waves_[i];
		vertex[i].Normal = waves_.Normal(i);
	}

	d3d_context_->Unmap(waves_vertex_buffer_, 0);

	// Animate the lights.

	// Circle point light over the land surface.
	point_light_.Position.x = 70 * cosf(.2f * timer_.TotalTime());
	point_light_.Position.z = 70 * sinf(.2f * timer_.TotalTime());
	point_light_.Position.y = MathHelper::Max(GetLandHeight(point_light_.Position.x, point_light_.Position.z), -3.f) + 10.f;

	// The spotlight takes on the camera position and is aimed in the
	// same direction the camera is looking.  In this way, it looks
	// like we are holding a flashlight.
	spot_light_.Position = eye_pos_w_;
	XMStoreFloat3(&spot_light_.Direction, XMVector3Normalize(target - pos));
}

void LightingApp::DrawScene()
{
	d3d_context_->ClearRenderTargetView(render_target_view_, (const float*)&Colors::LightSteelBlue);
	d3d_context_->ClearDepthStencilView(depth_stencil_view_, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	d3d_context_->IASetInputLayout(input_layout_);
	d3d_context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	// Set constants
	XMMATRIX view = XMLoadFloat4x4(&view_);
	XMMATRIX proj = XMLoadFloat4x4(&proj_);
	XMMATRIX view_proj = view * proj;

	// Set per frame constants.
	fx_direct_light_->SetRawValue(&direct_light_, 0, sizeof(direct_light_));
	fx_point_light_->SetRawValue(&point_light_, 0, sizeof(point_light_));
	fx_spot_light_->SetRawValue(&spot_light_, 0, sizeof(spot_light_));
	fx_eye_posW_->SetRawValue(&eye_pos_w_, 0, sizeof(eye_pos_w_));

	D3DX11_TECHNIQUE_DESC tech_desc;
	technique_->GetDesc(&tech_desc);
	for (int p = 0; p < tech_desc.Passes; ++p)
	{
		// Draw the land.
		d3d_context_->IASetVertexBuffers(0, 1, &land_vertex_buffer_, &stride, &offset);
		d3d_context_->IASetIndexBuffer(land_index_buffer_, DXGI_FORMAT_R32_UINT, 0);

		// Set per object constants.
		XMMATRIX world = XMLoadFloat4x4(&grid_world_);
		XMMATRIX worldInvTanspose = MathHelper::InverseTranspose(world);
		XMMATRIX wvp = world * view * proj;
		fx_world_->SetMatrix((float*)&world);
		fx_world_inv_transpose_->SetMatrix((float*)&worldInvTanspose);
		fx_WVP_->SetMatrix((float*)&wvp);
		fx_material_->SetRawValue(&land_material_, 0, sizeof(land_material_));

		technique_->GetPassByIndex(p)->Apply(0, d3d_context_);
		d3d_context_->DrawIndexed(grid_index_count_, 0, 0);

		// Draw the waves.
		d3d_context_->RSSetState(wireframe_RS_);

		d3d_context_->IASetVertexBuffers(0, 1, &waves_vertex_buffer_, &stride, &offset);
		d3d_context_->IASetIndexBuffer(waves_index_buffer_, DXGI_FORMAT_R32_UINT, 0);

		// Set per object constants.
		world = XMLoadFloat4x4(&waves_world_);
		worldInvTanspose = MathHelper::InverseTranspose(world);
		wvp = world * view * proj;
		fx_world_->SetMatrix((float*)&world);
		fx_world_inv_transpose_->SetMatrix((float*)&worldInvTanspose);
		fx_WVP_->SetMatrix((float*)&wvp);
		fx_material_->SetRawValue(&waves_material_, 0, sizeof(waves_material_));

		technique_->GetPassByIndex(p)->Apply(0, d3d_context_);
		d3d_context_->DrawIndexed(3 * waves_.TriangleCount(), 0, 0);

		// Restore default.
		d3d_context_->RSSetState(0);
	}

	HR(swap_chain_->Present(0, 0));
}

void LightingApp::OnMouseDown(WPARAM btn_state, int x, int y)
{
	last_mouse_pos_.x = x;
	last_mouse_pos_.y = y;

	SetCapture(main_wnd_);
}

void LightingApp::OnMouseUp(WPARAM btn_state, int x, int y)
{
	ReleaseCapture();
}

void LightingApp::OnMouseMove(WPARAM btn_state, int x, int y)
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
		float dx = 0.2f * (float)(x - last_mouse_pos_.x);
		float dy = 0.2f * (float)(y - last_mouse_pos_.y);

		// Update the camera radius based on input.
		radius_ += dx - dy;

		// Restrict the radius.
		radius_ = MathHelper::Clamp(radius_, 50.0f, 500.0f);
	}

	last_mouse_pos_.x = x;
	last_mouse_pos_.y = y;
}

float LightingApp::GetLandHeight(float x, float z) const
{
	return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
}

XMFLOAT3 LightingApp::GetLandNormal(float x, float z) const
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

	LightingApp theApp(hInstance);

	if (!theApp.Init())
	{
		return 0;
	}

	return theApp.Run();
}