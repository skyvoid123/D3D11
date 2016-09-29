#include "d3dApp.h"
#include "d3dx11effect.h"
#include "GeometryGenerator.h"
#include "MathHelper.h"

struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};

class ShapesApp : public D3DApp
{
public:
	ShapesApp(HINSTANCE hInstance);
	~ShapesApp();

	virtual bool Init() override;
	virtual void OnResize() override;
	virtual void UpdateScene(float dt) override;
	virtual void DrawScene() override;

	virtual void OnMouseDown(WPARAM btn_state, int x, int y) override;
	virtual void OnMouseUp(WPARAM btn_state, int x, int y) override;
	virtual void OnMouseMove(WPARAM btn_state, int x, int y) override;

private:
	void BuildGeometryBuffers();
	void BuildFX();
	void BuildVertexLayout();

private:
	ID3D11Buffer* vertex_buffer_;
	ID3D11Buffer* index_buffer_;

	ID3DX11Effect* fx_;
	ID3DX11EffectTechnique* technique_;
	ID3DX11EffectMatrixVariable* fx_WVP_;

	ID3D11InputLayout* input_layout_;

	ID3D11RasterizerState* wireframe_RS_;

	// Define transformations from local spaces to world space.
	XMFLOAT4X4 sphere_world_[10];
	XMFLOAT4X4 cylinder_world_[10];
	XMFLOAT4X4 box_world_;
	XMFLOAT4X4 grid_world_;
	XMFLOAT4X4 center_sphere_world_;

	XMFLOAT4X4 view_;
	XMFLOAT4X4 proj_;

	int box_vertex_offset_;
	int grid_vertex_offset_;
	int sphere_vertex_offset_;
	int cylinder_vertex_offset_;

	UINT box_index_offset_;
	UINT grid_index_offset_;
	UINT sphere_index_offset_;
	UINT cylinder_index_offset_;

	UINT box_index_count_;
	UINT grid_index_count_;
	UINT sphere_index_count_;
	UINT cylinder_index_count_;

	float theta_;
	float phi_;
	float radius_;

	POINT last_mouse_pos_;
};

ShapesApp::ShapesApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
	, vertex_buffer_(nullptr)
	, index_buffer_(nullptr)
	, fx_(nullptr)
	, technique_(nullptr)
	, fx_WVP_(nullptr)
	, input_layout_(nullptr)
	, wireframe_RS_(nullptr)
	, theta_(1.5f * MathHelper::Pi)
	, phi_(0.1f * MathHelper::Pi)
	, radius_(200.0f)
{
	main_wnd_caption_ = L"Shapes Demo";

	last_mouse_pos_.x = 0;
	last_mouse_pos_.y = 0;

	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&grid_world_, I);
	XMStoreFloat4x4(&view_, I);
	XMStoreFloat4x4(&proj_, I);

	XMMATRIX box_scale = XMMatrixScaling(2.f, 1.f, 2.f);
	XMMATRIX box_offset = XMMatrixTranslation(0.f, .5f, 0.f);
	XMStoreFloat4x4(&box_world_, XMMatrixMultiply(box_scale, box_offset));

	XMMATRIX center_sphere_scale = XMMatrixScaling(2.f, 2.f, 2.f);
	XMMATRIX center_sphere_offset = XMMatrixTranslation(0.f, 2.f, 0.f);
	XMStoreFloat4x4(&center_sphere_world_, XMMatrixMultiply(center_sphere_scale, center_sphere_offset));

	for (int i = 0; i < 5; ++i)
	{
		XMStoreFloat4x4(&cylinder_world_[i * 2], XMMatrixTranslation(-5.f, 1.5f, -10.f + i*5.f));
		XMStoreFloat4x4(&cylinder_world_[i * 2 + 1], XMMatrixTranslation(5.f, 1.5f, -10.f + i*5.f));

		XMStoreFloat4x4(&sphere_world_[i * 2], XMMatrixTranslation(-5.f, 3.5f, 10.f + i*5.f));
		XMStoreFloat4x4(&sphere_world_[i * 2 + 1], XMMatrixTranslation(5.f, 3.5f, 10.f + i*5.f));
	}
}

ShapesApp::~ShapesApp()
{
	ReleaseCOM(vertex_buffer_);
	ReleaseCOM(index_buffer_);
	ReleaseCOM(fx_);
	ReleaseCOM(input_layout_);
	ReleaseCOM(wireframe_RS_);
}

bool ShapesApp::Init()
{
	if (!D3DApp::Init())
	{
		return false;
	}
	BuildGeometryBuffers();
	BuildFX();
	BuildVertexLayout();

	D3D11_RASTERIZER_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.FillMode = D3D11_FILL_WIREFRAME;
	desc.CullMode = D3D11_CULL_BACK;
	desc.FrontCounterClockwise = false;
	desc.DepthClipEnable = true;

	HR(d3d_device_->CreateRasterizerState(&desc, &wireframe_RS_));

	return true;
}

void ShapesApp::BuildGeometryBuffers()
{
	GeometryGenerator::MeshData box;
	GeometryGenerator::MeshData grid;
	GeometryGenerator::MeshData sphere;
	GeometryGenerator::MeshData cylinder;

	GeometryGenerator generator;

	generator.CreateBox(1.f, 1.f, 1.f, box);
	generator.CreateGrid(20.f, 30.f, 60, 40, grid);
	generator.CreateSphere(.5f, 20, 20, sphere);
	generator.CreateCylinder(.5f, .3f, 3.f, 20, 20, cylinder);

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

	UINT total_vertex_count =
		box.Vertices.size() +
		grid.Vertices.size() +
		sphere.Vertices.size() +
		cylinder.Vertices.size();

	UINT total_index_count =
		box_index_count_ +
		grid_index_count_ +
		sphere_index_count_ +
		cylinder_index_count_;

	// Extract the vertex elements we are interested in and pack the
	// vertices of all the meshes into one vertex buffer.
	std::vector<Vertex> v(total_vertex_count);

	XMFLOAT4 black(0.f, 0.f, 0.f, 1.f);

	UINT k = 0;
	// box
	for (int i = 0; i < box.Vertices.size(); ++i, ++k)
	{
		v[k].Pos = box.Vertices[i].Position;
		v[k].Color = black;
	}
	// grid
	for (int i = 0; i < grid.Vertices.size(); ++i, ++k)
	{
		v[k].Pos = grid.Vertices[i].Position;
		v[k].Color = black;
	}
	// sphere
	for (int i = 0; i < sphere.Vertices.size(); ++i, ++k)
	{
		v[k].Pos = sphere.Vertices[i].Position;
		v[k].Color = black;
	}
	// cylinder
	for (int i = 0; i < cylinder.Vertices.size(); ++i, ++k)
	{
		v[k].Pos = cylinder.Vertices[i].Position;
		v[k].Color = black;
	}

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex) * total_vertex_count;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &v[0];

	HR(d3d_device_->CreateBuffer(&vbd, &vinitData, &vertex_buffer_));


	// Pack the indices of all the meshes into one index buffer.
	std::vector<UINT> indices;
	indices.insert(indices.end(), box.Indices.begin(), box.Indices.end());
	indices.insert(indices.end(), grid.Indices.begin(), grid.Indices.end());
	indices.insert(indices.end(), sphere.Indices.begin(), sphere.Indices.end());
	indices.insert(indices.end(), cylinder.Indices.begin(), cylinder.Indices.end());

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * total_index_count;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &indices[0];

	HR(d3d_device_->CreateBuffer(&ibd, &iinitData, &index_buffer_));
}

void ShapesApp::BuildFX()
{
	std::ifstream fin("fx/color.fxo", std::ios::binary);

	fin.seekg(0, std::ios_base::end);
	int size = (int)fin.tellg();
	fin.seekg(0, std::ios_base::beg);

	std::vector<char> compiled_shader(size);

	fin.read(&compiled_shader[0], size);
	fin.close();

	HR(D3DX11CreateEffectFromMemory(&compiled_shader[0], size,
		0, d3d_device_, &fx_));

	technique_ = fx_->GetTechniqueByName("ColorTech");
	fx_WVP_ = fx_->GetVariableByName("gWorldViewProj")->AsMatrix();
}

void ShapesApp::BuildVertexLayout()
{
	// Create the vertex input layout.
	D3D11_INPUT_ELEMENT_DESC vertex_desc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	// Create the input layout
	D3DX11_PASS_DESC pass_desc;
	technique_->GetPassByIndex(0)->GetDesc(&pass_desc);
	HR(d3d_device_->CreateInputLayout(vertex_desc, 2, pass_desc.pIAInputSignature,
		pass_desc.IAInputSignatureSize, &input_layout_));
}

void ShapesApp::OnResize()
{
	D3DApp::OnResize();

	XMMATRIX p = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&proj_, p);
}

void ShapesApp::UpdateScene(float dt)
{
	// Convert Spherical to Cartesian coordinates.
	float x = radius_*sinf(phi_)*cosf(theta_);
	float z = radius_*sinf(phi_)*sinf(theta_);
	float y = radius_*cosf(phi_);

	// Build the view matrix.
	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX v = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&view_, v);
}

void ShapesApp::DrawScene()
{
	d3d_context_->ClearRenderTargetView(render_target_view_, (const float*)&Colors::LightSteelBlue);
	d3d_context_->ClearDepthStencilView(depth_stencil_view_, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	d3d_context_->IASetInputLayout(input_layout_);
	d3d_context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	d3d_context_->RSSetState(wireframe_RS_);

	UINT stride = sizeof(Vertex);
	UINT off = 0;
	d3d_context_->IASetVertexBuffers(0, 1, &vertex_buffer_, &stride, &off);
	d3d_context_->IASetIndexBuffer(index_buffer_, DXGI_FORMAT_R32_UINT, 0);

	// Set constants
	XMMATRIX view = XMLoadFloat4x4(&view_);
	XMMATRIX proj = XMLoadFloat4x4(&proj_);
	XMMATRIX view_proj = view * proj;

	D3DX11_TECHNIQUE_DESC tech_desc;
	technique_->GetDesc(&tech_desc);
	for (int p = 0; p < tech_desc.Passes; ++p)
	{
		// draw grid
		XMMATRIX word = XMLoadFloat4x4(&grid_world_);
		fx_WVP_->SetMatrix((float*)&(word * view_proj));
		technique_->GetPassByIndex(p)->Apply(0, d3d_context_);
		d3d_context_->DrawIndexed(grid_index_count_, grid_index_offset_, grid_vertex_offset_);

		// draw box
		word = XMLoadFloat4x4(&box_world_);
		fx_WVP_->SetMatrix((float*)&(word * view_proj));
		technique_->GetPassByIndex(p)->Apply(0, d3d_context_);
		d3d_context_->DrawIndexed(box_index_count_, box_index_offset_, box_vertex_offset_);

		// draw center sphere
		word = XMLoadFloat4x4(&center_sphere_world_);
		fx_WVP_->SetMatrix((float*)&(word * view_proj));
		technique_->GetPassByIndex(p)->Apply(0, d3d_context_);
		d3d_context_->DrawIndexed(sphere_index_count_, sphere_index_offset_, sphere_vertex_offset_);
	
		// draw 10 cylinders
		for (int i = 0; i < 10; ++i)
		{
			word = XMLoadFloat4x4(&cylinder_world_[i]);
			fx_WVP_->SetMatrix((float*)&(word * view_proj));
			technique_->GetPassByIndex(p)->Apply(0, d3d_context_);
			d3d_context_->DrawIndexed(cylinder_index_count_, cylinder_index_offset_, cylinder_vertex_offset_);
		}

		// draw 10 sphere
		for (int i = 0; i < 10; ++i)
		{
			word = XMLoadFloat4x4(&sphere_world_[i]);
			fx_WVP_->SetMatrix((float*)&(word * view_proj));
			technique_->GetPassByIndex(p)->Apply(0, d3d_context_);
			d3d_context_->DrawIndexed(sphere_index_count_, sphere_index_offset_, sphere_vertex_offset_);
		}
	}

	HR(swap_chain_->Present(0, 0));
}

void ShapesApp::OnMouseDown(WPARAM btn_state, int x, int y)
{
	last_mouse_pos_.x = x;
	last_mouse_pos_.y = y;

	SetCapture(main_wnd_);
}

void ShapesApp::OnMouseUp(WPARAM btn_state, int x, int y)
{
	ReleaseCapture();
}

void ShapesApp::OnMouseMove(WPARAM btn_state, int x, int y)
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

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	ShapesApp theApp(hInstance);

	if (!theApp.Init())
	{
		return 0;
	}

	return theApp.Run();
}