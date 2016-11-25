#pragma once
#include "d3dUtil.h"

class BlurFilter
{
public:
	BlurFilter();
	~BlurFilter();

	ID3D11ShaderResourceView* GetBlurredOutput();

	// Generate Gaussian blur weights.
	void SetGaussianWeights(float sigma);

	// Manually specify blur weights.
	void SetWeights(const float weights[9]);

	///<summary>
	/// The width and height should match the dimensions of the input texture to blur.
	/// It is OK to call Init() again to reinitialize the blur filter with a different 
	/// dimension or format.
	///</summary>
	void Init(ID3D11Device* device, UINT width, UINT height, DXGI_FORMAT format);

	///<summary>
	/// Blurs the input texture blurCount times.  Note that this modifies the input texture, not a copy of it.
	///</summary>
	void BlurInPlace(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* inputSRV, ID3D11UnorderedAccessView* inputUAV, int blurCount);


private:
	int m_Width;
	int m_Height;
	DXGI_FORMAT m_Format;

	ID3D11ShaderResourceView* m_BlurredOutputTexSRV;
	ID3D11UnorderedAccessView* m_BlurredOutputTexUAV;

};

