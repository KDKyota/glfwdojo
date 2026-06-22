#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum class Camera_Movement {
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
	UP, 
	DOWN
};

// default camera values
namespace CameraDefaults {
	constexpr float YAW = -90.0f;
	constexpr float PITCH = 0.0f;
	constexpr float SPEED = 2.5f;
	constexpr float SENSITIVITY = 0.1f;
	constexpr float ZOOM = 45.0f;
}

class Camera
{
private:
	glm::vec3 Position;
	glm::vec3 Front;
	glm::vec3 Up;
	glm::vec3 Right;
	glm::vec3 WorldUp;

	// euler angles
	float Yaw;
	float Pitch;
	float Roll;
	// camera position
	float MovementSpeed;
	float MouseSensitivity;
	float Zoom;

	// calculate the front vector from the Camera's Euler Angles
	void UpdateCameraVectors();

public:
	// constructor with vectors
	Camera();

	// return the view matrix calculated using Eular Angles and the LookAt Matrix
	glm::mat4 GetViewMatrix() const;

	glm::vec3 GetViewPosition() const;

	const float& GetZoomValue() const;

	// process input recievedc from any keyborad-like input system.
	void ProcessKeyboard(Camera_Movement direction, float deltaTime);

	// process input recieved from a mouse input system.
	void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true);

	// processes input recieved from a mouse scroll-wheel event.
	void ProcessMouseScroll(float yoffset);



};

