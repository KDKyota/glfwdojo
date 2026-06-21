#include "Camera.h"

Camera::Camera() 
	: Position(glm::vec3(0.0f, 0.0f, 0.0f)),
	Up(glm::vec3(0.0f, 1.0f, 0.0f)),
	Yaw(CameraDefaults::YAW),
	Pitch(CameraDefaults::PITCH),
	Front(glm::vec3(0.0f, 0.0f, -1.0f)),
	MovementSpeed(CameraDefaults::SPEED),
	MouseSensitivity(CameraDefaults::SENSITIVITY),
	Zoom(CameraDefaults::ZOOM),
	WorldUp(glm::vec3(0.0f, 1.0f, 0.0f))
{
	UpdateCameraVectors();
};

glm::mat4 Camera::GetViewMatrix() const
{
	return glm::lookAt(Position, Position + Front, Up);
};

const float& Camera::GetZoomValue() const
{
	return Zoom;
};

void Camera::UpdateCameraVectors()
{
	glm::vec3 front;
	front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
	front.y = sin(glm::radians(Pitch));
	front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
	Front = glm::normalize(front);
	Right = glm::normalize(glm::cross(Front, WorldUp));
	Up = glm::normalize(glm::cross(Right, Front));
};

void Camera::ProcessKeyboard(Camera_Movement direction, float deltaTime)
{
	float velocity = MovementSpeed * deltaTime;

	if (direction == Camera_Movement::FORWARD)
	{
		Position += velocity * Front;
	}
	else if (direction == Camera_Movement::BACKWARD)
	{
		Position -= velocity * Front;
	}
	else if (direction == Camera_Movement::LEFT)
	{
		Position -= velocity * Right;
	}
	else
	{
		Position += velocity * Right;
	}
};

void Camera::ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch)
{
	xoffset *= MouseSensitivity;
	yoffset *= MouseSensitivity;

	Yaw += xoffset;
	Pitch += yoffset;

	// make sure that when pitch is out of bounds, screen doesn't get flipped
	if (constrainPitch)
	{
		if (Pitch > 89.0f) Pitch = 89.0f;
		if (Pitch < -89.0f) Pitch = -89.0f;
	}

	UpdateCameraVectors();

};

void Camera::ProcessMouseScroll(float yoffset)
{
	Zoom -= yoffset;
	if (Zoom < 1.0f) Zoom = 1.0f;
	if (Zoom > 45.0f) Zoom = 45.0f;
};


