#include "scene/Collision.h"
#include "HeightField.h"
#include "Character.h"

#include <algorithm>
#include <cmath>

namespace
{

    // 角では2面から同時に押されるので繰り返して落ち着かせる
    constexpr int kResolveIterations = 4; // Resolve() が計算を繰り返す最大回数
    constexpr float kCenterEpsilonSq = 1e-8f;
    // 判定を見た目より太らせて接触する手前で止める
    constexpr float kSkinWidth = 0.02f;

    constexpr float kGroundContactTolerance = 0.02f; // 足元の障害物に接触しているとみなす距離

    /**
     * @brief XZ 平面での押し出し量を求める 重なっていなければ false
     *
     * @param[out] push 重なっているときの押し出し量
     */
    bool pushOutXZ(const gl::AABB &box, const glm::vec3 &center, float radius, glm::vec2 &push)
    {
        // 注意: 判定と押し出し量で違う半径を使うと壁際で震える
        const float skinRadius = radius + kSkinWidth;
        // 矩形上で円中心に最も近い点と そこから中心へ向かうベクトル
        const float closestX = std::clamp(center.x, box.min.x, box.max.x);
        const float closestZ = std::clamp(center.z, box.min.z, box.max.z);
        const glm::vec2 offset(center.x - closestX, center.z - closestZ);
        const float distanceSq = glm::dot(offset, offset);

        if (distanceSq > skinRadius * skinRadius)
            return false;

        // distanceSq がほぼ 0 なら中心が矩形の内側にある（offset がゼロベクトルになる）
        if (distanceSq > kCenterEpsilonSq)
        {
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

glm::vec3 gl::CollisionWorld::Resolve(const glm::vec3 &footPosition, float radius, float height) const
{
    glm::vec3 result = footPosition;

    for (int i = 0; i < kResolveIterations; ++i)
    {
        bool pushed = false;
        for (const AABB &box : boxes_)
        {
            // 高さが重なっていなければ XZ を調べる必要がない
            if (result.y >= box.max.y - kGroundContactTolerance || result.y + height <= box.min.y)
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


/* std::optional<float> gl::CollisionWorld::FindGroundHeight(const glm::vec3 &footPos, const float radius) const
{
    std::optional<float> groundHeight;
    // 床や箱などの AABB 上の処理
    for (const AABB &box : boxes_)
    {
        // 足元の XZ が矩形の外側にあれば continue
        if (footPos.x + radius < box.min.x || footPos.x - radius > box.max.x || footPos.z + radius < box.min.z || footPos.z - radius > box.max.z)
            continue;

        if (footPos.y >= box.max.y - kGroundContactTolerance)
        {
            if (!groundHeight.has_value() || box.max.y > *groundHeight)
            { // groundheight の値が box の天井より低いなら
                groundHeight = box.max.y;
            }
        }
    }

    // 次のループは地形（Terrain）上での処理
    for (const HeightField *terrain : terrains_)
    {
        const float terrainHeight = terrain->GetHeightAt(footPos.x, footPos.z);
        // 足元の中心一点を確認する
        // 足元の中心点が地形の外側にあれば continue
        if (!terrain->IsOnTerrain(footPos.x, footPos.z))
            continue;
        if (!groundHeight.has_value() || terrainHeight > *groundHeight)
            groundHeight = terrainHeight;
    }

    return groundHeight;
}


std::optional<glm::vec3> gl::CollisionWorld::FindGroundNormal(const glm::vec3 &footPos) const
{
    std::optional<glm::vec3> groundNormal;
    for (const HeightField *terrain : terrains_)
    {
        // 足元が地形の外なら continue
        if (!terrain->IsOnTerrain(footPos.x, footPos.z))
            continue;
        groundNormal = terrain->GetNormalAt(footPos.x, footPos.z);
    }
    return groundNormal;
} */

bool gl::CollisionWorld::IsSteepGround(glm::vec3 normal, float maxDegree) const
{
    return normal.y < glm::cos(glm::radians(maxDegree));
}

std::optional<gl::GroundContact> gl::CollisionWorld::FindGround(const glm::vec3 &footPos, const float radius) const
{
    std::optional<GroundContact> ground;
    // 床や箱などの AABB 上の処理
    for (const AABB &box : boxes_)
    {
        // 足元の XZ が矩形の外側にあれば continue
        if (footPos.x + radius < box.min.x || footPos.x - radius > box.max.x || footPos.z + radius < box.min.z || footPos.z - radius > box.max.z)
            continue;

        if (footPos.y >= box.max.y - kGroundContactTolerance)
        {
            if (!ground.has_value() || box.max.y > ground->height)
            {                                                   // groundheight の値が box の天井より低いなら
                ground = {box.max.y, glm::vec3(0.f, 1.f, 0.f)}; // y 正の方向を法線とする
                // 注意：optional でのアロー演算子は未定義動作なので、丸ごと代入するしかない
            }
        }
    }

    // 次のループは地形（Terrain）上での処理
    for (const HeightField *terrain : terrains_)
    {
        const float terrainHeight = terrain->GetHeightAt(footPos.x, footPos.z);
        // 足元の中心一点を確認する
        // 足元の中心点が地形の外側にあれば continue
        if (!terrain->IsOnTerrain(footPos.x, footPos.z))
            continue;
        if (!ground.has_value() || terrainHeight > ground->height)
        {
            ground = {terrainHeight, terrain->GetNormalAt(footPos.x, footPos.z)};
        }
    }
    return ground;
}

glm::vec3 gl::CollisionWorld::ConstrainMoveToSteepSlope(glm::vec3 footPos, glm::vec3 horizontalMove) const
{
    glm::vec3 nextFramePos = footPos + horizontalMove;
    glm::vec3 moveAmount = horizontalMove; // 移動量（return の値）
    for (const auto &terrain : terrains_)
    {
        if (!terrain->IsOnTerrain(nextFramePos.x, nextFramePos.z)) // 地形ではないなら関係ない（TODO: 床に階段をつくとなったときには変更が必要）
            continue;
        if (terrain->GetHeightAt(nextFramePos.x, nextFramePos.z) <= footPos.y) // 登らないときは関係ない
            continue;

        glm::vec3 normal = terrain->GetNormalAt(nextFramePos.x, nextFramePos.z);


        if (!IsSteepGround(normal, CharacterDefaults::CLIMB_MAX_DEGREE)) // ここでは登れる傾きなので何もしない
            continue;

        // 下る向き（水平成分）
        glm::vec3 downDirection = glm::normalize(glm::vec3(normal.x, 0.f, normal.z));
        // 登る量（horizontalMove と downDirection が逆向きなら崖へ向かっていることになる）
        float climbAmount = glm::dot(moveAmount, downDirection);

        if (climbAmount < 0)
        { // 崖へ向かっているとき
            moveAmount -= downDirection * climbAmount;
        }
    }
    return moveAmount;
}
