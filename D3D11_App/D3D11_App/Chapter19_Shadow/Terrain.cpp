#include "Terrain.h"
#include "Camera.h"
#include "DDSTextureLoader.h"
#include "Vertex.h"
#include "Effects.h"
#include <DirectXPackedVector.h>

Terrain::Terrain()
	: m_QuadPatchVB(nullptr)
	, m_QuadPatchIB(nullptr)
	, m_HeightMapSRV(nullptr)
	, m_LayerMapArraySRV(nullptr)
	, m_BlendMapSRV(nullptr)
	, m_NumPatchVertices(0)
	, m_NumPatchQuadFaces(0)
	, m_NumPatchVertRows(0)
	, m_NumPatchVertCols(0)
{
	XMStoreFloat4x4(&m_World, XMMatrixIdentity());

	m_Mat.Ambient = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_Mat.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_Mat.Specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 64.0f);
	m_Mat.Reflect = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
}

Terrain::~Terrain()
{
	ReleaseCOM(m_QuadPatchVB);
	ReleaseCOM(m_QuadPatchIB);
	ReleaseCOM(m_HeightMapSRV);
	ReleaseCOM(m_LayerMapArraySRV);
	ReleaseCOM(m_BlendMapSRV);
}

float Terrain::GetWidth() const
{
	// Total terrain width.
	return (m_Info.HeightmapWidth - 1) * m_Info.CellSpacing;
}

float Terrain::GetDepth() const
{
	// Total terrain depth.
	return (m_Info.HeightmapHeight - 1) * m_Info.CellSpacing;
}

float Terrain::GetHeight(float x, float z) const
{
	// Transform from terrain local space to "cell" space.
	float c = (x + 0.5f * GetWidth()) / m_Info.CellSpacing;
	float d = (z - 0.5f * GetDepth()) / -m_Info.CellSpacing;

	// Get the row and column we are in.
	int row = (int)floorf(d);
	int col = (int)floorf(c);

	// Grab the heights of the cell we are in.
	// A*--*B
	//  |    / |
	//  |  /   |
	// C*--*D
	float A = m_Heightmap[row * m_Info.HeightmapWidth + col];
	float B = m_Heightmap[row * m_Info.HeightmapWidth + col + 1];
	float C = m_Heightmap[(row + 1) * m_Info.HeightmapWidth + col];
	float D = m_Heightmap[(row + 1) * m_Info.HeightmapWidth + col + 1];

	// Where we are relative to the cell.
	float s = c - col;
	float t = d - row;

	// If upper triangle ABC.
	if (s + t <= 1.0f)
	{
		float uy = B - A;
		float vy = C - A;
		return A + s * uy + t * vy;
	}
	else  // lower triangle DCB.
	{
		float uy = C - D;
		float vy = B - D;
		return D + (1.0f - s) * uy + (1.0f - t) * vy;
	}
}

XMMATRIX Terrain::GetWorld() const
{
	return XMLoadFloat4x4(&m_World);
}

void Terrain::SetWorld(CXMMATRIX M)
{
	XMStoreFloat4x4(&m_World, M);
}

void Terrain::Init(ID3D11Device * device, ID3D11DeviceContext * dc, const InitInfo & initInfo)
{
	m_Info = initInfo;

	// Divide heightmap into patches such that each patch has CellsPerPatch.
	m_NumPatchVertRows = (m_Info.HeightmapHeight - 1) / CellsPerPatch + 1;
	m_NumPatchVertCols = (m_Info.HeightmapWidth - 1) / CellsPerPatch + 1;

	m_NumPatchVertices = m_NumPatchVertRows * m_NumPatchVertCols;
	m_NumPatchQuadFaces = (m_NumPatchVertRows - 1) * (m_NumPatchVertCols - 1);

	LoadHeightmap();
	Smooth();
	CalcAllPatchBoundsY();

	BuildQuadPatchVB(device);
	BuildQuadPatchIB(device);
	BuildHeightmapSRV(device);

	ID3D11Resource* texRes = nullptr;

	HR(DirectX::CreateDDSTextureFromFile(device,
		m_Info.LayerMapArrayFilename.c_str(), &texRes, &m_LayerMapArraySRV));
	ReleaseCOM(texRes);

	HR(DirectX::CreateDDSTextureFromFile(device,
		m_Info.BlendMapFilename.c_str(), &texRes, &m_BlendMapSRV));
	ReleaseCOM(texRes);
}

void Terrain::Draw(ID3D11DeviceContext * dc, const Camera & cam, DirectionalLight lights[3])
{
	dc->IASetInputLayout(InputLayouts::Terrain);
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);

	UINT stride = sizeof(Vertex::Terrain);
	UINT offset = 0;

	dc->IASetVertexBuffers(0, 1, &m_QuadPatchVB, &stride, &offset);
	dc->IASetIndexBuffer(m_QuadPatchIB, DXGI_FORMAT_R16_UINT, 0);

	XMMATRIX viewProj = cam.ViewProj();
	XMMATRIX world = XMLoadFloat4x4(&m_World);
	XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
	XMMATRIX worldViewProj = world * viewProj;

	XMFLOAT4 worldPlanes[6];
	ExtractFrustumPlanes(worldPlanes, viewProj);

	// Set pet frame constants
	Effects::TerrainFX->SetViewProj(viewProj);
	Effects::TerrainFX->SetEyePosW(cam.GetPosition());
	Effects::TerrainFX->SetDirLights(lights);
	Effects::TerrainFX->SetFogColor(Colors::Silver);
	Effects::TerrainFX->SetFogStart(15.0f);
	Effects::TerrainFX->SetFogRange(175.0f);
	Effects::TerrainFX->SetMinDist(20.0f);
	Effects::TerrainFX->SetMaxDist(500.0f);
	Effects::TerrainFX->SetMinTess(0.0f);
	Effects::TerrainFX->SetMaxTess(6.0f);
	Effects::TerrainFX->SetTexelCellSpaceU(1.0f / m_Info.HeightmapWidth);
	Effects::TerrainFX->SetTexelCellSpaceV(1.0f / m_Info.HeightmapHeight);
	Effects::TerrainFX->SetWorldCellSpace(m_Info.CellSpacing);
	Effects::TerrainFX->SetWorldFrustumPlanes(worldPlanes);

	Effects::TerrainFX->SetLayerMapArray(m_LayerMapArraySRV);
	Effects::TerrainFX->SetBlendMap(m_BlendMapSRV);
	Effects::TerrainFX->SetHeightMap(m_HeightMapSRV);

	Effects::TerrainFX->SetMaterial(m_Mat);

	ID3DX11EffectTechnique* tech = Effects::TerrainFX->Light1Tech;
	D3DX11_TECHNIQUE_DESC techDesc;
	tech->GetDesc(&techDesc);

	for (int i = 0; i < techDesc.Passes; ++i)
	{
		ID3DX11EffectPass* pass = tech->GetPassByIndex(i);
		pass->Apply(0, dc);
		dc->DrawIndexed(m_NumPatchQuadFaces * 4, 0, 0);
	}

	// FX sets tessellation stages, but it does not disable them.  So do that here
	// to turn off tessellation.
	dc->HSSetShader(nullptr, nullptr, 0);
	dc->DSSetShader(nullptr, nullptr, 0);
}

void Terrain::LoadHeightmap()
{
	// A height for each vertex
	std::vector<unsigned char> in(m_Info.HeightmapWidth * m_Info.HeightmapHeight);

	std::ifstream fin(m_Info.HeightMapFilename, std::ios_base::binary);

	if (fin)
	{
		fin.read((char*)&in[0], in.size());
		fin.close();
	}

	// Copy the array data into a float array and scale it.
	m_Heightmap.resize(in.size());
	for (int i = 0; i < m_Heightmap.size(); ++i)
	{
		m_Heightmap[i] = (in[i] / 255.0f) * m_Info.HeightScale;
	}
}

void Terrain::Smooth()
{
	std::vector<float> dest(m_Heightmap.size());

	for (int i = 0; i < m_Info.HeightmapHeight; ++i)
	{
		for (int j = 0; j < m_Info.HeightmapWidth; ++j)
		{
			dest[i * m_Info.HeightmapWidth + j] = Average(i, j);
		}
	}

	// Replace the old heightmap with the filtered one.
	m_Heightmap = dest;
}

bool Terrain::InBounds(int i, int j)
{
	// True if ij are valid indices; false otherwise.
	return
		i >= 0 && i < (int)m_Info.HeightmapHeight &&
		j >= 0 && j < (int)m_Info.HeightmapWidth;
}

float Terrain::Average(int i, int j)
{
	// Function computes the average height of the ij element.
	// It averages itself with its eight neighbor pixels.  Note
	// that if a pixel is missing neighbor, we just don't include it
	// in the average--that is, edge pixels don't have a neighbor pixel.
	//
	// ----------
	// | 1| 2| 3|
	// ----------
	// |4 |ij| 6|
	// ----------
	// | 7| 8| 9|
	// ----------

	float sum = 0.0f;
	int num = 0;

	for (int m = i - 1; m <= i + 1; ++m)
	{
		for (int n = j - 1; n <= j + 1; ++n)
		{
			if (InBounds(m, n))
			{
				sum += m_Heightmap[m * m_Info.HeightmapWidth + n];
				++num;
			}
		}
	}

	return sum / num;
}

void Terrain::CalcAllPatchBoundsY()
{
	m_PatchBoundY.resize(m_NumPatchQuadFaces);

	// For each patch
	for (int i = 0; i < m_NumPatchVertRows - 1; ++i)
	{
		for (int j = 0; j < m_NumPatchVertCols - 1; ++j)
		{
			CalcPatchBoundsY(i, j);
		}
	}
}

void Terrain::CalcPatchBoundsY(UINT i, UINT j)
{
	// Scan the heightmap values this patch covers and compute the min/max height.

	int x0 = j * CellsPerPatch;
	int x1 = (j + 1) * CellsPerPatch;

	int y0 = i * CellsPerPatch;
	int y1 = (i + 1) * CellsPerPatch;

	float minY = +MathHelper::Infinity;
	float maxY = -MathHelper::Infinity;

	for (int y = y0; y <= y1;  ++y)
	{
		for (int x = x0; x <= x1; ++x)
		{
			int idx = y * m_Info.HeightmapWidth + x;
			minY = MathHelper::Min(minY, m_Heightmap[idx]);
			maxY = MathHelper::Max(maxY, m_Heightmap[idx]);
		}
	}

	int patchID = i * (m_NumPatchVertCols - 1) + j;
	m_PatchBoundY[patchID] = XMFLOAT2(minY, maxY);
}

void Terrain::BuildQuadPatchVB(ID3D11Device * device)
{
	std::vector<Vertex::Terrain> vertices(m_NumPatchVertices);
	
	float halfWidth = 0.5f * GetWidth();
	float halfDepth = 0.5f * GetDepth();

	float patchWidth = GetWidth() / (m_NumPatchVertCols - 1);
	float patchDepth = GetDepth() / (m_NumPatchVertRows - 1);
	float du = 1.0f / (m_NumPatchVertCols - 1);
	float dv = 1.0f / (m_NumPatchVertRows - 1);

	for (int i = 0; i < m_NumPatchVertRows; ++i)
	{
		float z = halfDepth - i * patchDepth;
		for (int j = 0; j < m_NumPatchVertCols; ++j)
		{
			float x = -halfWidth + j * patchWidth;

			vertices[i * m_NumPatchVertCols + j].Pos = XMFLOAT3(x, 0, z);

			// Stretch texture over grid.
			vertices[i * m_NumPatchVertCols + j].Tex.x = j * du;
			vertices[i * m_NumPatchVertCols + j].Tex.y = i * dv;
		}
	}

	// Store axis-aligned bounding box y-bounds in upper-left patch corner.
	for (int i = 0; i < m_NumPatchVertRows - 1; ++i)
	{
		for (int j = 0; j < m_NumPatchVertCols - 1; ++j)
		{
			int patchID = i * (m_NumPatchVertCols - 1) + j;
			vertices[i * m_NumPatchVertCols + j].BoundsY = m_PatchBoundY[patchID];
		}
	}

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex::Terrain) * vertices.size();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = &vertices[0];
	HR(device->CreateBuffer(&vbd, &data, &m_QuadPatchVB));
}

void Terrain::BuildQuadPatchIB(ID3D11Device * device)
{
	std::vector<USHORT> indices(m_NumPatchQuadFaces * 4);	// 4 indices per quad face
	
	// Iterate over each quad and compute indices.
	int idx = 0;
	for (int i = 0; i < m_NumPatchVertRows - 1; ++i)
	{
		for (int j = 0; j < m_NumPatchVertCols - 1; ++j)
		{
			// Top row of 2x2 quad patch
			indices[idx++] = i * m_NumPatchVertCols + j;
			indices[idx++] = i * m_NumPatchVertCols + j + 1;

			// Bottom row of 2x2 quad patch
			indices[idx++] =( i + 1) * m_NumPatchVertCols + j;
			indices[idx++] = (i + 1) * m_NumPatchVertCols + j + 1;
		}
	}

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(USHORT) * indices.size();
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	ibd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = &indices[0];
	HR(device->CreateBuffer(&ibd, &data, &m_QuadPatchIB));
}

void Terrain::BuildHeightmapSRV(ID3D11Device * device)
{
	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = m_Info.HeightmapWidth;
	texDesc.Height = m_Info.HeightmapHeight;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R16_FLOAT;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	std::vector<DirectX::PackedVector::HALF> hmap(m_Heightmap.size());
	std::transform(m_Heightmap.begin(), m_Heightmap.end(), hmap.begin(), DirectX::PackedVector::XMConvertFloatToHalf);

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = &hmap[0];
	data.SysMemPitch = m_Info.HeightmapWidth * sizeof(DirectX::PackedVector::HALF);
	data.SysMemSlicePitch = 0;

	ID3D11Texture2D* hmapTex = nullptr;
	HR(device->CreateTexture2D(&texDesc, &data, &hmapTex));

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;
	HR(device->CreateShaderResourceView(hmapTex, &srvDesc, &m_HeightMapSRV));

	ReleaseCOM(hmapTex);
}
