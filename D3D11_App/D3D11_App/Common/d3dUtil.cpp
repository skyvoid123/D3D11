#include "d3dUtil.h"

using namespace DirectX;

bool Box::IsIntersectTriangle(FXMVECTOR v0, CXMVECTOR v1, CXMVECTOR v2)
{
	XMVECTOR Zero = XMVectorZero();
	
	// Load the box.
	XMVECTOR Center = XMLoadFloat3(&center);
	XMVECTOR Extents = XMLoadFloat3(&extent);

	XMVECTOR BoxMin = Center - Extents;
	XMVECTOR BoxMax = Center + Extents;

	// Test the axes of the box (in effect test the AAB against the minimal AAB 
	// around the triangle).
	XMVECTOR TriMin = XMVectorMin(XMVectorMin(v0, v1), v2);
	XMVECTOR TriMax = XMVectorMax(XMVectorMax(v0, v1), v2);

	// for each i in (x, y, z) if a_min(i) > b_max(i) or b_min(i) > a_max(i) then disjoint
	XMVECTOR Disjoint = XMVectorOrInt(XMVectorGreater(TriMin, BoxMax), XMVectorGreater(BoxMin, TriMax));
	if (XMVector3NotEqualInt(Disjoint, Zero))
		return false;

	// Test the plane of the triangle.
	XMVECTOR Normal = XMVector3Cross(v1 - v0, v2 - v0);
	XMVECTOR Dist = XMVector3Dot(Normal, v0);

	// Assert that the triangle is not degenerate.
	assert(!XMVector3Equal(Normal, Zero));

	// for each i in (x, y, z) if n(i) >= 0 then v_min(i)=b_min(i), v_max(i)=b_max(i)
	// else v_min(i)=b_max(i), v_max(i)=b_min(i)
	XMVECTOR NormalSelect = XMVectorGreater(Normal, Zero);
	XMVECTOR V_Min = XMVectorSelect(BoxMax, BoxMin, NormalSelect);
	XMVECTOR V_Max = XMVectorSelect(BoxMin, BoxMax, NormalSelect);

	// if n dot v_min + d > 0 || n dot v_max + d < 0 then disjoint
	XMVECTOR MinDist = XMVector3Dot(V_Min, Normal);
	XMVECTOR MaxDist = XMVector3Dot(V_Max, Normal);

	XMVECTOR NoIntersection = XMVectorGreater(MinDist, Dist);
	NoIntersection = XMVectorOrInt(NoIntersection, XMVectorLess(MaxDist, Dist));

	// Move the box center to zero to simplify the following tests.
	XMVECTOR TV0 = v0 - Center;
	XMVECTOR TV1 = v1 - Center;
	XMVECTOR TV2 = v2 - Center;

	// Test the edge/edge axes (3*3).
	XMVECTOR e0 = TV1 - TV0;
	XMVECTOR e1 = TV2 - TV1;
	XMVECTOR e2 = TV0 - TV2;

	// Make w zero.
	e0 = XMVectorInsert(e0, Zero, 0, 0, 0, 0, 1);
	e1 = XMVectorInsert(e1, Zero, 0, 0, 0, 0, 1);
	e2 = XMVectorInsert(e2, Zero, 0, 0, 0, 0, 1);

	XMVECTOR Axis;
	XMVECTOR p0, p1, p2;
	XMVECTOR Min, Max;
	XMVECTOR Radius;

	// Axis == (1,0,0) x e0 = (0, -e0.z, e0.y)
	Axis = XMVectorPermute<XM_PERMUTE_0W, XM_PERMUTE_1Z, XM_PERMUTE_0Y, XM_PERMUTE_0X>(e0, -e0);
	p0 = XMVector3Dot(TV0, Axis);
	// p1 = XMVector3Dot( v1, Axis ); // p1 = p0;
	p2 = XMVector3Dot(TV2, Axis);
	Min = XMVectorMin(p0, p2);
	Max = XMVectorMax(p0, p2);
	Radius = XMVector3Dot(Extents, XMVectorAbs(Axis));
	NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(Min, Radius));
	NoIntersection = XMVectorOrInt(NoIntersection, XMVectorLess(Max, -Radius));

	// Axis == (1,0,0) x e1 = (0, -e1.z, e1.y)
	Axis = XMVectorPermute<XM_PERMUTE_0W, XM_PERMUTE_1Z, XM_PERMUTE_0Y, XM_PERMUTE_0X>(e1, -e1);
	p0 = XMVector3Dot(TV0, Axis);
	p1 = XMVector3Dot(TV1, Axis);
	// p2 = XMVector3Dot( v2, Axis ); // p2 = p1;
	Min = XMVectorMin(p0, p1);
	Max = XMVectorMax(p0, p1);
	Radius = XMVector3Dot(Extents, XMVectorAbs(Axis));
	NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(Min, Radius));
	NoIntersection = XMVectorOrInt(NoIntersection, XMVectorLess(Max, -Radius));

	// Axis == (1,0,0) x e2 = (0, -e2.z, e2.y)
	Axis = XMVectorPermute<XM_PERMUTE_0W, XM_PERMUTE_1Z, XM_PERMUTE_0Y, XM_PERMUTE_0X>(e2, -e2);
	p0 = XMVector3Dot(TV0, Axis);
	p1 = XMVector3Dot(TV1, Axis);
	// p2 = XMVector3Dot( v2, Axis ); // p2 = p0;
	Min = XMVectorMin(p0, p1);
	Max = XMVectorMax(p0, p1);
	Radius = XMVector3Dot(Extents, XMVectorAbs(Axis));
	NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(Min, Radius));
	NoIntersection = XMVectorOrInt(NoIntersection, XMVectorLess(Max, -Radius));

	// Axis == (0,1,0) x e0 = (e0.z, 0, -e0.x)
	Axis = XMVectorPermute<XM_PERMUTE_0Z, XM_PERMUTE_0W, XM_PERMUTE_1X, XM_PERMUTE_0Y>(e0, -e0);
	p0 = XMVector3Dot(TV0, Axis);
	// p1 = XMVector3Dot( v1, Axis ); // p1 = p0;
	p2 = XMVector3Dot(TV2, Axis);
	Min = XMVectorMin(p0, p2);
	Max = XMVectorMax(p0, p2);
	Radius = XMVector3Dot(Extents, XMVectorAbs(Axis));
	NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(Min, Radius));
	NoIntersection = XMVectorOrInt(NoIntersection, XMVectorLess(Max, -Radius));

	// Axis == (0,1,0) x e1 = (e1.z, 0, -e1.x)
	Axis = XMVectorPermute<XM_PERMUTE_0Z, XM_PERMUTE_0W, XM_PERMUTE_1X, XM_PERMUTE_0Y>(e1, -e1);
	p0 = XMVector3Dot(TV0, Axis);
	p1 = XMVector3Dot(TV1, Axis);
	// p2 = XMVector3Dot( v2, Axis ); // p2 = p1;
	Min = XMVectorMin(p0, p1);
	Max = XMVectorMax(p0, p1);
	Radius = XMVector3Dot(Extents, XMVectorAbs(Axis));
	NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(Min, Radius));
	NoIntersection = XMVectorOrInt(NoIntersection, XMVectorLess(Max, -Radius));

	// Axis == (0,1,0) x e2 = (e2.z, 0, -e2.x)
	Axis = XMVectorPermute<XM_PERMUTE_0Z, XM_PERMUTE_0W, XM_PERMUTE_1X, XM_PERMUTE_0Y>(e2, -e2);
	p0 = XMVector3Dot(TV0, Axis);
	p1 = XMVector3Dot(TV1, Axis);
	// p2 = XMVector3Dot( v2, Axis ); // p2 = p0;
	Min = XMVectorMin(p0, p1);
	Max = XMVectorMax(p0, p1);
	Radius = XMVector3Dot(Extents, XMVectorAbs(Axis));
	NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(Min, Radius));
	NoIntersection = XMVectorOrInt(NoIntersection, XMVectorLess(Max, -Radius));

	// Axis == (0,0,1) x e0 = (-e0.y, e0.x, 0)
	Axis = XMVectorPermute<XM_PERMUTE_1Y, XM_PERMUTE_0X, XM_PERMUTE_0W, XM_PERMUTE_0Z>(e0, -e0);
	p0 = XMVector3Dot(TV0, Axis);
	// p1 = XMVector3Dot( v1, Axis ); // p1 = p0;
	p2 = XMVector3Dot(TV2, Axis);
	Min = XMVectorMin(p0, p2);
	Max = XMVectorMax(p0, p2);
	Radius = XMVector3Dot(Extents, XMVectorAbs(Axis));
	NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(Min, Radius));
	NoIntersection = XMVectorOrInt(NoIntersection, XMVectorLess(Max, -Radius));

	// Axis == (0,0,1) x e1 = (-e1.y, e1.x, 0)
	Axis = XMVectorPermute<XM_PERMUTE_1Y, XM_PERMUTE_0X, XM_PERMUTE_0W, XM_PERMUTE_0Z>(e1, -e1);
	p0 = XMVector3Dot(TV0, Axis);
	p1 = XMVector3Dot(TV1, Axis);
	// p2 = XMVector3Dot( v2, Axis ); // p2 = p1;
	Min = XMVectorMin(p0, p1);
	Max = XMVectorMax(p0, p1);
	Radius = XMVector3Dot(Extents, XMVectorAbs(Axis));
	NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(Min, Radius));
	NoIntersection = XMVectorOrInt(NoIntersection, XMVectorLess(Max, -Radius));

	// Axis == (0,0,1) x e2 = (-e2.y, e2.x, 0)
	Axis = XMVectorPermute<XM_PERMUTE_1Y, XM_PERMUTE_0X, XM_PERMUTE_0W, XM_PERMUTE_0Z>(e2, -e2);
	p0 = XMVector3Dot(TV0, Axis);
	p1 = XMVector3Dot(TV1, Axis);
	// p2 = XMVector3Dot( v2, Axis ); // p2 = p0;
	Min = XMVectorMin(p0, p1);
	Max = XMVectorMax(p0, p1);
	Radius = XMVector3Dot(Extents, XMVectorAbs(Axis));
	NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(Min, Radius));
	NoIntersection = XMVectorOrInt(NoIntersection, XMVectorLess(Max, -Radius));

	return XMVector4NotEqualInt(NoIntersection, XMVectorTrueInt());
}


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

ID3D11ShaderResourceView* D3DHelper::CreateRandomTexture1DSRV(ID3D11Device* device)
{
	// 
	// Create the random data.
	//
	XMFLOAT4 randomVal[1024];
	for (int i = 0; i < 1024; ++i)
	{
		randomVal[i].x = MathHelper::RandF(-1, 1);
		randomVal[i].y = MathHelper::RandF(-1, 1);
		randomVal[i].z = MathHelper::RandF(-1, 1);
		randomVal[i].w = MathHelper::RandF(-1, 1);
	}

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = randomVal;
	data.SysMemPitch = 1024 * sizeof(XMFLOAT4);
	data.SysMemSlicePitch = 0;

	//
	// Create the texture.
	//
	D3D11_TEXTURE1D_DESC texDesc;
	texDesc.Width = 1024;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	texDesc.Usage = D3D11_USAGE_IMMUTABLE;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;
	texDesc.ArraySize = 1;

	ID3D11Texture1D* randomTex = nullptr;
	HR(device->CreateTexture1D(&texDesc, &data, &randomTex));

	//
	// Create the resource view.
	//
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
	srvDesc.Texture1D.MipLevels = texDesc.MipLevels;
	srvDesc.Texture1D.MostDetailedMip = 0;

	ID3D11ShaderResourceView* randomTexSRV = nullptr;
	HR(device->CreateShaderResourceView(randomTex, &srvDesc, &randomTexSRV));

	ReleaseCOM(randomTex);

	return randomTexSRV;
}

