#include "Camera.h"

using namespace DirectX;

bool Frustum::IsPointInFrustum(FXMVECTOR v) const
{
	for (int i = 0; i < 6; ++i)
	{
		if (XMVectorGetX(XMPlaneDotCoord(XMLoadFloat4(&m_Planes[i].p), v)) <= 0)
		{
			return false;
		}
	}

	return true;
}

bool Frustum::IsIntersected(const Box& box) const
{
	XMFLOAT4 v[8];

	XMFLOAT3 maxV;
	XMFLOAT3 minV;

	XMStoreFloat3(&maxV, box.GetMaxV());
	XMStoreFloat3(&minV, box.GetMinV());

	v[0].x = minV.x; v[0].y = minV.y; v[0].z = minV.z;
	v[1].x = maxV.x; v[1].y = minV.y; v[1].z = minV.z;
	v[2].x = minV.x; v[2].y = minV.y; v[2].z = maxV.z;
	v[3].x = maxV.x; v[3].y = minV.y; v[3].z = maxV.z;

	v[4].x = minV.x; v[4].y = maxV.y; v[4].z = minV.z;
	v[5].x = maxV.x; v[5].y = maxV.y; v[5].z = minV.z;
	v[6].x = minV.x; v[6].y = maxV.y; v[6].z = maxV.z;
	v[7].x = maxV.x; v[7].y = maxV.y; v[7].z = maxV.z;

	for (int i = 0; i < 8; ++i)
	{
		if (IsPointInFrustum(XMLoadFloat4(&v[i])))
		{
			return true;
		}
	}
	return false;

	/*for (int i = 0; i < 6; ++i)
	{
		XMFLOAT4 v;

		XMFLOAT3 maxV;
		XMFLOAT3 minV;

		XMStoreFloat3(&maxV, box.GetMaxV());
		XMStoreFloat3(&minV, box.GetMinV());

		if (m_Planes[i].p.x > 0)
		{
			v.x = maxV.x;
		}
		else
		{
			v.x = minV.x;
		}

		if (m_Planes[i].p.y > 0)
		{
			v.y = maxV.y;
		}
		else
		{
			v.y = minV.y;
		}

		if (m_Planes[i].p.z > 0)
		{
			v.z = maxV.z;
		}
		else
		{
			v.z = minV.z;
		}

		if (XMVectorGetX(XMPlaneDotCoord(XMLoadFloat4(&m_Planes[i].p), XMLoadFloat4(&v))) <= 0)
		{
			return false;
		}
	}

	return true;*/
}

Camera::Camera()
	: m_Position(0.f, 0.f, -0.f)
	, m_Right(1.f, 0.f, 0.f)
	, m_Up(0.f, 1.f, 0.f)
	, m_Look(0.f, 0.f, 1.f)
{
	SetLens(0.25f * MathHelper::Pi, 1.0, 1.f, 1000.f);
}

Camera::~Camera()
{

}

XMVECTOR Camera::GetPositionXM() const
{
	return XMLoadFloat3(&m_Position);
}

XMFLOAT3 Camera::GetPosition() const
{
	return m_Position;
}

void Camera::SetPosition(float x, float y, float z)
{
	m_Position = XMFLOAT3(x, y, z);
}

void Camera::SetPosition(const XMFLOAT3& v)
{
	m_Position = v;
}

XMVECTOR Camera::GetRightXM()const
{
	return XMLoadFloat3(&m_Right);
}

XMFLOAT3 Camera::GetRight()const
{
	return m_Right;
}

XMVECTOR Camera::GetUpXM()const
{
	return XMLoadFloat3(&m_Up);
}

XMFLOAT3 Camera::GetUp()const
{
	return m_Up;
}

XMVECTOR Camera::GetLookXM()const
{
	return XMLoadFloat3(&m_Look);
}

XMFLOAT3 Camera::GetLook()const
{
	return m_Look;
}

float Camera::GetNearZ()const
{
	return m_NearZ;
}

float Camera::GetFarZ()const
{
	return m_FarZ;
}

float Camera::GetAspect()const
{
	return m_Aspect;
}

float Camera::GetFovY()const
{
	return m_FovY;
}

float Camera::GetFovX()const
{
	float halfWidth = 0.5f*GetNearWindowWidth();
	return 2.0f*atan(halfWidth / m_NearZ);
}

float Camera::GetNearWindowWidth()const
{
	return m_Aspect * m_NearWindowHeight;
}

float Camera::GetNearWindowHeight()const
{
	return m_NearWindowHeight;
}

float Camera::GetFarWindowWidth()const
{
	return m_Aspect * m_FarWindowHeight;
}

float Camera::GetFarWindowHeight()const
{
	return m_FarWindowHeight;
}

const Frustum& Camera::GetFrustum() const
{
	return m_Frustum;
}

void Camera::SetLens(float fovY, float aspect, float zn, float zf)
{
	m_FovY = fovY;
	m_Aspect = aspect;
	m_NearZ = zn; 
	m_FarZ = zf;

	m_NearWindowHeight = 2.f * m_NearZ * tanf(0.5f * m_FovY);
	m_FarWindowHeight = 2.f * m_FarZ * tanf(0.5f * m_FovY);

	XMMATRIX P = XMMatrixPerspectiveFovLH(fovY, aspect, zn, zf);
	XMStoreFloat4x4(&m_Proj, P);
}

void Camera::LookAt(FXMVECTOR pos, FXMVECTOR target, FXMVECTOR worldUp)
{
	XMVECTOR L = XMVector3Normalize(XMVectorSubtract(target, pos));
	XMVECTOR R = XMVector3Normalize(XMVector3Cross(worldUp, L));
	XMVECTOR U = XMVector3Cross(L, R);

	XMStoreFloat3(&m_Position, pos);
	XMStoreFloat3(&m_Look, L);
	XMStoreFloat3(&m_Right, R);
	XMStoreFloat3(&m_Up, U);
}

void Camera::LookAt(const XMFLOAT3& pos, const XMFLOAT3& target, const XMFLOAT3& up)
{
	XMVECTOR P = XMLoadFloat3(&pos);
	XMVECTOR T = XMLoadFloat3(&target);
	XMVECTOR U = XMLoadFloat3(&up);

	LookAt(P, T, U);
}

XMMATRIX Camera::View()const
{
	return XMLoadFloat4x4(&m_View);
}

XMMATRIX Camera::Proj()const
{
	return XMLoadFloat4x4(&m_Proj);
}

XMMATRIX Camera::ViewProj()const
{
	return XMMatrixMultiply(View(), Proj());
}

void Camera::Strafe(float d)
{
	// m_Position += d * m_Right
	XMVECTOR s = XMVectorReplicate(d);
	XMVECTOR r = XMLoadFloat3(&m_Right);
	XMVECTOR p = XMLoadFloat3(&m_Position);
	XMStoreFloat3(&m_Position, XMVectorMultiplyAdd(s, r, p));
}

void Camera::Walk(float d)
{
	// m_Position += d * m_Look
	XMVECTOR s = XMVectorReplicate(d);
	XMVECTOR l = XMLoadFloat3(&m_Look);
	XMVECTOR p = XMLoadFloat3(&m_Position);
	XMStoreFloat3(&m_Position, XMVectorMultiplyAdd(s, l, p));
}

void Camera::Pitch(float angle)
{
	// Rotate up and look vector about the right vector.
	XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&m_Right), angle);
	XMStoreFloat3(&m_Up, XMVector3TransformNormal(XMLoadFloat3(&m_Up), R));
	XMStoreFloat3(&m_Look, XMVector3TransformNormal(XMLoadFloat3(&m_Look), R));
}

void Camera::RotateY(float angle)
{
	// Rotate the basis vectors about the world y-axis.
	XMMATRIX R = XMMatrixRotationY(angle);
	XMStoreFloat3(&m_Right, XMVector3TransformNormal(XMLoadFloat3(&m_Right), R));
	XMStoreFloat3(&m_Up, XMVector3TransformNormal(XMLoadFloat3(&m_Up), R));
	XMStoreFloat3(&m_Look, XMVector3TransformNormal(XMLoadFloat3(&m_Look), R));
}

void Camera::UpdateViewMatrix()
{
	XMVECTOR R = XMLoadFloat3(&m_Right);
	XMVECTOR U = XMLoadFloat3(&m_Up);
	XMVECTOR L = XMLoadFloat3(&m_Look);
	XMVECTOR P = XMLoadFloat3(&m_Position);

	// Keep camera's axes orthogonal to each other and of unit length.
	L = XMVector3Normalize(L);
	U = XMVector3Normalize(XMVector3Cross(L, R));
	R = XMVector3Cross(U, L);

	// Fill in the view matrix entries.
	float x = -XMVectorGetX(XMVector3Dot(P, R));
	float y = -XMVectorGetX(XMVector3Dot(P, U));
	float z = -XMVectorGetX(XMVector3Dot(P, L));

	XMStoreFloat3(&m_Right, R);
	XMStoreFloat3(&m_Up, U);
	XMStoreFloat3(&m_Look, L);

	m_View(0, 0) = m_Right.x;
	m_View(1, 0) = m_Right.y;
	m_View(2, 0) = m_Right.z;
	m_View(3, 0) = x;

	m_View(0, 1) = m_Up.x;
	m_View(1, 1) = m_Up.y;
	m_View(2, 1) = m_Up.z;
	m_View(3, 1) = y;

	m_View(0, 2) = m_Look.x;
	m_View(1, 2) = m_Look.y;
	m_View(2, 2) = m_Look.z;
	m_View(3, 2) = z;

	m_View(0, 3) = 0.0f;
	m_View(1, 3) = 0.0f;
	m_View(2, 3) = 0.0f;
	m_View(3, 3) = 1.0f;


}

void Camera::CalLocalFrustum(FXMMATRIX world)
{
	// Corners of the projection frustum in homogenous space.
	static XMVECTOR HomogenousPoints[6] =
	{
		{ -1.0f, 1.0f, 0.0f, 1.0f }, // top-left-near
		{ -1.0f, 1.0f, 1.0f, 1.0f }, // top-left-far
		{ 1.0f, 1.0f, 1.0f, 1.0f }, // top-right-far
		{ -1.0f, -1.0f, 0.0f, 1.0f }, // bottom-left-near
		{ 1.0f, -1.0f, 0.0f, 1.0f }, // bottom-right-near
		{ 1.0f, -1.0f, 1.0f, 1.0f } // bottom-right-far
	};
	XMVECTOR Determinant;
	XMMATRIX wvp = XMMatrixMultiply(world, ViewProj());
	XMMATRIX matInverse = XMMatrixInverse(&Determinant, Proj());
	// Compute the frustum corners in world space.
	XMVECTOR Points[6];
	for (int i = 0; i < 6; i++)
	{
		// Transform point.
		Points[i] = XMVector4Transform(HomogenousPoints[i], matInverse);
		Points[i] *= XMVectorReciprocal(XMVectorSplatW(Points[i]));
	}

	// Top plane
	m_Frustum.m_Planes[0] = XMPlaneFromPoints(Points[2], Points[1], Points[0]);

	// Bottom plane
	m_Frustum.m_Planes[1] = XMPlaneFromPoints(Points[5], Points[4], Points[3]);

	// Left plane
	m_Frustum.m_Planes[2] = XMPlaneFromPoints(Points[3], Points[0], Points[1]);

	// Right plane
	m_Frustum.m_Planes[3] = XMPlaneFromPoints(Points[4], Points[5], Points[2]);

	// Near plane
	m_Frustum.m_Planes[4] = XMPlaneFromPoints(Points[0], Points[3], Points[4]);

	// Far plane
	m_Frustum.m_Planes[5] = XMPlaneFromPoints(Points[1], Points[2], Points[5]);
}
