#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "SceneUnits.h"

enum class Camera_Movement {
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
	UP,
	DOWN
};

enum class CameraMode {
	FreeLook,    // 自由視点。シーンの任意の場所を見に行くデバッグ用に残してある
	ThirdPerson, // 注視点のまわりを周回する追従カメラ
};

// default camera values
namespace CameraDefaults {
	constexpr float YAW = -90.0f;
	constexpr float PITCH = 0.0f;
	// メートル毎秒。人の歩行は 1.4、走りは 4.5 前後（gl::units 参照）
	constexpr float SPEED = gl::units::freeCameraSpeed;
	constexpr float SENSITIVITY = 0.1f;
	constexpr float ZOOM = 45.0f;

	/* ---- 三人称モード ---- */
	constexpr float ORBIT_DISTANCE = 4.0f;
	constexpr float ORBIT_MIN_DISTANCE = 1.5f;
	constexpr float ORBIT_MAX_DISTANCE = 12.0f;
	// 注視点は足元ではなく胸のあたり
	constexpr float TARGET_HEIGHT = gl::units::characterHeight * 0.7f;
	// 追従の追いつく速さ
	constexpr float FOLLOW_STIFFNESS = 8.0f;
}

class Camera
{
private:
	glm::vec3 Position;
	// 姿勢はクォータニオンで保持し、Front / Right / Up はここから導出する
	glm::quat Orientation;
	glm::vec3 Front;
	glm::vec3 Up;
	glm::vec3 Right;
	glm::vec3 WorldUp;

	CameraMode mode_ = CameraMode::FreeLook;
	glm::vec3 followTarget_ = glm::vec3(0.0f);
	bool hasFollowTarget_ = false;
	float orbitDistance_ = CameraDefaults::ORBIT_DISTANCE;

	// camera position
	float MovementSpeed;
	float MouseSensitivity;
	float Zoom;

	// Orientation から Front / Right / Up を作り直す
	void UpdateCameraVectors();

	// pitch は Front から逆算する
	float CurrentPitch() const;

public:
	// constructor with vectors
	Camera();

	// return the view matrix calculated using Eular Angles and the LookAt Matrix
	glm::mat4 GetViewMatrix() const;

	glm::vec3 GetViewPosition() const;

	glm::vec3 GetViewFront() const;

	const float& GetZoomValue() const;

	// process input recievedc from any keyborad-like input system.
	void ProcessKeyboard(Camera_Movement direction, float deltaTime);

	// process input recieved from a mouse input system.
	void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true);

	// processes input recieved from a mouse scroll-wheel event.
	void ProcessMouseScroll(float yoffset);

	/* ---- 三人称モード ---- */

	// 追従先は毎フレーム外から与える
	void SetFollowTarget(const glm::vec3& position);
	void ClearFollowTarget();

	// 追従先が無いときは三人称へ切り替えない（カメラが動かせなくなるため）
	void ToggleMode();
	CameraMode Mode() const;

	void Update(float deltaTime);
};
