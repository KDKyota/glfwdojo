#include "Character.h"

#include <cmath>

namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kInputEpsilon = 1e-4f; // 0 より大きいけど十分に小さい数

/// 角度差を -π〜π に作り直す（170° と -190° は同じ角度なのでそろえるための関数）
float wrapAngle(float radians) {
    while (radians > kPi)
        radians -= 2.0f * kPi;
    while (radians < -kPi)
        radians += 2.0f * kPi;
    return radians;
}

} // namespace

Character::Character(const glm::vec3 &position, float height) : position_(position), height_(height) {
}

void Character::Move(const glm::vec3 &cameraFront, const glm::vec2 &input, float deltaTime,
                     const gl::CollisionWorld &world) {
    isMoving_ = glm::dot(input, input) > kInputEpsilon;
    if (!isMoving_)
        return;

    // 注意: foward の Y を 0 にしないとカメラが下を向いたとき前進で床に潜る
    const glm::vec3 forward = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));
    const glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));

    // 注意: 合成してから正規化しないと斜め移動が sqrt(2) 倍速くなる
    const glm::vec3 direction = glm::normalize(forward * input.y + right * input.x);

    position_ += direction * CharacterDefaults::MOVE_SPEED * deltaTime;
    // 動かしてから押し戻す 面に沿った成分は残るので壁沿いに滑る
    position_ = world.Resolve(position_, CharacterDefaults::RADIUS, height_);
    turnTowards(direction, deltaTime);
}

void Character::turnTowards(const glm::vec3 &direction, float deltaTime) {
    const float targetYaw = std::atan2(direction.x, direction.z);
    // 注意: ここで差を折り返さないと 180 度付近で逆回りするゾ！
    const float delta = wrapAngle(targetYaw - yaw_);
    const float blend = 1.0f - std::exp(-CharacterDefaults::TURN_STIFFNESS * deltaTime);
    yaw_ = wrapAngle(yaw_ + delta * blend);
}
