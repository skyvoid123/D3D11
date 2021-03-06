#include "d3dApp.h"
#include "d3dx11effect.h"
#include "GeometryGenerator.h"
#include "MathHelper.h"
#include "Waves.h"

struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};

class WavesApp : public D3DApp
{
public:
	WavesApp(HINSTANCE hInstance);
	~WavesApp();

	virtual bool Init() override;
	virtual void OnResize() override;
	virtual void UpdateScene(float dt) override;
	virtual void DrawScene() override;

	virtual void OnMouseDown(WPARAM btn_state, int x, int y) override;
	virtual void OnMouseUp(WPARAM btn_state, int x, int y) override;
	virtual void OnMouseMove(WPARAM btn_state, int x, int y) override;

private:
	float GetHeight(float x, float z) const;
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

	ID3D11InputLayout* input_layout_;

	ID3D11RasterizerState* wireframe_RS_;

	// Define transformations from local spaces to world space.
	XMFLOAT4X4 grid_world_;
	XMFLOAT4X4 waves_world_;
	XMFLOAT4X4 view_;
	XMFLOAT4X4 proj_;

	UINT grid_index_count_;

	Waves waves_;

	float theta_;
	float phi_;
	float radius_;

	POINT last_mouse_pos_;
};

WavesApp::WavesApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
	, land_vertex_buffer_(nullptr)
	, land_index_buffer_(nullptr)
	, waves_vertex_buffer_(nullptr)
	, waves_index_buffer_(nullptr)
	, fx_(nullptr)
	, technique_(nullptr)
	, fx_WVP_(nullptr)
	, input_layout_(nullptr)
	, wireframe_RS_(nullptr)
	, grid_index_count_(0)
	, waves_()
	, theta_(1.5f * MathHelper::Pi)
	, phi_(0.1f * MathHelper::Pi)
	, radius_(200.0f)
{
	main_wnd_caption_ = L"Waves Demo";

	last_mouse_pos_.x = 0;
	last_mouse_pos_.y = 0;

	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&grid_world_, I);
	XMStoreFloat4x4(&waves_world_, I);
	XMStoreFloat4x4(&view_, I);
	XMStoreFloat4x4(&proj_, I);
}

WavesApp::~WavesApp()
{
	ReleaseCOM(land_vertex_buffer_);
	ReleaseCOM(land_index_buffer_);
	ReleaseCOM(waves_vertex_buffer_);
	ReleaseCOM(waves_index_buffer_);
	ReleaseCOM(fx_);
	ReleaseCOM(input_layout_);
	ReleaseCOM(wireframe_RS_);
}

bool WavesApp::Init()
{
	if (!D3DApp::Init())
	{
		return false;
	}

	waves_.Init(200, 200, .8f, .03f, 3.25f, .4f);

	BuildLandGeometryBuffers();
	BuildWavesGeometryBuffers();
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

void WavesApp::BuildLandGeometryBuffers()
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
		pos.y = GetHeight(pos.x, pos.z);
		v[i].Pos = pos;

		// Color the vertex based on its height.
		if (pos.y < -10.f)
		{
			// Sandy beach color.
			v[i].Color = XMFLOAT4(1.f, 0.96f, 0.62f, 1.0f);
		}
		else if (pos.y < 5.f)
		{
			// Light yellow-green.
			v[i].Color = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
		}
		else if (pos.y < 12.f)
		{
			// Dark yellow-green.
			v[i].Color = XMFLOAT4(0.1f, 0.48f, 0.19f, 1.0f);
		}
		else if (pos.y < 20.f)
		{
			// Dark brown.
			v[i].Color = XMFLOAT4(0.45f, 0.39f, 0.34f, 1.0f);
		}
		else
		{
			// White snow.
			v[i].Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		}
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

void WavesApp::BuildWavesGeometryBuffers()
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

void WavesApp::BuildFX()
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

void WavesApp::BuildVertexLayout()
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

void WavesApp::OnResize()
{
	D3DApp::OnResize();

	XMMATRIX p = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&proj_, p);
}

void WavesApp::UpdateScene(float dt)
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

	//
	// Every quarter second, generate a random wave.
	//
	static float t_base = 0.f;
	if ((timer_.TotalTime() - t_base) >= .25f)
	{
		t_base += .25f;

		UINT i = 5 + rand() % 190;
		UINT j = 5 + rand() % 190;

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
		vertex[i].Color = XMFLOAT4(0.f, 0.f, 0.f, 1.f);
	}

	d3d_context_->Unmap(waves_vertex_buffer_, 0);
}

void WavesApp::DrawScene()
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

	D3DX11_TECHNIQUE_DESC tech_desc;
	technique_->GetDesc(&tech_desc);
	for (int p = 0; p < tech_desc.Passes; ++p)
	{
		// Draw the land.
		d3d_context_->IASetVertexBuffers(0, 1, &land_vertex_buffer_, &stride, &offset);
		d3d_context_->IASetIndexBuffer(land_index_buffer_, DXGI_FORMAT_R32_UINT, 0);

		XMMATRIX world = XMLoadFloat4x4(&grid_world_);
		XMMATRIX wvp = world * view * proj;
		fx_WVP_->SetMatrix((float*)&wvp);
		technique_->GetPassByIndex(p)->Apply(0, d3d_context_);
		d3d_context_->DrawIndexed(grid_index_count_, 0, 0);

		// Draw the waves.
		d3d_context_->RSSetState(wireframe_RS_);

		d3d_context_->IASetVertexBuffers(0, 1, &waves_vertex_buffer_, &stride, &offset);
		d3d_context_->IASetIndexBuffer(waves_index_buffer_, DXGI_FORMAT_R32_UINT, 0);

		world = XMLoadFloat4x4(&waves_world_);
		wvp = world * view * proj;
		fx_WVP_->SetMatrix((float*)&wvp);
		technique_->GetPassByIndex(p)->Apply(0, d3d_context_);
		d3d_context_->DrawIndexed(3 * waves_.TriangleCount(), 0, 0);

		// Restore default.
		d3d_context_->RSSetState(0);
	}

	HR(swap_chain_->Present(0, 0));
}

void WavesApp::OnMouseDown(WPARAM btn_state, int x, int y)
{
	last_mouse_pos_.x = x;
	last_mouse_pos_.y = y;

	SetCapture(main_wnd_);
}

void WavesApp::OnMouseUp(WPARAM btn_state, int x, int y)
{
	ReleaseCapture();
}

void WavesApp::OnMouseMove(WPARAM btn_state, int x, int y)
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

float WavesApp::GetHeight(float x, float z) const
{
	return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	WavesApp theApp(hInstance);

	if (!theApp.Init())
	{
		return 0;
	}

	return theApp.Run();
}