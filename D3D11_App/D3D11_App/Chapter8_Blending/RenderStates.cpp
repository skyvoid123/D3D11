#include "RenderStates.h"

ID3D11RasterizerState* RenderStates::WireframeRS = nullptr;
ID3D11RasterizerState* RenderStates::NoCullRS = nullptr;

ID3D11BlendState* RenderStates::AlphaToCoverageBS = nullptr;
ID3D11BlendState* RenderStates::TransparentBS = nullptr;

void RenderStates::InitAll(ID3D11Device* device)
{
	//
	// WireframeRS
	//
	D3D11_RASTERIZER_DESC wireframeDesc;
	ZeroMemory(&wireframeDesc, sizeof(wireframeDesc));
	wireframeDesc.FillMode = D3D11_FILL_WIREFRAME;
	wireframeDesc.CullMode = D3D11_CULL_BACK;
	wireframeDesc.FrontCounterClockwise = FALSE;
	wireframeDesc.DepthClipEnable = TRUE;
	HR(device->CreateRasterizerState(&wireframeDesc, &WireframeRS));

	//
	// NoCullRS
	//
	D3D11_RASTERIZER_DESC noCullDesc;
	ZeroMemory(&noCullDesc, sizeof(noCullDesc));
	noCullDesc.FillMode = D3D11_FILL_SOLID;
	noCullDesc.CullMode = D3D11_CULL_NONE;
	noCullDesc.FrontCounterClockwise = FALSE;
	noCullDesc.DepthClipEnable = TRUE;
	HR(device->CreateRasterizerState(&noCullDesc, &NoCullRS));

	//
	// AlphaToCoverageBS
	//
	D3D11_BLEND_DESC alphaToCoverageDesc;
	ZeroMemory(&alphaToCoverageDesc, sizeof(alphaToCoverageDesc));
	alphaToCoverageDesc.AlphaToCoverageEnable = TRUE;
	alphaToCoverageDesc.IndependentBlendEnable = FALSE;
	alphaToCoverageDesc.RenderTarget[0].BlendEnable = FALSE;
	alphaToCoverageDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	HR(device->CreateBlendState(&alphaToCoverageDesc, &AlphaToCoverageBS));

	//
	// TransparentBS
	//
	D3D11_BLEND_DESC transparentDesc;
	ZeroMemory(&transparentDesc, sizeof(transparentDesc));
	transparentDesc.AlphaToCoverageEnable = FALSE;
	transparentDesc.IndependentBlendEnable = FALSE;
	transparentDesc.RenderTarget[0].BlendEnable = TRUE;
	transparentDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	transparentDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	transparentDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	transparentDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	transparentDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	transparentDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	transparentDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	HR(device->CreateBlendState(&transparentDesc, &TransparentBS));
}

void RenderStates::DestroyAll()
{
	ReleaseCOM(WireframeRS);
	ReleaseCOM(NoCullRS);
	ReleaseCOM(AlphaToCoverageBS);
	ReleaseCOM(TransparentBS);
}