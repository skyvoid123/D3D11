#include "Ssao.h"
#include "Camera.h"
#include "Effects.h"
#include "Vertex.h"
#include <DirectXPackedVector.h>

Ssao::Ssao(ID3D11Device* device, ID3D11DeviceContext* dc, int width, int height, float fovy, float farZ)
	: m_Device(device)
	, m_DC(dc)
	, m_ScreenQuadVB(nullptr)
	, m_ScreenQuadIB(nullptr)
	, m_RandomVectorSRV(nullptr)
	, m_NormalDepthRTV(nullptr)
	, m_NormalDepthSRV(nullptr)
	, m_AmbientRTV0(nullptr)
	, m_AmbientSRV0(nullptr)
	, m_AmbientRTV1(nullptr)
	, m_AmbientSRV1(nullptr)
{
	OnResize(width, height, fovy, farZ);
	BuildFullScreenQuad();
	BuildOffsetVectors();
	BuildRandomVectorTexture();
}

Ssao::~Ssao()
{
	ReleaseCOM(m_ScreenQuadVB);
	ReleaseCOM(m_ScreenQuadIB);
	ReleaseCOM(m_RandomVectorSRV);

	ReleaseTextureViews();
}

ID3D11ShaderResourceView* Ssao::NormalDepthSRV()
{
	return m_NormalDepthSRV;
}

ID3D11ShaderResourceView* Ssao::AmbientSRV()
{
	return m_AmbientSRV0;
}

void Ssao::OnResize(int width, int height, float fovy, float farZ)
{
	m_RenderTargetWidth = width;
	m_RenderTargetHeight = height;

	// We render to ambient map at half the resolution.
	m_AmbientMapViewport.TopLeftX = 0.0f;
	m_AmbientMapViewport.TopLeftY = 0.0f;
	m_AmbientMapViewport.Width = width / 2.0f;
	m_AmbientMapViewport.Height = height / 2.0f;
	m_AmbientMapViewport.MinDepth = 0.0f;
	m_AmbientMapViewport.MaxDepth = 1.0f;

	BuildFrustumFarCorners(fovy, farZ);
	BuildTextureViews();
}

void Ssao::SetNormalDepthRenderTarget(ID3D11DepthStencilView* dsv)
{
	m_DC->OMSetRenderTargets(1, &m_NormalDepthRTV, dsv);

	// Clear view space normal to (0,0,-1) and clear depth to be very far away.  
	float clearColor[] = { 0.0f, 0.0f, -1.0f, 1e5f };
	m_DC->ClearRenderTargetView(m_NormalDepthRTV, clearColor);
}

void Ssao::ComputeSsao(const Camera& camera)
{
	// Bind the ambient map as the render target.  Observe that this pass does not bind 
	// a depth/stencil buffer--it does not need it, and without one, no depth test is
	// performed, which is what we want.
	m_DC->OMSetRenderTargets(1, &m_AmbientRTV0, nullptr);
	m_DC->ClearRenderTargetView(m_AmbientRTV0, (const float*)&Colors::Black);
	m_DC->RSSetViewports(1, &m_AmbientMapViewport);

	// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
	static const XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	XMMATRIX P = camera.Proj();
	XMMATRIX PT = XMMatrixMultiply(P, T);

	Effects::SsaoFX->SetViewToTexSpace(PT);
	Effects::SsaoFX->SetOffsetVectors(m_Offsets);
	Effects::SsaoFX->SetFrustumCorners(m_FrustumFarCorners);
	Effects::SsaoFX->SetNormalDepthMap(m_NormalDepthSRV);
	Effects::SsaoFX->SetRandomVecMap(m_RandomVectorSRV);

	UINT stride = sizeof(Vertex::Basic32);
	UINT offset = 0;

	m_DC->IASetInputLayout(InputLayouts::Basic32);
	m_DC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_DC->IASetVertexBuffers(0, 1, &m_ScreenQuadVB, &stride, &offset);
	m_DC->IASetIndexBuffer(m_ScreenQuadIB, DXGI_FORMAT_R16_UINT, 0);

	ID3DX11EffectTechnique* tech = Effects::SsaoFX->SsaoTech;
	D3DX11_TECHNIQUE_DESC techDesc;

	tech->GetDesc(&techDesc);
	for (int p = 0; p < techDesc.Passes; ++p)
	{
		tech->GetPassByIndex(0)->Apply(0, m_DC);
		m_DC->DrawIndexed(6, 0, 0);
	}
}

void Ssao::BlurAmbientMap(int blurCount)
{
	for (int i = 0; i < blurCount; ++i)
	{
		// Ping-pong the two ambient map textures as we apply
		// horizontal and vertical blur passes.
		BlurAmbientMap(m_AmbientSRV0, m_AmbientRTV1, true);
		BlurAmbientMap(m_AmbientSRV1, m_AmbientRTV0, false);
	}
}

void Ssao::BlurAmbientMap(ID3D11ShaderResourceView* inputSRV, ID3D11RenderTargetView* outputRTV, bool horzBlur)
{
	m_DC->OMSetRenderTargets(1, &outputRTV, nullptr);
	m_DC->ClearRenderTargetView(outputRTV, (const float*)&Colors::Black);
	m_DC->RSSetViewports(1, &m_AmbientMapViewport);

	Effects::SsaoBlurFX->SetTexelWidth(1.0f / m_AmbientMapViewport.Width);
	Effects::SsaoBlurFX->SetTexelHeight(1.0f / m_AmbientMapViewport.Height);
	Effects::SsaoBlurFX->SetNormalDepthMap(m_NormalDepthSRV);
	Effects::SsaoBlurFX->SetInputImage(inputSRV);

	ID3DX11EffectTechnique* tech;
	if (horzBlur)
	{
		tech = Effects::SsaoBlurFX->HorzBlurTech;
	}
	else
	{
		tech = Effects::SsaoBlurFX->VertBlurTech;
	}

	UINT stride = sizeof(Vertex::Basic32);
	UINT offset = 0;

	m_DC->IASetInputLayout(InputLayouts::Basic32);
	m_DC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_DC->IASetVertexBuffers(0, 1, &m_ScreenQuadVB, &stride, &offset);
	m_DC->IASetIndexBuffer(m_ScreenQuadIB, DXGI_FORMAT_R16_UINT, 0);

	D3DX11_TECHNIQUE_DESC techDesc;
	tech->GetDesc(&techDesc);
	for (int p = 0; p < techDesc.Passes; ++p)
	{
		tech->GetPassByIndex(p)->Apply(0, m_DC);
		m_DC->DrawIndexed(6, 0, 0);

		// Unbind the input SRV as it is going to be an output in the next blur.
		Effects::SsaoBlurFX->SetInputImage(nullptr);
		tech->GetPassByIndex(p)->Apply(0, m_DC);
	}
}

void Ssao::BuildFrustumFarCorners(float fovy, float farZ)
{
	float aspect = (float)m_RenderTargetWidth / m_RenderTargetHeight;
	float halfHeight = farZ * tanf(0.5f * fovy);
	float halfWidth = aspect * halfHeight;

	m_FrustumFarCorners[0] = XMFLOAT4(-halfWidth, -halfHeight, farZ, 1.0f);
	m_FrustumFarCorners[1] = XMFLOAT4(-halfWidth, +halfHeight, farZ, 1.0f);
	m_FrustumFarCorners[2] = XMFLOAT4(+halfWidth, +halfHeight, farZ, 1.0f);
	m_FrustumFarCorners[3] = XMFLOAT4(+halfWidth, -halfHeight, farZ, 1.0f);
}

void Ssao::BuildFullScreenQuad()
{
	Vertex::Basic32 v[4];

	v[0].Pos = XMFLOAT3(-1.0f, -1.0f, 0.0f);
	v[1].Pos = XMFLOAT3(-1.0f, +1.0f, 0.0f);
	v[2].Pos = XMFLOAT3(+1.0f, +1.0f, 0.0f);
	v[3].Pos = XMFLOAT3(+1.0f, -1.0f, 0.0f);

	// Store far plane frustum corner indices in Normal.x slot.
	v[0].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
	v[1].Normal = XMFLOAT3(1.0f, 0.0f, 0.0f);
	v[2].Normal = XMFLOAT3(2.0f, 0.0f, 0.0f);
	v[3].Normal = XMFLOAT3(3.0f, 0.0f, 0.0f);

	v[0].Tex = XMFLOAT2(0.0f, 1.0f);
	v[1].Tex = XMFLOAT2(0.0f, 0.0f);
	v[2].Tex = XMFLOAT2(1.0f, 0.0f);
	v[3].Tex = XMFLOAT2(1.0f, 1.0f);

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex::Basic32) * 4;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = v;

	HR(m_Device->CreateBuffer(&vbd, &vinitData, &m_ScreenQuadVB));

	USHORT indices[6] =
	{
		0, 1, 2,
		0, 2, 3
	};

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(USHORT) * 6;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.StructureByteStride = 0;
	ibd.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = indices;

	HR(m_Device->CreateBuffer(&ibd, &iinitData, &m_ScreenQuadIB));
}

void Ssao::BuildTextureViews()
{
	ReleaseTextureViews();

	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = m_RenderTargetWidth;
	texDesc.Height = m_RenderTargetHeight;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	ID3D11Texture2D* normalDepthTex = nullptr;
	HR(m_Device->CreateTexture2D(&texDesc, nullptr, &normalDepthTex));
	HR(m_Device->CreateShaderResourceView(normalDepthTex, nullptr, &m_NormalDepthSRV));
	HR(m_Device->CreateRenderTargetView(normalDepthTex, nullptr, &m_NormalDepthRTV));
	ReleaseCOM(normalDepthTex);

	// Render ambient map at half resolution.
	texDesc.Width = m_RenderTargetWidth / 2;
	texDesc.Height = m_RenderTargetHeight / 2;
	texDesc.Format = DXGI_FORMAT_R16_FLOAT;

	ID3D11Texture2D* ambientTex0 = nullptr;
	HR(m_Device->CreateTexture2D(&texDesc, nullptr, &ambientTex0));
	HR(m_Device->CreateShaderResourceView(ambientTex0, nullptr, &m_AmbientSRV0));
	HR(m_Device->CreateRenderTargetView(ambientTex0, nullptr, &m_AmbientRTV0));
	ReleaseCOM(ambientTex0);

	ID3D11Texture2D* ambientTex1 = nullptr;
	HR(m_Device->CreateTexture2D(&texDesc, nullptr, &ambientTex1));
	HR(m_Device->CreateShaderResourceView(ambientTex1, nullptr, &m_AmbientSRV1));
	HR(m_Device->CreateRenderTargetView(ambientTex1, nullptr, &m_AmbientRTV1));
	ReleaseCOM(ambientTex1);
}

void Ssao::ReleaseTextureViews()
{
	ReleaseCOM(m_NormalDepthRTV);
	ReleaseCOM(m_NormalDepthSRV);
	ReleaseCOM(m_AmbientRTV0);
	ReleaseCOM(m_AmbientSRV0);
	ReleaseCOM(m_AmbientRTV1);
	ReleaseCOM(m_AmbientSRV1);
}

void Ssao::BuildRandomVectorTexture()
{
	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = 256;
	texDesc.Height = 256;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_IMMUTABLE;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA initData;
	initData.SysMemPitch = 256 * sizeof(DirectX::PackedVector::XMCOLOR);

	DirectX::PackedVector::XMCOLOR color[256 * 256];
	for (int i = 0; i < 256; ++i)
	{
		for (int j = 0; j < 256; ++j)
		{
			XMFLOAT3 v(MathHelper::RandF(), MathHelper::RandF(), MathHelper::RandF());

			color[i * 256 + j] = DirectX::PackedVector::XMCOLOR(v.x, v.y, v.z, 0.0f);
		}
	}

	initData.pSysMem = color;

	ID3D11Texture2D* tex = 0;
	HR(m_Device->CreateTexture2D(&texDesc, &initData, &tex));
	HR(m_Device->CreateShaderResourceView(tex, 0, &m_RandomVectorSRV));

	// view saves a reference.
	ReleaseCOM(tex);
}

void Ssao::BuildOffsetVectors()
{
	// Start with 14 uniformly distributed vectors.  We choose the 8 corners of the cube
	// and the 6 center points along each cube face.  We always alternate the points on 
	// opposites sides of the cubes.  This way we still get the vectors spread out even
	// if we choose to use less than 14 samples.

	// 8 cube corners
	m_Offsets[0] = XMFLOAT4(+1.0f, +1.0f, +1.0f, 0.0f);
	m_Offsets[1] = XMFLOAT4(-1.0f, -1.0f, -1.0f, 0.0f);

	m_Offsets[2] = XMFLOAT4(-1.0f, +1.0f, +1.0f, 0.0f);
	m_Offsets[3] = XMFLOAT4(+1.0f, -1.0f, -1.0f, 0.0f);

	m_Offsets[4] = XMFLOAT4(+1.0f, +1.0f, -1.0f, 0.0f);
	m_Offsets[5] = XMFLOAT4(-1.0f, -1.0f, +1.0f, 0.0f);

	m_Offsets[6] = XMFLOAT4(-1.0f, +1.0f, -1.0f, 0.0f);
	m_Offsets[7] = XMFLOAT4(+1.0f, -1.0f, +1.0f, 0.0f);

	// 6 centers of cube faces
	m_Offsets[8] = XMFLOAT4(-1.0f, 0.0f, 0.0f, 0.0f);
	m_Offsets[9] = XMFLOAT4(+1.0f, 0.0f, 0.0f, 0.0f);

	m_Offsets[10] = XMFLOAT4(0.0f, -1.0f, 0.0f, 0.0f);
	m_Offsets[11] = XMFLOAT4(0.0f, +1.0f, 0.0f, 0.0f);

	m_Offsets[12] = XMFLOAT4(0.0f, 0.0f, -1.0f, 0.0f);
	m_Offsets[13] = XMFLOAT4(0.0f, 0.0f, +1.0f, 0.0f);

	for (int i = 0; i < 14; ++i)
	{
		// Create random lengths in [0.25, 1.0].
		float s = MathHelper::RandF(0.25f, 1.0f);

		XMVECTOR v = s * XMVector4Normalize(XMLoadFloat4(&m_Offsets[i]));

		XMStoreFloat4(&m_Offsets[i], v);
	}

}