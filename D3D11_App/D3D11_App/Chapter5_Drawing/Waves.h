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

	// Returns the solution at the ith grid point.
	const XMFLOAT3& operator[] (int i)
	{
		return curr_solution_[i];
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
};