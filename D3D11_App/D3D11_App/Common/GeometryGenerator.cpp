#include "GeometryGenerator.h"
#include "MathHelper.h"

void GeometryGenerator::CreateBox(float width, float height, float depth, MeshData& meshData)
{
	// Create Vertex
	Vertex v[24];
	//std::vector<Vertex>v;
	//v.resize(24);

	float w2 = 0.5f * width;
	float h2 = 0.5f * height;
	float d2 = 0.5f * depth;

	// Fill in the front face vertex data.
	v[0] = { -w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };
	v[1] = { -w2, h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	v[2] = { w2, h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f };
	v[3] = { w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f };

	// Fill in the back face vertex data.
	v[4] = { -w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f };
	v[5] = { +w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f };
	v[6] = { +w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	v[7] = { -w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f };

	// Fill in the top face vertex data.
	v[8] = { -w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };
	v[9] = { -w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	v[10] = { +w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f };
	v[11] = { +w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f };

	// Fill in the bottom face vertex data.
	v[12] = { -w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f };
	v[13] = { +w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f };
	v[14] = { +w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	v[15] = { -w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f };

	// Fill in the left face vertex data.
	v[16] = { -w2, -h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f };
	v[17] = { -w2, +h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f };
	v[18] = { -w2, +h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f };
	v[19] = { -w2, -h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f };

	// Fill in the right face vertex data.
	v[20] = { +w2, -h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f };
	v[21] = { +w2, +h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f };
	v[22] = { +w2, +h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f };
	v[23] = { +w2, -h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f };

	meshData.Vertices.assign(&v[0], &v[24]);

	// Create the indices.
	UINT i[36] = {
		// front face
		0, 1, 2,
		0, 2, 3,
		// back face
		4, 5, 6,
		4, 6, 7,
		// top face
		8, 9, 10,
		8, 10, 11,
		// bottom face
		12, 13, 14,
		12, 14, 15,
		// left face
		16, 17, 18,
		16, 18, 19,
		// right face
		20, 21, 22,
		20, 22, 23
	};

	meshData.Indices.assign(&i[0], &i[36]);
}

void GeometryGenerator::CreateSphere(float radius, UINT sliceCount, UINT stackCount, MeshData& meshData)
{
	meshData.Vertices.clear();
	meshData.Indices.clear();

	// Compute the vertices stating at the top pole and moving down the stacks.

	// Poles: note that there will be texture coordinate distortion as there is
	// not a unique point on the texture map to assign to the pole when mapping
	// a rectangular texture onto a sphere.
	Vertex topVertex(0.0f, radius, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	Vertex bottomVertex(0.0f, -radius, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

	meshData.Vertices.push_back(topVertex);

	float phiStep = XM_PI / stackCount;
	float thetaStep = 2 * XM_PI / sliceCount;

	// Compute vertices for each stack ring (do not count the poles as rings).
	for (UINT i = 1; i < stackCount; ++i)
	{
		float phi = i * phiStep;
		// Vertices of ring.
		for (UINT j = 0; j <= sliceCount; ++j)
		{
			float theta = j * thetaStep;

			Vertex v;
			v.Position.x = radius * sinf(phi) * cosf(theta);
			v.Position.y = radius * cosf(phi);
			v.Position.z = radius * sinf(phi) * sinf(theta);

			// Partial derivative of P with respect to theta
			v.TangentU.x = -radius * sinf(phi) * sinf(theta);
			v.TangentU.y = 0;
			v.TangentU.z = radius * sinf(phi) * cosf(theta);

			XMVECTOR T = XMLoadFloat3(&v.TangentU);
			XMStoreFloat3(&v.TangentU, XMVector3Normalize(T));

			XMVECTOR p = XMLoadFloat3(&v.Position);
			XMStoreFloat3(&v.Normal, XMVector3Normalize(p));

			v.TexC.x = (float)j / sliceCount;
			v.TexC.y = (float)i / stackCount;

			meshData.Vertices.push_back(v);
		}
	}

	meshData.Vertices.push_back(bottomVertex);

	// Compute indices for top stack.  The top stack was written first to the vertex buffer
	// and connects the top pole to the first ring.
	for (UINT i = 1; i <= sliceCount; ++i)
	{
		meshData.Indices.push_back(0);
		meshData.Indices.push_back(i + 1);
		meshData.Indices.push_back(i);
	}

	// Compute indices for inner stacks (not connected to poles).

	// Offset the indices to the index of the first vertex in the first ring.
	// This is just skipping the top pole vertex.
	UINT baseIndex = 1;
	UINT ringVertexCount = sliceCount + 1;
	for (UINT i = 0; i < stackCount - 2; ++i)
	{
		for (UINT j = 0; j < sliceCount; ++j)
		{
			meshData.Indices.push_back(baseIndex + i*ringVertexCount + j);
			meshData.Indices.push_back(baseIndex + i*ringVertexCount + j+1);
			meshData.Indices.push_back(baseIndex + (i+1)*ringVertexCount + j);

			meshData.Indices.push_back(baseIndex + (i + 1)*ringVertexCount + j);
			meshData.Indices.push_back(baseIndex + i*ringVertexCount + j + 1);
			meshData.Indices.push_back(baseIndex + (i + 1)*ringVertexCount + j+1);
		}
	}

	// Compute indices for bottom stack.  The bottom stack was written last to the vertex buffer
	// and connects the bottom pole to the bottom ring.

	// South pole vertex was added last.
	UINT southPoleIndex = (UINT)meshData.Vertices.size() - 1;

	// Offset the indices to the index of the first vertex in the last ring.
	baseIndex = southPoleIndex - ringVertexCount;

	for (UINT i = 0; i < sliceCount; ++i)
	{
		meshData.Indices.push_back(baseIndex + i);
		meshData.Indices.push_back(baseIndex + i + 1);
		meshData.Indices.push_back(southPoleIndex);
	}
}

void GeometryGenerator::CreateFullscreenQuad(MeshData& meshData)
{
	meshData.Vertices.resize(4);
	meshData.Indices.resize(6);

	// Position coordinates specified in NDC space.
	meshData.Vertices[0] = Vertex(
		-1.f, -1.f, 0.f,
		0.f, 0.f, -1.f,
		1.f, 0.f, 0.f,
		0.f, 1.f);

	meshData.Vertices[1] = Vertex(
		-1.0f, +1.0f, 0.0f,
		0.0f, 0.0f, -1.0f,
		1.0f, 0.0f, 0.0f,
		0.0f, 0.0f);

	meshData.Vertices[2] = Vertex(
		+1.0f, +1.0f, 0.0f,
		0.0f, 0.0f, -1.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f);

	meshData.Vertices[3] = Vertex(
		+1.0f, -1.0f, 0.0f,
		0.0f, 0.0f, -1.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 1.0f);

	meshData.Indices[0] = 0;
	meshData.Indices[1] = 1;
	meshData.Indices[2] = 2;

	meshData.Indices[3] = 0;
	meshData.Indices[4] = 2;
	meshData.Indices[5] = 3;
}

void GeometryGenerator::Subdivide(MeshData& meshData)
{
	// Save a copy of the input geometry.
	MeshData inputCopy = meshData;

	meshData.Vertices.resize(0);
	meshData.Indices.resize(0);

	//       v1
	//       *
	//      / \
	//     /   \
	//  m0*-----*m1
	//   / \   / \
	//  /   \ /   \
	// *-----*-----*
	// v0    m2     v2

	UINT numTris = inputCopy.Indices.size() / 3;
	for (UINT i = 0; i < numTris; ++i)
	{
		Vertex v0 = inputCopy.Vertices[inputCopy.Indices[i * 3]];
		Vertex v1 = inputCopy.Vertices[inputCopy.Indices[i * 3 + 1]];
		Vertex v2 = inputCopy.Vertices[inputCopy.Indices[i * 3 + 2]];

		// Generate the midpoints.
		Vertex m0, m1, m2;

		// For subdivision, we just care about the position component.  We derive the other
		// vertex components in CreateGeosphere.
		m0.Position = XMFLOAT3(
			0.5f*(v0.Position.x + v1.Position.x),
			0.5f*(v0.Position.y + v1.Position.y),
			0.5f*(v0.Position.z + v1.Position.z));

		m1.Position = XMFLOAT3(
			0.5f*(v1.Position.x + v2.Position.x),
			0.5f*(v1.Position.y + v2.Position.y),
			0.5f*(v1.Position.z + v2.Position.z));

		m2.Position = XMFLOAT3(
			0.5f*(v0.Position.x + v2.Position.x),
			0.5f*(v0.Position.y + v2.Position.y),
			0.5f*(v0.Position.z + v2.Position.z));

		// Add new geometry.
		meshData.Vertices.push_back(v0); // 0
		meshData.Vertices.push_back(v1); // 1
		meshData.Vertices.push_back(v2); // 2
		meshData.Vertices.push_back(m0); // 3
		meshData.Vertices.push_back(m1); // 4
		meshData.Vertices.push_back(m2); // 5

		meshData.Indices.push_back(i * 6 + 0);
		meshData.Indices.push_back(i * 6 + 3);
		meshData.Indices.push_back(i * 6 + 5);

		meshData.Indices.push_back(i * 6 + 3);
		meshData.Indices.push_back(i * 6 + 4);
		meshData.Indices.push_back(i * 6 + 5);

		meshData.Indices.push_back(i * 6 + 5);
		meshData.Indices.push_back(i * 6 + 4);
		meshData.Indices.push_back(i * 6 + 2);

		meshData.Indices.push_back(i * 6 + 3);
		meshData.Indices.push_back(i * 6 + 1);
		meshData.Indices.push_back(i * 6 + 4);

	}
}

void GeometryGenerator::CreateGeosphere(float radius, UINT numSubdivisions, MeshData& meshData)
{
	// Put a cap on the number of subdivisions.
	numSubdivisions = MathHelper::Min(numSubdivisions, 5u);

	// Approximate a sphere by tessellating an icosahedron.

	const float X = 0.525731f;
	const float Z = 0.850651f;

	XMFLOAT3 pos[12] =
	{
		XMFLOAT3(-X, 0.0f, Z),  XMFLOAT3(X, 0.0f, Z),
		XMFLOAT3(-X, 0.0f, -Z), XMFLOAT3(X, 0.0f, -Z),
		XMFLOAT3(0.0f, Z, X),   XMFLOAT3(0.0f, Z, -X),
		XMFLOAT3(0.0f, -Z, X),  XMFLOAT3(0.0f, -Z, -X),
		XMFLOAT3(Z, X, 0.0f),   XMFLOAT3(-Z, X, 0.0f),
		XMFLOAT3(Z, -X, 0.0f),  XMFLOAT3(-Z, -X, 0.0f)
	};

	DWORD k[60] =
	{
		1,4,0,  4,9,0,  4,5,9,  8,5,4,  1,8,4,
		1,10,8, 10,3,8, 8,3,5,  3,2,5,  3,7,2,
		3,10,7, 10,6,7, 6,11,7, 6,0,11, 6,1,0,
		10,1,6, 11,0,9, 2,11,9, 5,2,9,  11,2,7
	};

	meshData.Vertices.resize(12);
	meshData.Indices.resize(60);

	for (UINT i = 0; i < 12; ++i)
		meshData.Vertices[i].Position = pos[i];

	for (UINT i = 0; i < 60; ++i)
		meshData.Indices[i] = k[i];

	for (UINT i = 0; i < numSubdivisions; ++i)
		Subdivide(meshData);

	// Project vertices onto sphere and scale.
	for (UINT i = 0; i < meshData.Vertices.size(); ++i)
	{
		// Project onto unit sphere.
		XMVECTOR n = XMVector3Normalize(XMLoadFloat3(&meshData.Vertices[i].Position));

		// Project onto sphere.
		XMVECTOR p = radius*n;

		XMStoreFloat3(&meshData.Vertices[i].Position, p);
		XMStoreFloat3(&meshData.Vertices[i].Normal, n);

		// Derive texture coordinates from spherical coordinates.
		float theta = MathHelper::AngleFromXY(
			meshData.Vertices[i].Position.x,
			meshData.Vertices[i].Position.z);

		float phi = acosf(meshData.Vertices[i].Position.y / radius);

		meshData.Vertices[i].TexC.x = theta / XM_2PI;
		meshData.Vertices[i].TexC.y = phi / XM_PI;

		// Partial derivative of P with respect to theta
		meshData.Vertices[i].TangentU.x = -radius*sinf(phi)*sinf(theta);
		meshData.Vertices[i].TangentU.y = 0.0f;
		meshData.Vertices[i].TangentU.z = +radius*sinf(phi)*cosf(theta);

		XMVECTOR T = XMLoadFloat3(&meshData.Vertices[i].TangentU);
		XMStoreFloat3(&meshData.Vertices[i].TangentU, XMVector3Normalize(T));
	}
}

void GeometryGenerator::CreateCylinder(float bottomRadius, float topRadius, float height, UINT sliceCount, UINT stackCount, MeshData& meshData)
{
	meshData.Vertices.clear();
	meshData.Indices.clear();

	// Build Stacks.
	float stack_height = height / stackCount;

	// Amount to increment radius as we move up each stack level from bottom to top.
	float radius_step = (topRadius - bottomRadius) / stackCount;

	UINT ring_count = stackCount + 1;

	float dtheta = XM_2PI / sliceCount;

	// Compute vertices for each stack ring starting at the bottom and moving up.
	for (UINT i = 0; i < ring_count; ++i)
	{
		float y = -0.5f * height + stack_height * i;
		float r = bottomRadius + radius_step * i;
		for (UINT j = 0; j < sliceCount + 1; ++j)
		{
			Vertex v;
			float c = cosf(j * dtheta);
			float s = sinf(j * dtheta);
			v.Position = XMFLOAT3(r * c, y, r * s);

			v.TexC.x = (float)j / sliceCount;
			v.TexC.y = 1.f - (float)i / stackCount;

			// This is unit length.
			v.TangentU = XMFLOAT3(-s, 0.f, c);

			float dr = bottomRadius - topRadius;
			XMFLOAT3 bitangent(dr*c, -height, dr*s);

			XMVECTOR T = XMLoadFloat3(&v.TangentU);
			XMVECTOR B = XMLoadFloat3(&bitangent);
			XMVECTOR N = XMVector3Normalize(XMVector3Cross(T, B));
			XMStoreFloat3(&v.Normal, N);

			meshData.Vertices.push_back(v);
		}
	}

	// Add one because we duplicate the first and last vertex per ring
	// since the texture coordinates are different.
	UINT ringVertexCount = sliceCount + 1;

	// Compute indices for each stack.
	for (UINT i = 0; i < stackCount; ++i)
	{
		for (UINT j = 0; j < sliceCount; ++j)
		{
			meshData.Indices.push_back(i*ringVertexCount + j);
			meshData.Indices.push_back((i + 1)*ringVertexCount + j);
			meshData.Indices.push_back((i + 1)*ringVertexCount + j + 1);

			meshData.Indices.push_back(i*ringVertexCount + j);
			meshData.Indices.push_back((i + 1)*ringVertexCount + j + 1);
			meshData.Indices.push_back(i*ringVertexCount + j + 1);
		}
	}

	BuildCylinderTopCap(bottomRadius, topRadius, height, sliceCount, stackCount, meshData);
	BuildCylinderBottomCap(bottomRadius, topRadius, height, sliceCount, stackCount, meshData);
}

void GeometryGenerator::BuildCylinderTopCap(float bottomRadius, float topRadius, float height, UINT sliceCount, UINT stackCount, MeshData& meshData)
{
	UINT base_idx = (UINT)meshData.Vertices.size();
	float y = 0.5f * height;
	float dtheta = XM_2PI / sliceCount;

	// Duplicate cap ring vertices because the texture coordinates and normals differ.
	for (UINT i = 0; i < sliceCount + 1; ++i)
	{
		float x = topRadius*cosf(i*dtheta);
		float z = topRadius*sinf(i*dtheta);

		// Scale down by the height to try and make top cap texture coord area
		// proportional to base.
		float u = x / height + 0.5f;
		float v = z / height + 0.5f;

		meshData.Vertices.push_back(Vertex(x, y, z, 0.f, 1.f, 0.f, 1.f, 0.f, 0.f, u, v));
	}

	// Cap center vertex.
	meshData.Vertices.push_back(Vertex(0.0f, y, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f));

	// Index of center vertex.
	UINT center_idx = (UINT)meshData.Vertices.size() - 1;

	for (UINT i = 0; i < sliceCount; ++i)
	{
		meshData.Indices.push_back(center_idx);
		meshData.Indices.push_back(base_idx + i + 1);
		meshData.Indices.push_back(base_idx + i);
	}
}

void GeometryGenerator::BuildCylinderBottomCap(float bottomRadius, float topRadius, float height, UINT sliceCount, UINT stackCount, MeshData& meshData)
{
	UINT base_idx = (UINT)meshData.Vertices.size();
	float y = -0.5f * height;
	float dtheta = XM_2PI / sliceCount;

	// Duplicate cap ring vertices because the texture coordinates and normals differ.
	for (UINT i = 0; i < sliceCount + 1; ++i)
	{
		float x = bottomRadius*cosf(i*dtheta);
		float z = bottomRadius*sinf(i*dtheta);

		// Scale down by the height to try and make top cap texture coord area
		// proportional to base.
		float u = x / height + 0.5f;
		float v = z / height + 0.5f;

		meshData.Vertices.push_back(Vertex(x, y, z, 0.f, -1.f, 0.f, 1.f, 0.f, 0.f, u, v));
	}

	// Cap center vertex.
	meshData.Vertices.push_back(Vertex(0.0f, y, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f));
	
	// Index of center vertex.
	UINT center_idx = (UINT)meshData.Vertices.size() - 1;

	for (UINT i = 0; i < sliceCount; ++i)
	{
		meshData.Indices.push_back(center_idx);
		meshData.Indices.push_back(base_idx + i);
		meshData.Indices.push_back(base_idx + i + 1);
		
	}
}


void GeometryGenerator::CreateGrid(float width, float depth, UINT m, UINT n, MeshData& meshData)
{
	UINT vertex_count = m * n;
	UINT face_count = (m - 1) * (n - 1) * 2;

	// Create the vertices.
	float dx = width / (n - 1);
	float dz = depth / (m - 1);

	float du = 1.0f / (n - 1);
	float dv = 1.0f / (m - 1);

	meshData.Vertices.resize(vertex_count);

	for (UINT z = 0; z < m; ++z)
	{
		float current_depth = depth / 2 - z * dz;
		for (UINT x = 0; x < n; ++x)
		{
			float current_width = -width / 2 + x * dx;
			meshData.Vertices[z * n + x].Position = XMFLOAT3(current_width, 0.0f, current_depth);
			meshData.Vertices[z * n + x].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
			meshData.Vertices[z * n + x].TangentU = XMFLOAT3(1.0f, 0.0f, 0.0f);
			meshData.Vertices[z * n + x].TexC.x = (float)x / n;
			meshData.Vertices[z * n + x].TexC.y = (float)z / m;
		}
	}

	// Create the indices.
	meshData.Indices.resize(face_count * 3);
	UINT current_index = 0;

	for (UINT z = 0; z < m - 1; ++z)
	{
		for (UINT x = 0; x < n - 1; ++x)
		{
			meshData.Indices[current_index] = z * n + x;
			meshData.Indices[current_index + 1] = z * n + x + 1;
			meshData.Indices[current_index + 2] = (z + 1) * n+ x;

			meshData.Indices[current_index + 3] = (z + 1) * n + x;
			meshData.Indices[current_index + 4] = z * n + x + 1;
			meshData.Indices[current_index + 5] = (z + 1) * n + x + 1;

			current_index += 6;
		}
	}
}