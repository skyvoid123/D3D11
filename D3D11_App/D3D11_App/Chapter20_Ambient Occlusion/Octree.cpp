#include "Octree.h"

Octree::Octree()
	: m_RootNode(nullptr)
{

}

Octree::~Octree()
{
	SafeDelete(m_RootNode);
}

void Octree::Build(const std::vector<XMFLOAT3>& vertices, const std::vector<UINT>& indices)
{
	// Cache a copy of the vertices.
	m_Vertices = vertices;

	// Build AABB to contain the scene mesh.
	Box sceneBounds = BuildAABB();

	m_RootNode = new OctreeNode();
	m_RootNode->Bounds = sceneBounds;

	BuildOctree(m_RootNode, indices);
}

Box Octree::BuildAABB()
{
	XMVECTOR vmin = XMVectorReplicate(+MathHelper::Infinity);
	XMVECTOR vmax = XMVectorReplicate(-MathHelper::Infinity);

	for (int i = 0; i < m_Vertices.size(); ++i)
	{
		XMVECTOR p = XMLoadFloat3(&m_Vertices[i]);
		vmin = XMVectorMin(vmin, p);
		vmax = XMVectorMax(vmax, p);
	}

	Box bounds;
	XMVECTOR c = 0.5f * (vmin + vmax);
	XMVECTOR e = 0.5f * (vmax - vmin);

	XMStoreFloat3(&bounds.center, c);
	XMStoreFloat3(&bounds.extent, e);

	return bounds;
}

void Octree::BuildOctree(OctreeNode* parent, const std::vector<UINT>& indices)
{
	int triCount = indices.size() / 3;

	if (triCount < 60)
	{
		parent->IsLeaf = true;
		parent->Indices = indices;
	}
	else
	{
		parent->IsLeaf = false;
		Box subbox[8];
		parent->Subdivide(subbox);

		for (int i = 0; i < 8; ++i)
		{
			// Allocate a new subnode.
			parent->Children[i] = new OctreeNode();
			parent->Children[i]->Bounds = subbox[i];

			// Find triangles that intersect this node's bounding box.
			std::vector<UINT> intersectedTriIndices;
			for (int j = 0; j < triCount; ++j)
			{
				UINT i0 = indices[j * 3 + 0];
				UINT i1 = indices[j * 3 + 1];
				UINT i2 = indices[j * 3 + 2];

				XMVECTOR v0 = XMLoadFloat3(&m_Vertices[i0]);
				XMVECTOR v1 = XMLoadFloat3(&m_Vertices[i1]);
				XMVECTOR v2 = XMLoadFloat3(&m_Vertices[i2]);

				if (subbox[i].IsIntersectTriangle(v0, v1, v2))
				{
					intersectedTriIndices.push_back(i0);
					intersectedTriIndices.push_back(i1);
					intersectedTriIndices.push_back(i2);
				}
			}

			// Recurse.
			BuildOctree(parent->Children[i], intersectedTriIndices);
		}
	}
}

bool Octree::RayOctreeIntersect(FXMVECTOR rayPos, FXMVECTOR rayDir)
{
	return RayOctreeIntersect(m_RootNode, rayPos, rayDir);
}

bool Octree::RayOctreeIntersect(OctreeNode* parent, FXMVECTOR rayPos, FXMVECTOR rayDir)
{
	Ray ray(rayPos, rayDir);

	// Recurs until we find a leaf node (all the triangles are in the leaves).
	if (parent->IsLeaf)
	{
		int triCount = parent->Indices.size() / 3;
		for (int i = 0; i < triCount; ++i)
		{
			UINT i0 = parent->Indices[i * 3 + 0];
			UINT i1 = parent->Indices[i * 3 + 1];
			UINT i2 = parent->Indices[i * 3 + 2];

			XMVECTOR v0 = XMLoadFloat3(&m_Vertices[i0]);
			XMVECTOR v1 = XMLoadFloat3(&m_Vertices[i1]);
			XMVECTOR v2 = XMLoadFloat3(&m_Vertices[i2]);

			if (ray.IsIntersectTriangle(v0, v1, v2, nullptr))
			{
				return true;
			}
		}
		return false;
	}
	else
	{
		for (int i = 0; i < 8; ++i)
		{
			// Recurse down this node if the ray hit the child's box.
			if (ray.IsIntersectBox(parent->Children[i]->Bounds, nullptr))
			{
				if (RayOctreeIntersect(parent->Children[i], rayPos, rayDir))
				{
					return true;
				}
			}
		}

		// If we get here. then we did not hit any triangles.
		return false;
	}
}