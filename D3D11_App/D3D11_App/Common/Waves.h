#pragma once

#include <windows.h>
#include <DirectXMath.h>

using namespace DirectX;

class Waves
{
public:
	Waves();
	~Waves();

	UINT RowCount() const;
	UINT ColumnCount() const;
	UINT VertexCount() const;
	UINT TriangleCount() const;
	float Width() const;
	float Depth() const;

	// Returns the solution at the ith grid point.
	const XMFLOAT3& operator[] (int i) const
	{
		return curr_solution_[i];
	}

	// Returns the solution normal at the ith grid point.
	const XMFLOAT3& Normal(int i) const
	{
		return normals_[i];
	}

	// Returns the unit tangent vector at the ith grid point in the local x-axis direction.
	const XMFLOAT3& TangentX(int i) const
	{
		return tangentX_[i];
	}

	void Init(UINT rows, UINT cols, float dx, float dt, float speed, float damping);
	void Update(float dt);
	void Disturb(UINT rowth, UINT colth, float magnitude);

private:
	UINT row_count_;
	UINT col_count_;
	UINT vertex_count_;
	UINT tri_count_;

	// Simulation constants we can precompute.
	float k1_;
	float k2_;
	float k3_;

	float time_delta_;
	float time_step_;
	float spatial_step_;

	XMFLOAT3* prev_solution_;
	XMFLOAT3* curr_solution_;
	XMFLOAT3* normals_;
	XMFLOAT3* tangentX_;
};