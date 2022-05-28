#include "pch.h"
#include "Camera.h"

//camera for our app simple directX application. While it performs some basic functionality its incomplete. 
//

Camera::Camera()
{
	//initalise values. 
	//Orientation and Position are how we control the camera. 
	m_orientation.x = -90.0f;		//rotation around x - pitch
	m_orientation.y = 0.0f;		//rotation around y - yaw
	m_orientation.z = 0.0f;		//rotation around z - roll	//we tend to not use roll a lot in first person

	m_position.x = 0.0f;		//camera position in space. 
	m_position.y = 0.0f;
	m_position.z = 0.0f;

	//These variables are used for internal calculations and not set.  but we may want to queary what they 
	//externally at points
	m_lookat.x = 0.0f;		//Look target point
	m_lookat.y = 0.0f;
	m_lookat.z = 0.0f;

	m_forward.x = 0.0f;		//forward/look direction
	m_forward.y = 0.0f;
	m_forward.z = 0.0f;

	m_right.x = 0.0f;
	m_right.y = 0.0f;
	m_right.z = 0.0f;
	
	//
	m_movespeed = 0.30;
	m_camRotRate = 3.0;

	m_up.x = 0.0f;
	m_up.y = 1.0f;
	m_up.z = 0.0f;


	//force update with initial values to generate other camera data correctly for first update. 
	Update();
}


Camera::~Camera()
{
}

void Camera::Update()
{

	float pitch = (m_orientation.x) * 3.1415f / 180.0f;
	float yaw = (m_orientation.y) * 3.1415f / 180.0f;
	m_forward.x = sin(yaw) * sin(pitch);
	m_forward.y = cos(pitch);
	m_forward.z = cos(yaw) * sin(pitch);
	m_forward.Normalize();

	//rotation in yaw - using the paramateric equation of a circle
	//m_forward.x = sin((m_orientation.y)*3.1415f / 180.0f);
	//m_forward.z = cos((m_orientation.y)*3.1415f / 180.0f);
	//m_forward.Normalize();


	//m_up.z = cos((m_orientation.x) * 3.1415f / 180.0f);
	//m_up.y = sin((m_orientation.x) * 3.1415f / 180.0f);
	//m_up.Normalize();

	//create right vector from look Direction
//	m_up = m_forward;
//	m_up.Cross(DirectX::SimpleMath::Vector3::UnitY, m_up);

	m_forward.Cross(DirectX::SimpleMath::Vector3::UnitY, m_right);

	//m_right.Cross(m_forward, m_up);

	//update lookat point
	// m_lookat = m_position + m_up;
	m_lookat = m_position + m_forward;


	//apply camera vectors and create camera matrix
	m_cameraMatrix = (DirectX::SimpleMath::Matrix::CreateLookAt(m_position, m_lookat, DirectX::SimpleMath::Vector3::UnitY));

}

DirectX::SimpleMath::Matrix Camera::getCameraMatrix()
{
	return m_cameraMatrix;
}

void Camera::setPosition(DirectX::SimpleMath::Vector3 newPosition)
{
	m_position = newPosition;
}

DirectX::SimpleMath::Vector3 Camera::getPosition()
{
	return m_position;
}

DirectX::SimpleMath::Vector3 Camera::getForward()
{
	return m_forward;
}

DirectX::SimpleMath::Vector3 Camera::getUp()
{
	return m_up;
}



void Camera::setRotation(DirectX::SimpleMath::Vector3 newRotation)
{
	m_orientation = newRotation;
}

DirectX::SimpleMath::Vector3 Camera::getRotation()
{
	return m_orientation;
}

float Camera::getMoveSpeed()
{
	return m_movespeed;
}

float Camera::getRotationSpeed()
{
	return m_camRotRate;
}

void Camera::RenderReflection(float height, float y_rot) {
	DirectX::SimpleMath::Vector3 forward, position, lookAt;
	float radians;



	// Setup the position of the camera in the world.
	// For planar reflection invert the Y position of the camera.
	position.x = m_position.x;
	//position.y = -m_position.y + (height * 0.5f);
	position.y = -m_position.y + (height * 2.0f);
	position.z = m_position.z;


	// Calculate the rotation in radians.
	//radians = m_orientation.y * 0.0174532925f;
	radians = m_orientation.y * (2 * 3.1415926f / 360.0);

//	lookAt.x = sinf(radians) + m_position.x;
//lookAt.y = position.y;
//lookAt.z = cosf(radians) + m_position.z;

	//float pitch = (m_orientation.x) * 3.1415f / 180.0f;
	//float yaw = (m_orientation.y) * 3.1415f / 180.0f;
	forward.x = m_forward.x;
	forward.y = - m_forward.y;
	forward.z = m_forward.z;


	// Setup where the camera is looking.
	
	//if (-0.5 <= cos(y_rot * 3.1415926 / 180) && cos(y_rot * 3.1415926 / 180) <= 0.5 ) {
	//	lookAt.y = cos(pitch) + position.y;
	//	

	//}
	//else {
	//	lookAt.y = cos(pitch) + m_position.y;
	//}




	lookAt = position + forward;
	m_reflectionMatrix = (DirectX::SimpleMath::Matrix::CreateLookAt(position, lookAt, DirectX::SimpleMath::Vector3::UnitY));
	return;
}
DirectX::SimpleMath::Matrix Camera::getReflectionMatrix()
{
	return m_reflectionMatrix;
}
