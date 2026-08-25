#pragma once

#include <glm/glm.hpp>

#include "SceneUnits.h"

namespace CharacterDefaults {
constexpr float MOVE_SPEED = gl::units::walkSpeed;
// 進行方向へ向き直る速さ
constexpr float TURN_STIFFNESS = 12.0f;
} // namespace CharacterDefaults

/**
 * @brief プレイヤーが操作するキャラクターの位置と向きを持つ。
 */
class Character {
  public:
    /**
     * @brief 足元の位置を指定して生成する。
     */
    explicit Character(const glm::vec3 &position);

    /**
     * @brief カメラ基準の入力で移動し 進行方向へ向き直る
     *
     * @param cameraFront カメラの視線方向
     * @param input xが右方向yが前方向の -1〜1
     * @param deltaTime 前フレームからの経過時間
     */
    void Move(const glm::vec3 &cameraFront, const glm::vec2 &input, float deltaTime);

    const glm::vec3 &Position() const {
        return position_;
    }
    float Yaw() const {
        return yaw_;
    }
    bool IsMoving() const {
        return isMoving_;
    }

  private:
    glm::vec3 position_;
    float yaw_ = 0.0f;
    bool isMoving_ = false;

    /// 進行方向へyawを補間する。
    void turnTowards(const glm::vec3 &direction, float deltaTime);
};
