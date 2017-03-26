#include "d3dUtil.h"

using namespace DirectX;

//-----------------------------------------------------------------------------
// Compute the intersection of a ray (origin, direction) with an axis aligned 
// box using the slabs method.
//-----------------------------------------------------------------------------
bool Ray::IsIntersectBox(const Box& box, float* pDist)
{
	static const float Epsilon = 1e-6f;
	float tmin = 0.0f;
	float tmax = FLT_MAX;

	//The plane perpendicular to x-axie 
	if (abs(direction.x) < Epsilon)	//If the ray parallel to the plane 
	{
		//If the ray is not within AABB box, then not intersecting  
		if (origin.x < XMVectorGetX(box.GetMinV()) || origin.x > XMVectorGetX(box.GetMaxV()))
		{
			return false;
		}
	}
	else
	{
		//Compute the distance of ray to the near plane and far plane 
		float t1 = (XMVectorGetX(box.GetMinV()) - origin.x) / direction.x;
		float t2 = (XMVectorGetX(box.GetMaxV()) - origin.x) / direction.x;
		
		if (t1 > t2)
		{
			std::swap(t1, t2);
		}
		tmin = max(tmin, t1);
		tmax = min(tmax, t2);

		if (tmin > tmax)
		{
			return false;
		}
	}

	if (abs(direction.y) < Epsilon)	//If the ray parallel to the plane 
	{
		//If the ray is not within AABB box, then not intersecting  
		if (origin.y < XMVectorGetY(box.GetMinV()) || origin.y > XMVectorGetY(box.GetMaxV()))
		{
			return false;
		}
	}
	else
	{
		//Compute the distance of ray to the near plane and far plane 
		float t1 = (XMVectorGetY(box.GetMinV()) - origin.y) / direction.y;
		float t2 = (XMVectorGetY(box.GetMaxV()) - origin.y) / direction.y;

		if (t1 > t2)
		{
			std::swap(t1, t2);
		}
		tmin = max(tmin, t1);
		tmax = min(tmax, t2);

		if (tmin > tmax)
		{
			return false;
		}
	}

	if (abs(direction.z) < Epsilon)	//If the ray parallel to the plane 
	{
		//If the ray is not within AABB box, then not intersecting  
		if (origin.z < XMVectorGetZ(box.GetMinV()) || origin.z > XMVectorGetZ(box.GetMaxV()))
		{
			return false;
		}
	}
	else
	{
		//Compute the distance of ray to the near plane and far plane 
		float t1 = (XMVectorGetZ(box.GetMinV()) - origin.z) / direction.z;
		float t2 = (XMVectorGetZ(box.GetMaxV()) - origin.z) / direction.z;

		if (t1 > t2)
		{
			std::swap(t1, t2);
		}
		tmin = max(tmin, t1);
		tmax = min(tmax, t2);

		if (tmin > tmax)
		{
			return false;
		}
	}
	
	if (pDist)
	{
		*pDist = tmin;
	}
	
	return true;

	/*static const XMVECTOR Epsilon =
	{
		1e-20f, 1e-20f, 1e-20f, 1e-20f
	};
	static const XMVECTOR FltMin =
	{
		-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX
	};
	static const XMVECTOR FltMax =
	{
		FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX
	};

	// Load the box.
	XMVECTOR Center = XMLoadFloat3(&box.center);
	XMVECTOR Extents = XMLoadFloat3(&box.extent);

	// Load the ray.
	XMVECTOR origin = XMLoadFloat3(&origin);
	XMVECTOR direction = XMLoadFloat3(&direction);

	// Adjust ray origin to be relative to center of the box.
	XMVECTOR TOrigin = Center - origin;

	// Compute the dot product against each axis of the box.
	// Since the axis are (1,0,0), (0,1,0), (0,0,1) no computation is necessary.
	XMVECTOR AxisDotOrigin = TOrigin;
	XMVECTOR AxisDotDirection = direction;

	// if (fabs(AxisDotDirection) <= Epsilon) the ray is nearly parallel to the slab.
	XMVECTOR IsParallel = XMVectorLessOrEqual(XMVectorAbs(AxisDotDirection), Epsilon);

	// Test against all three axis simultaneously.
	XMVECTOR InverseAxisDotDirection = XMVectorReciprocal(AxisDotDirection);
	XMVECTOR t1 = (AxisDotOrigin - Extents) * InverseAxisDotDirection;
	XMVECTOR t2 = (AxisDotOrigin + Extents) * InverseAxisDotDirection;

	// Compute the max of min(t1,t2) and the min of max(t1,t2) ensuring we don't
	// use the results from any directions parallel to the slab.
	XMVECTOR t_min = XMVectorSelect(XMVectorMin(t1, t2), FltMin, IsParallel);
	XMVECTOR t_max = XMVectorSelect(XMVectorMax(t1, t2), FltMax, IsParallel);

	// t_min.x = maximum( t_min.x, t_min.y, t_min.z );
	// t_max.x = minimum( t_max.x, t_max.y, t_max.z );
	t_min = XMVectorMax(t_min, XMVectorSplatY(t_min));  // x = max(x,y)
	t_min = XMVectorMax(t_min, XMVectorSplatZ(t_min));  // x = max(max(x,y),z)
	t_max = XMVectorMin(t_max, XMVectorSplatY(t_max));  // x = min(x,y)
	t_max = XMVectorMin(t_max, XMVectorSplatZ(t_max));  // x = min(min(x,y),z)

	// if ( t_min > t_max ) return FALSE;
	XMVECTOR NoIntersection = XMVectorGreater(XMVectorSplatX(t_min), XMVectorSplatX(t_max));

	// if ( t_max < 0.0f ) return FALSE;
	NoIntersection = XMVectorOrInt(NoIntersection, XMVectorLess(XMVectorSplatX(t_max), XMVectorZero()));

	// if (IsParallel && (-Extents > AxisDotOrigin || Extents < AxisDotOrigin)) return FALSE;
	XMVECTOR ParallelOverlap = XMVectorInBounds(AxisDotOrigin, Extents);
	NoIntersection = XMVectorOrInt(NoIntersection, XMVectorAndCInt(IsParallel, ParallelOverlap));

	if (!XMVector3AnyTrue(NoIntersection))
	{
		// Store the x-component to *pDist
		XMStoreFloat(pDist, t_min);
		return true;
	}

	return false;*/
}

bool Ray::IsIntersectTriangle(FXMVECTOR v0, CXMVECTOR v1, CXMVECTOR v2, float * pDist)
{
	static const XMVECTOR Epsilon =
	{
		1e-20f, 1e-20f, 1e-20f, 1e-20f
	};

	XMVECTOR Zero = XMVectorZero();

	XMVECTOR e1 = v1 - v0;
	XMVECTOR e2 = v2 - v0;

	// p = direction ^ e2;
	XMVECTOR p = XMVector3Cross(XMLoadFloat3(&direction), e2);

	// det = e1 * p;
	XMVECTOR det = XMVector3Dot(e1, p);

	XMVECTOR u, v, t;

	if (XMVector3GreaterOrEqual(det, Epsilon))
	{
		// Determinate is positive (front side of the triangle).
		XMVECTOR s = XMLoadFloat3(&origin) - v0;

		// u = s * p;
		u = XMVector3Dot(s, p);

		XMVECTOR NoIntersection = XMVectorLess(u, Zero);
		NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(u, det));

		// q = s ^ e1;
		XMVECTOR q = XMVector3Cross(s, e1);

		// v = direction * q;
		v = XMVector3Dot(XMLoadFloat3(&direction), q);

		NoIntersection = XMVectorOrInt(NoIntersection, XMVectorLess(v, Zero));
		NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(u + v, det));

		// t = e2 * q;
		t = XMVector3Dot(e2, q);

		NoIntersection = XMVectorOrInt(NoIntersection, XMVectorLess(t, Zero));

		if (XMVector4EqualInt(NoIntersection, XMVectorTrueInt()))
			return false;
	}
	else if (XMVector3LessOrEqual(det, -Epsilon))
	{
		// Determinate is negative (back side of the triangle).
		XMVECTOR s = XMLoadFloat3(&origin) - v0;

		// u = s * p;
		u = XMVector3Dot(s, p);

		XMVECTOR NoIntersection = XMVectorGreater(u, Zero);
		NoIntersection = XMVectorOrInt(NoIntersection, XMVectorLess(u, det));

		// q = s ^ e1;
		XMVECTOR q = XMVector3Cross(s, e1);

		// v = direction * q;
		v = XMVector3Dot(XMLoadFloat3(&direction), q);

		NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(v, Zero));
		NoIntersection = XMVectorOrInt(NoIntersection, XMVectorLess(u + v, det));

		// t = e2 * q;
		t = XMVector3Dot(e2, q);

		NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(t, Zero));

		if (XMVector4EqualInt(NoIntersection, XMVectorTrueInt()))
			return false;
	}
	else
	{
		// Parallel ray.
		return false;
	}

	XMVECTOR inv_det = XMVectorReciprocal(det);

	t *= inv_det;

	// u * inv_det and v * inv_det are the barycentric cooridinates of the intersection.

	// Store the x-component to *pDist
	if (pDist)
	{
		XMStoreFloat(pDist, t);
	}
	
	return true;
}

void ExtractFrustumPlanes(XMFLOAT4 planes[6], CXMMATRIX T)
{
	XMFLOAT4X4 M;
	XMStoreFloat4x4(&M, T);

	//
	// Left
	//
	planes[0].x = M(0, 3) + M(0, 0);
	planes[0].y = M(1, 3) + M(1, 0);
	planes[0].z = M(2, 3) + M(2, 0);
	planes[0].w = M(3, 3) + M(3, 0);

	//
	// Right
	//
	planes[1].x = M(0, 3) - M(0, 0);
	planes[1].y = M(1, 3) - M(1, 0);
	planes[1].z = M(2, 3) - M(2, 0);
	planes[1].w = M(3, 3) - M(3, 0);

	//
	// Bottom
	//
	planes[2].x = M(0, 3) + M(0, 1);
	planes[2].y = M(1, 3) + M(1, 1);
	planes[2].z = M(2, 3) + M(2, 1);
	planes[2].w = M(3, 3) + M(3, 1);

	//
	// Top
	//
	planes[3].x = M(0, 3) - M(0, 1);
	planes[3].y = M(1, 3) - M(1, 1);
	planes[3].z = M(2, 3) - M(2, 1);
	planes[3].w = M(3, 3) - M(3, 1);

	//
	// Near
	//
	planes[4].x = M(0, 2);
	planes[4].y = M(1, 2);
	planes[4].z = M(2, 2);
	planes[4].w = M(3, 2);

	//
	// Far
	//
	planes[5].x = M(0, 3) - M(0, 2);
	planes[5].y = M(1, 3) - M(1, 2);
	planes[5].z = M(2, 3) - M(2, 2);
	planes[5].w = M(3, 3) - M(3, 2);

	// Normalize the plane equations.
	for (int i = 0; i < 6; ++i)
	{
		XMVECTOR v = XMPlaneNormalize(XMLoadFloat4(&planes[i]));
		XMStoreFloat4(&planes[i], v);
	}
}
