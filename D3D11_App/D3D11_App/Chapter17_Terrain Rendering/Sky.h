#pragma once
#include "d3dUtil.h"

class Camera;

class Sky
{
public:
	Sky(ID3D11Device* device, const std::wstring& cubemapFilename, float skySphereRadius);
	~Sky();

	ID3D11ShaderResourceView* GetCubeMapSRV();

	void Draw(ID3D11DeviceContext* dc, const Camera& camera);

private:
	Sky(const Sky&);
	Sky& operator=(const Sky&);

private:
	ID3D11Buffer* m_SkyVB;
	ID3D11Buffer* m_SkyIB;

	ID3D11ShaderResourceView* m_CubeMapSRV;

	UINT m_IndexCount;
};