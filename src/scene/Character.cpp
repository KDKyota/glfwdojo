#include "scene/Character.h"
#include "core/SceneUnits.h"

#include <cmath>
#include <iostream>

namespace
{

    constexpr float kPi = 3.14159265358979323846f;
    constexpr float kInputEpsilon = 1e-4f; // 0 より大きいけど十分に小さい数
    constexpr float kStickDist = gl::units::stairRiser;

    /// 角度を -π〜π の範囲へ正規化する
    float wrapAngle(float radians)
    {
        while (radians > kPi)
            radians -= 2.0f * kPi;
        while (radians < -kPi)
            radians += 2.0f * kPi;
        return radians;
    }

    CharacterMotionState determineMotionState(bool hasMoveInput, bool hasRunInput)
    {
        if (!hasMoveInput)
            return CharacterMotionState::Idle;
        if (hasRunInput)
            return CharacterMotionState::Run;

        return CharacterMotionState::Walk;
    }
} // namespace

Character::Character(const glm::vec3 &position, float height) : position_(position), height_(height), spawnPos_(position + glm::vec3(0.0f, CharacterDefaults::RESPAWN_HEIGHT, 0.0f))
{
}

void Character::Respawn()
{
    position_ = spawnPos_;
    verticalVelocity_ = 0.0f;
    yaw_ = 0.0f;
}

void Character::Move(const glm::vec3 &cameraFront, bool hasRunInput, bool hasJumpInput, const glm::vec2 &input, float deltaTime,
                     const gl::CollisionWorld &world)
{

    /* ---- 奈落に落ちていたらスポーン地点へ戻し 移動入力の有無を確かめる ---- */
    if (position_.y < CharacterDefaults::RESPAWN_THRESHOLD_Y)
    {
        Respawn();
    }
    /*if (MotionState() != CharacterMotionState::Jump && hasJumpInput) {
        verticalVelocity_ = gl::units::jumpSpeed;
        SetMotionState(CharacterMotionState::Jump);
    }*/
    const bool hasMoveInput = glm::dot(input, input) > kInputEpsilon;
    // 滑空は入力を受け付けない
    /*if (MotionState() != CharacterMotionState::Jump && MotionState() !=CharacterMotionState::Fall) {
        hasMoveInput = glm::dot(input, input) > kInputEpsilon;
        SetMotionState(determineMotionState(hasMoveInput, hasRunInput, hasJumpInput));
    }*/

    /* ---- 足元の地面を調べる 以降の接地判定と状態遷移はこの結果を使う ---- */
    const std::optional<gl::GroundContact> ground = world.FindGround(position_, CharacterDefaults::FOOT_RADIUS);

    // if (!groundHeight.has_value()) { // 床がない場合は落下状態にする
    //     SetMotionState(CharacterMotionState::Fall);
    // }

    /* ---- 重力で縦速度を更新し 移動後の足元の高さを求める ---- */

    verticalVelocity_ -= gl::units::gravity * deltaTime;

    float newHeight = position_.y + verticalVelocity_ * deltaTime; // 移動後の足元の高さ

    /* ---- 接地判定 めり込みか吸着なら接地とみなして足を地面に合わせる ここでの MotionState() はまだ前フレームの状態 ---- */
    // 前フレームのステートを取得して接地しているか
    const bool isPrevFrameOnGround = (MotionState() == CharacterMotionState::Idle || MotionState() == CharacterMotionState::Walk || MotionState() == CharacterMotionState::Run);
    // 上向きに動いていない
    const bool isNotMoveToUp = verticalVelocity_ <= 0;
    // 地面に吸着していい条件
    const bool canStickToGround = isPrevFrameOnGround && isNotMoveToUp;


    bool isOnGround = ground.has_value() && newHeight <= ground->height;

    if (ground.has_value()) // 地面がある
    {
        // 地面にめり込んでいるか
        const bool isSinkIntoGround = newHeight <= ground->height;
        // 吸着距離にいるか
        const bool isInStickDist = (newHeight - ground->height) <= kStickDist;
        isOnGround = isSinkIntoGround || (canStickToGround && isInStickDist);
    }
    else // 地面がないとき
    {
        isOnGround = false;
    }

    if (isOnGround)
    { // 地面にいる

        position_.y = ground->height;
        verticalVelocity_ = 0.0f;
    }
    else
    { // 空中にいる
        position_.y = newHeight;
    }

    bool isOnWalkableGround = isOnGround && !world.IsSteepGround(ground->normal); // 地面にいるけど急斜面ではない

    /* ---- 状態遷移 前フレームの状態と接地判定と入力から今フレームの状態を決める ---- */
    switch (MotionState())
    {
    case CharacterMotionState::Walk:
    case CharacterMotionState::Run:
    case CharacterMotionState::Idle:
        if (!isOnWalkableGround)
        {
            SetMotionState(CharacterMotionState::Fall);
        }
        else if (hasJumpInput)
        {
            verticalVelocity_ = gl::units::jumpSpeed;
            SetMotionState(CharacterMotionState::Jump_Start);
        }
        else
        {
            SetMotionState(determineMotionState(hasMoveInput, hasRunInput));
        }
        break;
    case CharacterMotionState::Jump_Start:
        if (verticalVelocity_ <= 0.0f)
            SetMotionState(CharacterMotionState::Fall);
        break;
    case CharacterMotionState::Fall:
        if (isOnWalkableGround)
        {
            SetMotionState(determineMotionState(hasMoveInput, hasRunInput));
        }
        break;
    }


    /* ---- 今のフレームの状態から水平速度を決める ---- */
    switch (MotionState())
    {
    case CharacterMotionState::Walk:
        velocity_ = gl::units::walkSpeed;
        break;
    case CharacterMotionState::Run:
        velocity_ = gl::units::runSpeed;
        break;
    case CharacterMotionState::Idle:
        velocity_ = 0.0f;
        break;
    case CharacterMotionState::Jump_Start:
    case CharacterMotionState::Fall:
        if (hasMoveInput)
        {
            velocity_ = hasRunInput ? gl::units::runSpeed : gl::units::walkSpeed;
        }
        else
        {
            velocity_ = 0.0f;
        }
        break;
    }


    /* ---- 水平移動 入力の向きへ動かし 崖と箱の壁で補正してから向き直る ---- */
    if (!hasMoveInput)
        return; // 注意：この early return を入れないと、以降でのderection の計算で 0 除算が起きる

    // 注意: forward の Y を 0 にしないとカメラが下を向いたとき前進で床に潜る
    const glm::vec3 forward = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));
    const glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));

    // 注意: 合成してから正規化しないと斜め移動が sqrt(2) 倍速くなる
    const glm::vec3 direction = glm::normalize(forward * input.y + right * input.x);

    // std::cout << "MotionState: " << static_cast<int>(MotionState()) << ", velocity: " << velocity_ << std::endl;

    // 水平移動量を扱う変数（この二行で水平量を取得する）
    glm::vec3 horizontalMove = direction * velocity_ * deltaTime;
    horizontalMove.y = 0.f; // 水平移動量なので、y成分を 0.f に
    // position_ += direction * velocity_ * deltaTime;

    // 崖などに移動する場合に補正した水平移動量に更新
    horizontalMove = world.ConstrainMoveToSteepSlope(position_, horizontalMove);
    position_ += horizontalMove;

    // 動かしてから押し戻す 面に沿った成分は残るので壁沿いに滑る
    position_ = world.Resolve(position_, CharacterDefaults::RADIUS, height_);
    // 壁沿いに滑っている間も入力した向きを保つ
    turnTowards(direction, deltaTime);
}

void Character::turnTowards(const glm::vec3 &direction, float deltaTime)
{
    const float targetYaw = std::atan2(direction.x, direction.z);
    // 注意: ここで差を折り返さないと 180 度付近で逆回りする
    const float delta = wrapAngle(targetYaw - yaw_);
    const float blend = 1.0f - std::exp(-CharacterDefaults::TURN_STIFFNESS * deltaTime);
    yaw_ = wrapAngle(yaw_ + delta * blend);
}
