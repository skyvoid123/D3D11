#include "Sky.h"
#include "DDSTextureLoader.h"
#include "GeometryGenerator.h"
#include "Camera.h"
#include "Vertex.h"
#include "Effects.h"

Sky::Sky(ID3D11Device* device, const std::wstring& cubemapFilename, float skySphereRadius)
{
	ID3D11Resource* tex_res = nullptr;
	HR(DirectX::CreateDDSTextureFromFile(device,
		cubemapFilename.c_str(), &tex_res, &m_CubeMapSRV));
	ReleaseCOM(tex_res);

	GeometryGenerator::MeshData sphere;
	GeometryGenerator geoGen;
	geoGen.CreateSphere(skySphereRadius, 30, 30, sphere);

	std::vector<XMFLOAT3> vertices(sphere.Vertices.size());
	for (int i = 0; i < vertices.size(); ++i)
	{
		vertices[i] = sphere.Vertices[i].Position;
	}

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.ByteWidth = sizeof(XMFLOAT3) * vertices.size();
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vData;
	vData.pSysMem = &vertices[0];
	HR(device->CreateBuffer(&vbd, &vData, &m_SkyVB));


	m_IndexCount = sphere.Indices.size();

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * m_IndexCount;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.StructureByteStride = 0;
	ibd.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA iData;
	iData.pSysMem = &sphere.Indices[0];
	HR(device->CreateBuffer(&ibd, &iData, &m_SkyIB));
}

Sky::~Sky()
{
	ReleaseCOM(m_SkyVB);
	ReleaseCOM(m_SkyIB);
	ReleaseCOM(m_CubeMapSRV);
}

ID3D11ShaderResourceView* Sky::GetCubeMapSRV()
{
	return m_CubeMapSRV;
}

void Sky::Draw(ID3D11DeviceContext* dc, const Camera& camera)
{
	XMFLOAT3 eyePos = camera.GetPosition();
	XMMATRIX W = XMMatrixTranslation(eyePos.x, eyePos.y, eyePos.z);
	XMMATRIX WVP = W * camera.ViewProj();

	Effects::SkyFX->SetWorldViewProj(WVP);
	Effects::SkyFX->SetCubeMap(m_CubeMapSRV);

	UINT stride = sizeof(XMFLOAT3);
	UINT offset = 0;
	dc->IASetInputLayout(InputLayouts::Pos);
	dc->IASetVertexBuffers(0, 1, &m_SkyVB, &stride, &offset);
	dc->IASetIndexBuffer(m_SkyIB, DXGI_FORMAT_R32_UINT, 0);
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	D3DX11_TECHNIQUE_DESC desc;
	Effects::SkyFX->SkyTech->GetDesc(&desc);

	for (int p = 0; p < desc.Passes; ++p)
	{
		auto pass = Effects::SkyFX->SkyTech->GetPassByIndex(p);
		pass->Apply(0, dc);
		dc->DrawIndexed(m_IndexCount, 0, 0);
	}
}