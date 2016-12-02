#include "BlurFilter.h"
#include "Effects.h"

BlurFilter::BlurFilter()
	: m_BlurredOutputTexSRV(nullptr)
	, m_BlurredOutputTexUAV(nullptr)
{
}

BlurFilter::~BlurFilter()
{
	ReleaseCOM(m_BlurredOutputTexSRV);
	ReleaseCOM(m_BlurredOutputTexUAV);
}


void BlurFilter::SetGaussianWeights(float sigma)
{
	float d = 2.f * sigma * sigma;
	float weights[9];
	float sum = 0.f;
	for (int i = 0; i < 9; ++i)
	{
		float x = (float)i;
		weights[i] = expf(-x * x / d);
		
		sum += weights[i];
	}

	// Divide by the sum so all the weights add up to 1.0.
	for (int i = 0; i < 9; ++i)
	{
		weights[i] /= sum;
	}

	Effects::BlurFX->SetWeights(weights);
}

void BlurFilter::SetWeights(const float weights[9])
{
	Effects::BlurFX->SetWeights(weights);
}

void BlurFilter::Init(ID3D11Device* device, UINT width, UINT height, DXGI_FORMAT format)
{
	ReleaseCOM(m_BlurredOutputTexSRV);
	ReleaseCOM(m_BlurredOutputTexUAV);

	m_Width = width;
	m_Height = height;
	m_Format = format;

	// Note, compressed formats cannot be used for UAV.  We get error like:
	// ERROR: ID3D11Device::CreateTexture2D: The format (0x4d, BC3_UNORM) 
	// cannot be bound as an UnorderedAccessView, or cast to a format that
	// could be bound as an UnorderedAccessView.  Therefore this format 
	// does not support D3D11_BIND_UNORDERED_ACCESS.

	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = format;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	ID3D11Texture2D* blurredTex = nullptr;
	HR(device->CreateTexture2D(&texDesc, nullptr, &blurredTex));

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	HR(device->CreateShaderResourceView(blurredTex, &srvDesc, &m_BlurredOutputTexSRV));

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	uavDesc.Format = format;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;
	HR(device->CreateUnorderedAccessView(blurredTex, &uavDesc, &m_BlurredOutputTexUAV));

	ReleaseCOM(blurredTex);
}

void BlurFilter::BlurInPlace(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* inputSRV, ID3D11UnorderedAccessView* inputUAV, int blurCount)
{
	//
	// Run the compute shader to blur the offscreen texture.
	// 
	for (int i = 0; i < blurCount; ++i)
	{
		// HORIZONTAL blur pass.
		D3DX11_TECHNIQUE_DESC techDesc;
		auto fx = Effects::BlurFX;
		fx->HorzBlurTech->GetDesc(&techDesc);
		for (int p = 0; p < techDesc.Passes; ++p)
		{
			fx->SetInputMap(inputSRV);
			fx->SetOutputMap(m_BlurredOutputTexUAV);
			fx->HorzBlurTech->GetPassByIndex(p)->Apply(0, dc);

			// How many groups do we need to dispatch to cover a row of pixels, where each
			// group covers 256 pixels (the 256 is defined in the ComputeShader).
			UINT numGroupsX = (UINT)ceilf(m_Width / 256.f);
			dc->Dispatch(numGroupsX, m_Height, 1);
		}

		// Unbind the input texture from the CS for good housekeeping.
		ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
		dc->CSSetShaderResources(0, 1, nullSRV);

		// Unbind output from compute shader (we are going to use this output as an input in the next pass, 
		// and a resource cannot be both an output and input at the same time.
		ID3D11UnorderedAccessView* nullUAV[1] = { nullptr };
		dc->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);

		// VERTICAL blur pass.
		fx->VertBlurTech->GetDesc(&techDesc);
		for (int p = 0; p < techDesc.Passes; ++p)
		{
			fx->SetInputMap(m_BlurredOutputTexSRV);
			fx->SetOutputMap(inputUAV);
			fx->VertBlurTech->GetPassByIndex(p)->Apply(0, dc);

			// How many groups do we need to dispatch to cover a column of pixels, where each
			// group covers 256 pixels  (the 256 is defined in the ComputeShader).
			UINT numGroupsY = (UINT)ceilf(m_Height / 256.f);
			dc->Dispatch(m_Width, numGroupsY, 1);
		}

		dc->CSSetShaderResources(0, 1, nullSRV);
		dc->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);
	}

	// Disable compute shader.
	dc->CSSetShader(nullptr, nullptr, 0);
}