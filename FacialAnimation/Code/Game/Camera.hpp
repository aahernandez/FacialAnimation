#pragma once
#include "Engine/Math/Vector3.hpp"
#include "Engine/Math/Matrix4.hpp"

class Camera
{
public:
	bool m_hasMoved;
	float m_rollAboutX;
	float m_pitchAboutY;
	float m_yawAboutZ;
	Vector3 m_position;
	Vector3 m_defaultOrientation;
	Vector3 m_defaultPosition;
	Matrix4 m_camMatrix;

	Camera();
	~Camera();

	void SetCameraPositionAndOrientation(const Vector3& playerPosAtEyeHeight);
	void ResetCameraPositionAndOrientation();
	void MakeMatrixLookAt(Vector3 target, Vector3 worldUp = Vector3( 0.0f, 1.0f, 0.0f ));

	Vector3 GetForwardXYZ() const;
	Vector3 GetForwardXY() const;
	Vector3 GetLeftXY() const;
};