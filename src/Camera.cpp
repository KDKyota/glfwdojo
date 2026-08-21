#include "Camera.h"

#include <cmath>
#include <glm/ext/quaternion_trigonometric.hpp>

Camera::Camera()
	: Position(glm::vec3(0.0f, 0.0f, 3.0f)),
	Front(glm::vec3(0.0f, 0.0f, -1.0f)),
	Up(glm::vec3(0.0f, 1.0f, 0.0f)),
	WorldUp(glm::vec3(0.0f, 1.0f, 0.0f)),
	MovementSpeed(CameraDefaults::SPEED),
	MouseSensitivity(CameraDefaults::SENSITIVITY),
	Zoom(CameraDefaults::ZOOM)
{
	// 単位クォータニオンが -Z 向き（YAW = -90 度）にあたるので、そこからの差分で初期姿勢を作る
	Orientation = glm::angleAxis(glm::radians(-(CameraDefaults::YAW + 90.0f)), WorldUp) *
	              glm::angleAxis(glm::radians(CameraDefaults::PITCH), glm::vec3(1.0f, 0.0f, 0.0f));
	UpdateCameraVectors();
};

/* Getter */
glm::mat4 Camera::GetViewMatrix() const
{
	return glm::lookAt(Position, Position + Front, Up);
};

glm::vec3 Camera::GetViewPosition() const
{
	return Position;
}

glm::vec3 Camera::GetViewFront() const
{
	return Front;
}

const float& Camera::GetZoomValue() const
{
	return Zoom;
};
/* ここまでGetter */

void Camera::UpdateCameraVectors()
{
	Front = glm::normalize(Orientation * glm::vec3(0.0f, 0.0f, -1.0f));
	// Right をワールドの上方向から導くことで、姿勢にロールが残っていても水平線は傾かない
	Right = glm::normalize(glm::cross(Front, WorldUp));
	Up = glm::normalize(glm::cross(Right, Front));
};

float Camera::CurrentPitch() const
{
	return std::asin(glm::clamp(Front.y, -1.0f, 1.0f));
}

void Camera::ProcessKeyboard(Camera_Movement direction, float deltaTime)
{
	// 三人称ではカメラの位置が注視点に縛られるので、直接の移動は受け付けない
	if (mode_ == CameraMode::ThirdPerson) return;

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
	else if (direction == Camera_Movement::RIGHT)
	{
		Position += velocity * Right;
	}
	else if (direction == Camera_Movement::UP)
	{
		Position += velocity * Up;
	}
	else if (direction == Camera_Movement::DOWN)
	{
		Position -= velocity * Up;
	}
};

void Camera::ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch)
{
	const float yaw = glm::radians(-xoffset * MouseSensitivity);
	float pitch = glm::radians(yoffset * MouseSensitivity);
	const float limit = glm::radians(CameraDefaults::PITCH_LIMIT);

	if (mode_ == CameraMode::ThirdPerson)
	{
		orbitYaw_ += yaw;
		orbitPitch_ += pitch;
		if (constrainPitch)
			orbitPitch_ = glm::clamp(orbitPitch_, -limit, limit);
		return;
	}

	if (constrainPitch)
	{
		// 真上・真下を越えると視界が反転する。回転そのものではなく増分を切り詰める
		const float current = CurrentPitch();
		pitch = glm::clamp(current + pitch, -limit, limit) - current;
	}

	// ヨーはワールドの上方向まわり（左から）、ピッチはカメラ自身の右方向まわり（右から）に掛ける。
	// 両方をローカル軸で掛けるとロールが溜まって水平線が傾いていく
	Orientation = glm::angleAxis(yaw, WorldUp) * Orientation *
	              glm::angleAxis(pitch, glm::vec3(1.0f, 0.0f, 0.0f));
	// 掛け続けると数値誤差で長さが 1 からずれるので毎回正規化する
	Orientation = glm::normalize(Orientation);

	UpdateCameraVectors();
};

void Camera::ProcessMouseScroll(float yoffset)
{
	// 三人称では画角ではなく注視点までの距離を変える方が自然
	if (mode_ == CameraMode::ThirdPerson)
	{
		orbitDistance_ = glm::clamp(orbitDistance_ - yoffset * 0.5f,
		                            CameraDefaults::ORBIT_MIN_DISTANCE,
		                            CameraDefaults::ORBIT_MAX_DISTANCE);
		return;
	}

	Zoom -= yoffset;
	if (Zoom < 1.0f) Zoom = 1.0f;
	if (Zoom > 45.0f) Zoom = 45.0f;
};

void Camera::SetFollowTarget(const glm::vec3& position)
{
	followTarget_ = position;
	hasFollowTarget_ = true;
}

glm::vec3 Camera::PivotPosition() const
{
	return followTarget_ + glm::vec3(0.0f, CameraDefaults::TARGET_HEIGHT, 0.0f);
}

void Camera::ClearFollowTarget()
{
	hasFollowTarget_ = false;
	mode_ = CameraMode::FreeLook;
}

void Camera::ToggleMode()
{
	if (mode_ == CameraMode::ThirdPerson)
	{
		mode_ = CameraMode::FreeLook;
		return;
	}
	// 追従先が無いまま切り替えると、移動もできず注視点も無い状態で固まる
	if (!hasFollowTarget_) return;

	// 現在の視線を軌道角へ引き継ぐ
	// こうしないと切り替えた瞬間に画面が飛ぶぞ！（長州力風）
	orbitYaw_ = std::atan2(-Front.x, -Front.z);
	orbitPitch_ = std::asin(glm::clamp(Front.y, -1.0f, 1.0f));
	// 補完の初期化
	// 合わせておかないとおかしな挙動になる
	smoothedPivot_ = PivotPosition();

	mode_ = CameraMode::ThirdPerson;
}

CameraMode Camera::Mode() const
{
	return mode_;
}

void Camera::Update(float deltaTime)
{
	if (mode_ != CameraMode::ThirdPerson || !hasFollowTarget_) return;

	// 視線の逆方向へ orbitDistance_ だけ下がった位置が理想の視点
	// const glm::vec3 desiredPosition = PivotPosition() - Front * orbitDistance_;

	// 1 フレームで詰める割合を指数で求める
	const float blend = 1.0f - std::exp(-CameraDefaults::FOLLOW_STIFFNESS * deltaTime);
	// Position = glm::mix(Position, desiredPosition, blend);
	smoothedPivot_ = glm::mix(smoothedPivot_, PivotPosition(), blend);

	const glm::quat orbit = glm::angleAxis(orbitYaw_, WorldUp) * glm::angleAxis(orbitPitch_, glm::vec3(1.0f, 0.0f, 0.0f));

	Position = smoothedPivot_ + orbit * glm::vec3(0.0f, 0.0f, orbitDistance_);
	Orientation = orbit;

	UpdateCameraVectors();

	// 位置が遅れている間は注視点が画面中心からずれるので、向きも slerp で追従させる
	// const glm::vec3 toPivot = PivotPosition() - Position;
	// if (glm::dot(toPivot, toPivot) > 1e-6f)
	// {
	// 	Orientation = glm::normalize(glm::slerp(Orientation, lookRotation(toPivot, WorldUp), blend));
	// 	UpdateCameraVectors();
	// }
}
