#pragma once
#include "d3dUtil.h"


struct OctreeNode
{
	Box Bounds;

	// This will be empty except for leaf nodes.
	std::vector<UINT> Indices;

	OctreeNode* Children[8];

	bool IsLeaf;

	OctreeNode()
	{
		for (int i = 0; i < 8; ++i)
		{
			Children[i] = nullptr;
		}

		Bounds.center = XMFLOAT3(0.0f, 0.0f, 0.0f);
		Bounds.extent = XMFLOAT3(0.0f, 0.0f, 0.0f);

		IsLeaf = false;
	}

	~OctreeNode()
	{
		for (int i = 0; i < 8; ++i)
		{
			SafeDelete(Children[i]);
		}
	}

	/// Subdivides the bounding box of this node into eight subboxes (vMin[i], vMax[i]) for i = 0:7.
	void Subdivide(Box box[8])
	{
		XMFLOAT3 halfExtent(
			0.5f * Bounds.extent.x,
			0.5f * Bounds.extent.y,
			0.5f * Bounds.extent.z);

		// "Top" four quadrants.
		box[0].center = XMFLOAT3(
			Bounds.center.x + halfExtent.x,
			Bounds.center.y + halfExtent.y,
			Bounds.center.z + halfExtent.z);
		box[0].extent = halfExtent;

		box[1].center = XMFLOAT3(
			Bounds.center.x - halfExtent.x,
			Bounds.center.y + halfExtent.y,
			Bounds.center.z + halfExtent.z);
		box[1].extent = halfExtent;

		box[2].center = XMFLOAT3(
			Bounds.center.x - halfExtent.x,
			Bounds.center.y + halfExtent.y,
			Bounds.center.z - halfExtent.z);
		box[2].extent = halfExtent;

		box[3].center = XMFLOAT3(
			Bounds.center.x + halfExtent.x,
			Bounds.center.y + halfExtent.y,
			Bounds.center.z - halfExtent.z);
		box[3].extent = halfExtent;

		// "Bottom" four quadrants.
		box[4].center = XMFLOAT3(
			Bounds.center.x + halfExtent.x,
			Bounds.center.y - halfExtent.y,
			Bounds.center.z + halfExtent.z);
		box[4].extent = halfExtent;

		box[5].center = XMFLOAT3(
			Bounds.center.x - halfExtent.x,
			Bounds.center.y - halfExtent.y,
			Bounds.center.z + halfExtent.z);
		box[5].extent = halfExtent;

		box[6].center = XMFLOAT3(
			Bounds.center.x - halfExtent.x,
			Bounds.center.y - halfExtent.y,
			Bounds.center.z - halfExtent.z);
		box[6].extent = halfExtent;

		box[7].center = XMFLOAT3(
			Bounds.center.x + halfExtent.x,
			Bounds.center.y - halfExtent.y,
			Bounds.center.z - halfExtent.z);
		box[7].extent = halfExtent;
	}
};

class Octree
{
public:
	Octree();
	~Octree();

	void Build(const std::vector<XMFLOAT3>& vertices, const std::vector<UINT>& indices);
	bool RayOctreeIntersect(FXMVECTOR rayPos, FXMVECTOR rayDir);

private:
	Box BuildAABB();
	void BuildOctree(OctreeNode* parent, const std::vector<UINT>& indices);
	bool RayOctreeIntersect(OctreeNode* parent, FXMVECTOR rayPos, FXMVECTOR rayDir);

private:
	OctreeNode* m_RootNode;
	std::vector<XMFLOAT3> m_Vertices;
};

