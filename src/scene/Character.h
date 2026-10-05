#pragma once

#include <glm/glm.hpp>

#include "scene/Collision.h"
#include "core/SceneUnits.h"

namespace CharacterDefaults {
// 進行方向へ向き直る速さ
constexpr float TURN_STIFFNESS = 12.0f;
// 衝突判定に使う円柱の半径 見た目のメッシュより少し太い
constexpr float RADIUS = 0.3f;
// 高さはモデルの実寸を渡す これは読み込めなかった場合の既定値
constexpr float HEIGHT = gl::units::characterHeight;
} // namespace CharacterDefaults

/**
 * @brief キャラクターのモーション状態
 */
enum class CharacterMotionState {
    Idle,
    Walk,
    Run,
    Jump,
    Fall,
};

/**
 * @brief プレイヤーが操作するキャラクターの位置と向きを持つ
 */
class Character {
  public:
    /**
     * @brief 足元の位置を指定して生成する
     */
    explicit Character(const glm::vec3 &position, float height = CharacterDefaults::HEIGHT);

    /**
     * @brief カメラ基準の入力で移動し 進行方向へ向き直る
     * @param hasRunInput 走る入力があるか
     * @param cameraFront カメラの視線方向
     * @param input xが右方向yが前方向の -1〜1
     * @param deltaTime 前フレームからの経過時間
     * @param world 移動後のめり込みを解消する障害物
     */
    void Move(const glm::vec3 &cameraFront, bool hasRunInput, bool hasJumpInput, const glm::vec2 &input, float deltaTime, const gl::CollisionWorld &world);

    const glm::vec3 &Position() const {
        return position_;
    }
    float Radius() const {
        return CharacterDefaults::RADIUS;
    }
    float Height() const {
        return height_;
    }
    float Yaw() const {
        return yaw_;
    }

    CharacterMotionState MotionState() const { return motionState_; }
    void SetMotionState(CharacterMotionState state) { motionState_ = state; }

    void SetVerticalVelocity(float velocity) { verticalVelocity_ = velocity; }

  private:
    glm::vec3 position_;
    float yaw_ = 0.0f;
    float height_ = CharacterDefaults::HEIGHT;
    CharacterMotionState motionState_ = CharacterMotionState::Idle;
    float velocity_ = gl::units::walkSpeed;

    float verticalVelocity_ = 0.0f; // 上下方向の速度

    /// 進行方向へyawを補間する
    void turnTowards(const glm::vec3 &direction, float deltaTime);
};
