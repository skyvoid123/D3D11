#include "d3dApp.h"
#include "d3dx11effect.h"
#include "GeometryGenerator.h"
#include "MathHelper.h"

struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};

class SkullApp : public D3DApp
{
public:
	SkullApp(HINSTANCE hInstance);
	~SkullApp();

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
	XMFLOAT4X4 world_;
	XMFLOAT4X4 view_;
	XMFLOAT4X4 proj_;

	UINT index_count_;

	float theta_;
	float phi_;
	float radius_;

	POINT last_mouse_pos_;
};

SkullApp::SkullApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
	, vertex_buffer_(nullptr)
	, index_buffer_(nullptr)
	, fx_(nullptr)
	, technique_(nullptr)
	, fx_WVP_(nullptr)
	, input_layout_(nullptr)
	, wireframe_RS_(nullptr)
	, index_count_(0)
	, theta_(1.5f * MathHelper::Pi)
	, phi_(0.1f * MathHelper::Pi)
	, radius_(20.0f)
{
	main_wnd_caption_ = L"Skull Demo";

	last_mouse_pos_.x = 0;
	last_mouse_pos_.y = 0;

	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&view_, I);
	XMStoreFloat4x4(&proj_, I);

	XMMATRIX T = XMMatrixTranslation(0.f, -2.f, 0.f);
	XMStoreFloat4x4(&world_, T);
}

SkullApp::~SkullApp()
{
	ReleaseCOM(vertex_buffer_);
	ReleaseCOM(index_buffer_);
	ReleaseCOM(fx_);
	ReleaseCOM(input_layout_);
	ReleaseCOM(wireframe_RS_);
}

bool SkullApp::Init()
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

void SkullApp::BuildGeometryBuffers()
{
	std::ifstream fin("Models/skull.txt");
	if (!fin)
	{
		MessageBox(0, L"Models/skull.txt not found.", 0, 0);
		return;
	}

	UINT vertex_count = 0;
	UINT trianle_count = 0;
	std::string ignore;

	fin >> ignore >> vertex_count;
	fin >> ignore >> trianle_count;
	fin >> ignore >> ignore >> ignore >> ignore;

	float nx, ny, nz;
	XMFLOAT4 black(0.f, 0.f, 0.f, 1.f);

	std::vector<Vertex> vertices(vertex_count);
	for (UINT i = 0; i < vertex_count; ++i)
	{
		auto& pos = vertices[i].Pos;
		fin >> pos.x >> pos.y >> pos.z;

		vertices[i].Color = black;

		// Normal not used in this demo.
		fin >> nx >> ny >> nz;

	}

	fin >> ignore >> ignore >> ignore;
	index_count_ = 3 * trianle_count;
	std::vector<UINT> indices(index_count_);
	for (UINT i = 0; i < trianle_count; ++i)
	{
		fin >> indices[3*i] >> indices[3*i + 1] >> indices[3*i + 2];
	}

	fin.close();

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex) * vertices.size();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &vertices[0];

	HR(d3d_device_->CreateBuffer(&vbd, &vinitData, &vertex_buffer_));

	// Pack the indices of all the meshes into one index buffer.
	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * index_count_;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &indices[0];

	HR(d3d_device_->CreateBuffer(&ibd, &iinitData, &index_buffer_));
}

void SkullApp::BuildFX()
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

void SkullApp::BuildVertexLayout()
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

void SkullApp::OnResize()
{
	D3DApp::OnResize();

	XMMATRIX p = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&proj_, p);
}

void SkullApp::UpdateScene(float dt)
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

void SkullApp::DrawScene()
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
	XMMATRIX world = XMLoadFloat4x4(&world_);
	XMMATRIX view = XMLoadFloat4x4(&view_);
	XMMATRIX proj = XMLoadFloat4x4(&proj_);
	XMMATRIX wvp = world * view * proj;

	fx_WVP_->SetMatrix((float*)&wvp);

	D3DX11_TECHNIQUE_DESC tech_desc;
	technique_->GetDesc(&tech_desc);
	for (int p = 0; p < tech_desc.Passes; ++p)
	{
		technique_->GetPassByIndex(p)->Apply(0, d3d_context_);
		d3d_context_->DrawIndexed(index_count_, 0, 0);
	}

	HR(swap_chain_->Present(0, 0));
}

void SkullApp::OnMouseDown(WPARAM btn_state, int x, int y)
{
	last_mouse_pos_.x = x;
	last_mouse_pos_.y = y;

	SetCapture(main_wnd_);
}

void SkullApp::OnMouseUp(WPARAM btn_state, int x, int y)
{
	ReleaseCapture();
}

void SkullApp::OnMouseMove(WPARAM btn_state, int x, int y)
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
		// Make each pixel correspond to 0.05 unit in the scene.
		float dx = 0.05f * (float)(x - last_mouse_pos_.x);
		float dy = 0.05f * (float)(y - last_mouse_pos_.y);

		// Update the camera radius based on input.
		radius_ += dx - dy;

		// Restrict the radius.
		radius_ = MathHelper::Clamp(radius_, 5.0f, 50.0f);
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

	SkullApp theApp(hInstance);

	if (!theApp.Init())
	{
		return 0;
	}

	return theApp.Run();
}