#include "Collision.h"

#include <algorithm>
#include <cmath>

namespace {

// 角では2面から同時に押されるので繰り返して落ち着かせる
constexpr int kResolveIterations = 4; // Resolve() が計算を繰り返す最大回数
constexpr float kCenterEpsilonSq = 1e-8f;
// 判定を見た目より太らせて接触する手前で止める
constexpr float kSkinWidth = 0.02f;

/// XZ 平面での押し出し量を求める 重なっていなければ false
bool pushOutXZ(const gl::AABB &box, const glm::vec3 &center, float radius, glm::vec2 &push) { // ここでいう push とは押し出しベクトル
    // 注意: 判定と押し出し量で違う半径を使うと壁際で震える
    const float skinRadius = radius + kSkinWidth;
    // 矩形に関して，円に最も近い点を見つける
    const float closestX = std::clamp(center.x, box.min.x, box.max.x);
    const float closestZ = std::clamp(center.z, box.min.z, box.max.z);
    const glm::vec2 offset(center.x - closestX, center.z - closestZ); // 矩形上の最近接点から円の中心へ向かうベクトル
    const float distanceSq = glm::dot(offset, offset);                // 距離の二乗

    /**
     * ここまでで center.x が既に[box.min.x, box.max.x]の範囲内にあれば
     * clamp は何もクランプせずに closestX = center.x をそのまま返す
     * そうなると offset = (0, 0) のゼロベクトルになる
     * そうなると distanceSq もほぼ 0 になる
     */

    if (distanceSq > skinRadius * skinRadius)
        return false;

    if (distanceSq > kCenterEpsilonSq) { // 中心が矩形の外側にある（ふつうはこの条件分岐を踏む）
        const float distance = std::sqrt(distanceSq);
        push = offset / distance * (skinRadius - distance);
        return true;
    }

    // 中心が矩形の内側なので抜ける距離が最も短い面へ出す
    const float toMinX = center.x - box.min.x;
    const float toMaxX = box.max.x - center.x;
    const float toMinZ = center.z - box.min.z;
    const float toMaxZ = box.max.z - center.z;
    const float shortest = std::min({toMinX, toMaxX, toMinZ, toMaxZ});

    if (shortest == toMinX)
        push = {-(toMinX + skinRadius), 0.0f};
    else if (shortest == toMaxX)
        push = {toMaxX + skinRadius, 0.0f};
    else if (shortest == toMinZ)
        push = {0.0f, -(toMinZ + skinRadius)};
    else
        push = {0.0f, toMaxZ + skinRadius};
    return true;
}

} // namespace

/// 全ての障害物に対して何度も繰り返して適用する
glm::vec3 gl::CollisionWorld::Resolve(const glm::vec3 &footPosition, float radius, float height) const {
    glm::vec3 result = footPosition;

    for (int i = 0; i < kResolveIterations; ++i) {
        bool pushed = false;
        for (const AABB &box : boxes_) {
            // 高さが重なっていなければ XZ を調べる必要がない
            if (result.y >= box.max.y || result.y + height <= box.min.y)
                continue;

            glm::vec2 push(0.0f);
            if (!pushOutXZ(box, result, radius, push))
                continue;

            result.x += push.x;
            result.z += push.y;
            pushed = true;
        }
        if (!pushed)
            break;
    }
    return result;
}
