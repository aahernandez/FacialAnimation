#include "Game/Camera.hpp"
#include "Engine/Math/MathUtilities.hpp"

Camera::Camera()
	: m_position(0.f, 0.f, 0.f)
	, m_yawAboutZ(0.f)
	, m_pitchAboutY(0.f)
	, m_rollAboutX(0.f)
	, m_defaultOrientation(0.f, 0.f, 180.f)
	, m_defaultPosition(0.f, 0.f, -25.f)
	, m_hasMoved(false)
{
}

Camera::~Camera()
{

}

void Camera::SetCameraPositionAndOrientation(const Vector3& playerPosAtEyeHeight)
{
	m_position = playerPosAtEyeHeight;
}

void Camera::ResetCameraPositionAndOrientation()
{
	m_position = m_defaultPosition;
	m_rollAboutX = m_defaultOrientation.x;
	m_pitchAboutY = m_defaultOrientation.y;
	m_yawAboutZ = m_defaultOrientation.z;
}

void Camera::MakeMatrixLookAt(Vector3 target, Vector3 worldUp /*= Vector3( 0.0f, 1.0f, 0.0f )*/)
{
	Vector3 dir = target - m_position;
	Vector3 forward = dir.GetNormalized();
	Vector3 right = worldUp.Cross(forward);
	Vector3 up = forward.Cross(right);

	Vector4 i = Vector4(right, 0.0f);
	Vector4 j = Vector4(up, 0.0f);
	Vector4 k = Vector4(forward, 0.0f);
	Vector4 t = Vector4(m_position, 1.0f);
	Matrix4 mat(i, j, k, t);

	m_camMatrix = mat;
}

Vector3 Camera::GetForwardXYZ() const
{
	float xPos = CosDegrees(m_yawAboutZ) * CosDegrees(m_pitchAboutY);
	float yPos = SinDegrees(m_yawAboutZ) * CosDegrees(m_pitchAboutY);
	float zPos = -SinDegrees(m_pitchAboutY);
	return Vector3(xPos, yPos, zPos) * 1.f;
}

Vector3 Camera::GetForwardXY() const
{
	Vector3 mForwardDirection = Vector3( SinDegrees(m_yawAboutZ), 0.f, -CosDegrees(m_yawAboutZ));
	return mForwardDirection * 1.f;
}

Vector3 Camera::GetLeftXY() const
{
	Vector3 mLeftDirection = Vector3( CosDegrees(m_yawAboutZ), 0.f, SinDegrees(m_yawAboutZ) );
	return mLeftDirection * 1.f;
}
