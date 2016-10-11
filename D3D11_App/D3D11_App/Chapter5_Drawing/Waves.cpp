#include "Waves.h"
#include <algorithm>
#include <vector>
#include <cassert>

Waves::Waves()
	: row_count_(0)
	, col_count_(0)
	, vertex_count_(0)
	, tri_count_(0)
	, k1_(0.f)
	, k2_(0.f)
	, k3_(0.f)
	, time_delta_(0.f)
	, time_step_(0.f)
	, spatial_step_(0.f)
	, prev_solution_(nullptr)
	, curr_solution_(nullptr)
{

}

Waves::~Waves()
{
	delete[] prev_solution_;
	delete[] curr_solution_;
}

UINT Waves::RowCount() const
{
	return row_count_;
}

UINT Waves::ColumnCount() const
{
	return col_count_;
}

UINT Waves::VertexCount() const
{
	return vertex_count_;
}

UINT Waves::TriangleCount() const
{
	return tri_count_;
}

void Waves::Init(UINT rows, UINT cols, float dx, float dt, float speed, float damping)
{
	row_count_ = rows;
	col_count_ = cols;

	vertex_count_ = rows * cols;
	tri_count_ = (rows - 1) * (cols - 1) * 2;

	time_step_ = dt;
	spatial_step_ = dx;

	float d = damping * dt + 2.f;
	float e = (speed * speed) * (dt * dt) / (dx * dx);
	k1_ = (damping * dt - 2.f) / d;
	k2_ = (4.f - 8.f * e) / d;
	k3_ = (2.f * e) / d;

	delete[] prev_solution_;
	delete[] curr_solution_;

	prev_solution_ = new XMFLOAT3[vertex_count_];
	curr_solution_ = new XMFLOAT3[vertex_count_];

	// Generate grid vertices in system memory.
	float width_half = (cols - 1) * dx * .5f;
	float depth_half = (rows - 1) * dx * .5f;
	for (UINT i = 0; i < rows; ++i)
	{
		float z = depth_half - i * dx;
		for (UINT j = 0; j < cols; ++j)
		{
			float x = -width_half + j * dx;
			prev_solution_[i*cols + j] = XMFLOAT3(x, 0.0f, z);
			curr_solution_[i*cols + j] = XMFLOAT3(x, 0.0f, z);
		}
	}
}

void Waves::Update(float dt)
{
	// Accumulate time.
	time_delta_ += dt;

	// Only update the simulation at the specified time step.
	if (time_delta_ >= time_step_)
	{
		// Only update interior points; we use zero boundary conditions.
		for (int i = 1; i < row_count_ - 1; ++i)
		{
			for (int j = 1; j < col_count_ - 1; ++j)
			{
				// After this update we will be discarding the old previous
				// buffer, so overwrite that buffer with the new update.
				// Note how we can do this inplace (read/write to same element) 
				// because we won't need prev_ij again and the assignment happens last.

				// Note j indexes x and i indexes z: h(x_j, z_i, t_k)
				// Moreover, our +z axis goes "down"; this is just to 
				// keep consistent with our row indices going down.
				prev_solution_[i*col_count_ + j].y =
					k1_ * prev_solution_[i*col_count_ + j].y +
					k2_ * curr_solution_[i*col_count_ + j].y +
					k3_ * (curr_solution_[(i + 1)*col_count_ + j].y +
						curr_solution_[(i - 1)*col_count_ + j].y +
						curr_solution_[i*col_count_ + j + 1].y +
						curr_solution_[i*col_count_ + j - 1].y);
			}
		}

		// We just overwrote the previous buffer with the new data, so
		// this data needs to become the current solution and the old
		// current solution becomes the new previous solution.
		std::swap(prev_solution_, curr_solution_);

		time_delta_ = 0.f;	// reset time;
	}
}

void Waves::Disturb(UINT rowth, UINT colth, float magnitude)
{
	// Don't disturb boundaries.
	assert(rowth > 1 && rowth < row_count_ - 2);
	assert(colth > 1 && colth < col_count_ - 2);

	float mag_half = 0.5f * magnitude;

	// Disturb the ijth vertex height and its neighbors.
	curr_solution_[rowth*col_count_ + colth].y += magnitude;
	curr_solution_[rowth*col_count_ + colth + 1].y += mag_half;
	curr_solution_[rowth*col_count_ + colth - 1].y += mag_half;
	curr_solution_[(rowth + 1)*col_count_ + colth].y += mag_half;
	curr_solution_[(rowth - 1)*col_count_ + colth].y += mag_half;
}