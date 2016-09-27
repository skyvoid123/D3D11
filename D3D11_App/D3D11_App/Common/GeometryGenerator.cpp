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