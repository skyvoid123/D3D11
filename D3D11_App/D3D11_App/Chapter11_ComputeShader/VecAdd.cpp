#include "d3dApp.h"
#include "MathHelper.h"
#include "Effects.h"
#include <fstream>

struct Data
{
	XMFLOAT3 v1;
	XMFLOAT2 v2;
};

class VecAddApp : public D3DApp
{
public:
	VecAddApp(HINSTANCE hInstance);
	~VecAddApp();

	virtual bool Init() override;
	virtual void OnResize() override;
	virtual void UpdateScene(float dt) override;
	virtual void DrawScene() override;

	void DoComputation();
	
private:
	void BuildBuffersAndViews();

private:
	ID3D11Buffer* output_buffer_;
	ID3D11Buffer* output_debug_buffer_;

	ID3D11ShaderResourceView* inputA_SRV_;
	ID3D11ShaderResourceView* inputB_SRV_;
	ID3D11UnorderedAccessView* output_UAV_;
	
	UINT data_num_;
};

VecAddApp::VecAddApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
	, output_buffer_(nullptr)
	, output_debug_buffer_(nullptr)
	, inputA_SRV_(nullptr)
	, inputB_SRV_(nullptr)
	, output_UAV_(nullptr)
	, data_num_(32)
{
	main_wnd_caption_ = L"Compute Shader Vec Add Demo";
}

VecAddApp::~VecAddApp()
{
	ReleaseCOM(output_buffer_);
	ReleaseCOM(output_debug_buffer_);

	ReleaseCOM(inputA_SRV_);
	ReleaseCOM(inputB_SRV_);
	ReleaseCOM(output_UAV_);

	Effects::DestroyAll();
}

bool VecAddApp::Init()
{
	if (!D3DApp::Init())
	{
		return false;
	}

	Effects::InitAll(d3d_device_);

	BuildBuffersAndViews();

	return true;
}

void VecAddApp::OnResize()
{
	D3DApp::OnResize();
}

void VecAddApp::UpdateScene(float dt)
{

}

void VecAddApp::DrawScene()
{
	d3d_context_->ClearRenderTargetView(render_target_view_, (const float*)&Colors::LightSteelBlue);
	d3d_context_->ClearDepthStencilView(depth_stencil_view_, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	HR(swap_chain_->Present(0, 0));
}

void VecAddApp::BuildBuffersAndViews()
{
	std::vector<Data> dataA(data_num_);
	std::vector<Data> dataB(data_num_);
	for (int i = 0; i < data_num_; ++i)
	{
		dataA[i].v1 = XMFLOAT3(i, i, i);
		dataA[i].v2 = XMFLOAT2(i, 0);

		dataB[i].v1 = XMFLOAT3(-i, i, 0.f);
		dataB[i].v2 = XMFLOAT2(0.f, -i);
	}

	// Create a buffer to be bound as a shader input (D3D11_BIND_SHADER_RESOURCE).
	D3D11_BUFFER_DESC inputDesc;
	inputDesc.Usage = D3D11_USAGE_DEFAULT;
	inputDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	inputDesc.ByteWidth = sizeof(Data) * data_num_;
	inputDesc.CPUAccessFlags = 0;
	inputDesc.StructureByteStride = sizeof(Data);
	inputDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

	D3D11_SUBRESOURCE_DATA vDataA;
	vDataA.pSysMem = &dataA[0];
	ID3D11Buffer* bufferA = nullptr;
	HR(d3d_device_->CreateBuffer(&inputDesc, &vDataA, &bufferA));

	D3D11_SUBRESOURCE_DATA vDataB;
	vDataB.pSysMem = &dataB[0];
	ID3D11Buffer* bufferB = nullptr;
	HR(d3d_device_->CreateBuffer(&inputDesc, &vDataB, &bufferB));

	// Create a read-write buffer the compute shader can write to (D3D11_BIND_UNORDERED_ACCESS).
	D3D11_BUFFER_DESC outputDesc;
	outputDesc.Usage = D3D11_USAGE_DEFAULT;
	outputDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	outputDesc.ByteWidth = sizeof(Data) * data_num_;
	outputDesc.CPUAccessFlags = 0;
	outputDesc.StructureByteStride = sizeof(Data);
	outputDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	HR(d3d_device_->CreateBuffer(&outputDesc, nullptr, &output_buffer_));

	// Create a system memory version of the buffer to read the results back from.
	outputDesc.Usage = D3D11_USAGE_STAGING;
	outputDesc.BindFlags = 0;
	outputDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	HR(d3d_device_->CreateBuffer(&outputDesc, nullptr, &output_debug_buffer_));

	// Create shader resource view
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.BufferEx.FirstElement = 0;
	srvDesc.BufferEx.Flags = 0;
	srvDesc.BufferEx.NumElements = data_num_;
	HR(d3d_device_->CreateShaderResourceView(bufferA, &srvDesc, &inputA_SRV_));
	HR(d3d_device_->CreateShaderResourceView(bufferB, &srvDesc, &inputB_SRV_));

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.Flags = 0;
	uavDesc.Buffer.NumElements = data_num_;
	HR(d3d_device_->CreateUnorderedAccessView(output_buffer_, &uavDesc, &output_UAV_));

	ReleaseCOM(bufferA);
	ReleaseCOM(bufferB);
}

void VecAddApp::DoComputation()
{
	Effects::VecAddFX->SetInputA(inputA_SRV_);
	Effects::VecAddFX->SetInputB(inputB_SRV_);
	Effects::VecAddFX->SetOutput(output_UAV_);

	D3DX11_TECHNIQUE_DESC techDesc;
	Effects::VecAddFX->VecAddTech->GetDesc(&techDesc);

	for (int p = 0; p < techDesc.Passes; ++p)
	{
		ID3DX11EffectPass* pass = Effects::VecAddFX->VecAddTech->GetPassByIndex(p);
		pass->Apply(0, d3d_context_);

		d3d_context_->Dispatch(1, 1, 1);
	}

	// Unbind the input textures from the CS for good housekeeping.
	ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
	d3d_context_->CSSetShaderResources(0, 1, nullSRV);

	// Unbind output from compute shader (we are going to use this output as an input in the next pass, 
	// and a resource cannot be both an output and input at the same time.
	ID3D11UnorderedAccessView* nullUAV[1] = { nullptr };
	d3d_context_->CSSetUnorderedAccessViews(0, 1, nullUAV, 0);

	// Disable compute shader.
	d3d_context_->CSSetShader(nullptr, nullptr, 0);

	// Output the results into a file
	std::ofstream fout("results.txt");

	// Copy the output buffer to system memory.
	d3d_context_->CopyResource(output_debug_buffer_, output_buffer_);

	// Map the data for reading.
	D3D11_MAPPED_SUBRESOURCE mappedData;
	d3d_context_->Map(output_debug_buffer_, 0, D3D11_MAP_READ, 0, &mappedData);
	Data* data = (Data*)mappedData.pData;
	
	for (int i = 0; i < data_num_; ++i)
	{
		fout << "(" << data[i].v1.x << ", " << data[i].v1.y << ", " << data[i].v1.z <<
			"; " << data[i].v2.x << ", " << data[i].v2.y << ")" << std::endl;
	}

	d3d_context_->Unmap(output_debug_buffer_, 0);

	fout.close();
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	VecAddApp theApp(hInstance);

	if (!theApp.Init())
	{
		return 0;
	}

	theApp.DoComputation();

	return 0;
}