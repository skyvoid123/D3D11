#pragma once

#include "d3dUtil.h"
#include "Camera.h"

class ShadowMap
{
public:
	ShadowMap(ID3D11Device* device, UINT width, UINT height);
	~ShadowMap();

	ID3D11ShaderResourceView* DepthMapSRV()
	{
		return m_DepthMapSRV;
	}

	void BindDsvAndSetNullRenderTarget(ID3D11DeviceContext* dc);

private:
	ShadowMap(const ShadowMap& rhs);
	ShadowMap& operator=(const ShadowMap& rhs);

private:
	UINT m_Width;
	UINT m_Height;

	ID3D11ShaderResourceView* m_DepthMapSRV;
	ID3D11DepthStencilView* m_DepthMapDSV;

	D3D11_VIEWPORT m_Viewport;
};