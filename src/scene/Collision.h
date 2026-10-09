#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <optional>

namespace gl {

/// 軸に平行な直方体 障害物の衝突判定に利用する
struct AABB {
    glm::vec3 min{0.0f};
    glm::vec3 max{0.0f};
};

/**
 * @brief 障害物の一覧を持ち 円柱との重なりを解消する
 */
class CollisionWorld {
  public:
    void Add(const AABB &box) {
        boxes_.push_back(box);
    }

    /**
     * @brief 主にキャラクターに適用する円柱が障害物にめり込んでいれば押し出した位置を返す
     *
     * @param footPosition 円柱の底面の中心
     * @param radius 円柱の半径
     * @param height 円柱の高さ
     */
    glm::vec3 Resolve(const glm::vec3 &footPosition, float radius, float height) const;

    /**
     * @brief 地面となる高さ（障害物を含む）を返す
     *
     * @param footPos 足元の位置（円柱の底面の中心）
     * @return std::optional<float> 地面から足元までの高さ、見つからなければ nullopt
     */
    std::optional<float> FindGroundHeight(const glm::vec3 &footPos, const float radius) const;

  private:
    std::vector<AABB> boxes_;
};

} // namespace gl
