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
constexpr float PITCH_LIMIT = 89.0f;
} // namespace CameraDefaults

/**
 * @brief View Matrix を生成するカメラ。FreeLook（自由視点）と ThirdPerson（追従）の2モードを持つ。
 */
class Camera {
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
    float orbitYaw_ = 0.0f;
    float orbitPitch_ = 0.0f;
    glm::vec3 smoothedPivot_ = glm::vec3(0.0f); // カメラが周回する中心点

    // camera position
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

    // 注視点の式を一本化する。Update() と ToggleMode() で食い違うと切り替え時だけ縦にずれる
    glm::vec3 PivotPosition() const;

    // Orientation から Front / Right / Up を作り直す
    void UpdateCameraVectors();

    // pitch は Front から逆算する
    float CurrentPitch() const;

  public:
    // constructor with vectors
    Camera();

    /**
     * @brief View Matrix を返す。
     */
    glm::mat4 GetViewMatrix() const;

    /**
     * @brief カメラの位置を返す。
     */
    glm::vec3 GetViewPosition() const;

    /**
     * @brief カメラの視線方向を返す。
     */
    glm::vec3 GetViewFront() const;

    const float &GetZoomValue() const;

    // process input recievedc from any keyborad-like input system.
    void ProcessKeyboard(Camera_Movement direction, float deltaTime);

    // process input recieved from a mouse input system.
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true);

    // processes input recieved from a mouse scroll-wheel event.
    void ProcessMouseScroll(float yoffset);

    /* ---- 三人称モード ---- */

    // 追従先は毎フレーム外から与える
    void SetFollowTarget(const glm::vec3 &position);
    void ClearFollowTarget();

    // 追従先が無いときは三人称へ切り替えない（カメラが動かせなくなるため）
    void ToggleMode();
    CameraMode Mode() const;

    /**
     * @brief ThirdPerson モードのカメラ追従と軌道回転を更新する。
     *
     * @param deltaTime 前フレームからの経過時間。
     */
    void Update(float deltaTime);
};
