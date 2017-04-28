#pragma once

#include "d3dUtil.h"

class Camera;

class Ssao
{
public:
	Ssao(ID3D11Device* device, ID3D11DeviceContext* dc, int width, int height, float fovy, float farZ);
	~Ssao();

	ID3D11ShaderResourceView* NormalDepthSRV();
	ID3D11ShaderResourceView* AmbientSRV();

	/// Call when the backbuffer is resized.  
	void OnResize(int width, int height, float fovy, float farZ);

	/// Changes the render target to the NormalDepth render target.  Pass the 
	/// main depth buffer as the depth buffer to use when we render to the
	/// NormalDepth map.  This pass lays down the scene depth so that there in
	/// no overdraw in the subsequent rendering pass.
	void SetNormalDepthRenderTarget(ID3D11DepthStencilView* dsv);

	/// Changes the render target to the Ambient render target and draws a fullscreen
	/// quad to kick off the pixel shader to compute the AmbientMap.  We still keep the
	/// main depth buffer binded to the pipeline, but depth buffer read/writes
	/// are disabled, as we do not need the depth buffer computing the Ambient map.
	void ComputeSsao(const Camera& camera);

	/// Blurs the ambient map to smooth out the noise caused by only taking a
	/// few random samples per pixel.  We use an edge preserving blur so that 
	/// we do not blur across discontinuities--we want edges to remain edges.
	void BlurAmbientMap(int blurCount);

private:
	Ssao(const Ssao& rhs);
	Ssao& operator=(const Ssao& rhs);

	void BlurAmbientMap(ID3D11ShaderResourceView* inputSRV, ID3D11RenderTargetView* outputRTV, bool horzBlur);

	void BuildFrustumFarCorners(float fovy, float farZ);

	void BuildFullScreenQuad();

	void BuildTextureViews();
	void ReleaseTextureViews();

	void BuildRandomVectorTexture();

	void BuildOffsetVectors();

private:
	ID3D11Device* m_Device;
	ID3D11DeviceContext* m_DC;

	ID3D11Buffer* m_ScreenQuadVB;
	ID3D11Buffer* m_ScreenQuadIB;

	ID3D11ShaderResourceView* m_RandomVectorSRV;
	
	ID3D11RenderTargetView* m_NormalDepthRTV;
	ID3D11ShaderResourceView* m_NormalDepthSRV;

	// Need two for ping-ponging during blur.
	ID3D11RenderTargetView* m_AmbientRTV0;
	ID3D11ShaderResourceView* m_AmbientSRV0;

	ID3D11RenderTargetView* m_AmbientRTV1;
	ID3D11ShaderResourceView* m_AmbientSRV1;

	UINT m_RenderTargetWidth;
	UINT m_RenderTargetHeight;

	XMFLOAT4 m_FrustumFarCorners[4];
	XMFLOAT4 m_Offsets[14];

	D3D11_VIEWPORT m_AmbientMapViewport;
};