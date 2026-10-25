#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <optional>

namespace gl
{

  class HeightField;

  /// 軸に平行な直方体 障害物の衝突判定に利用する
  struct AABB
  {
    glm::vec3 min{0.0f};
    glm::vec3 max{0.0f};
  };

  /// 地形情報をまとめた構造体
  struct GroundContact
  {
    float height;
    glm::vec3 normal;
  };

  /**
   * @brief 障害物の一覧を持ち 円柱との重なりを解消する
   */
  class CollisionWorld
  {
  public:
    /**
     * @brief 衝突判定の箱（キューブオブジェクトのことではない）を追加
     * @param box 実際に衝突判定とする座標（ AABB ）
     */
    void Add(const AABB &box)
    {
      boxes_.push_back(box);
    }

    /**
     * @brief 箱とは別に衝突判定の地形を追加
     * @param terrain HeightField の地形
     * 注意：SceneTerrain よりも長い寿命である必要がある
     */
    void Add(const HeightField *terrain)
    {
      terrains_.push_back(terrain);
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
     * @brief 地面となる高さ（障害物を含む）を返す 地形とそのほかで処理を変えています
     *
     * @param footPos 足元の位置（円柱の底面の中心）
     * @param radius 足元の判定に使う円柱の半径
     * @return std::optional<float> 地面から足元までの高さ、見つからなければ nullopt
     */
    // std::optional<float> FindGroundHeight(const glm::vec3 &footPos, const float radius) const;

    /**
     * @brief 地面の法線を返す
     * @param footPos 足元の位置
     * @return std::optional<glm::vec3> 地面の法線 見つからなければ nullopt
     * TODO: 地形の上に AABB があっても地形の法線を返す状態になっている
     */
    // std::optional<glm::vec3> FindGroundNormal(const glm::vec3 &footPos) const;

    /**
     * @brief 地形情報を返す
     * @param footPos 足元の位置（円柱の底面の中心）
     * @param radius 足元の判定に使う円柱の半径
     * @return std::optional<GroundContact> 地面の高さと法線 そこに地面がないなら nullopt
     */
    std::optional<GroundContact> FindGround(const glm::vec3 &footPos, const float radius) const;

    /**
     * @brief 足元の位置が急な斜面かどうかを判定する関数
     * @param normal 足元の地形の法線ベクトル
     * @return 急な斜面なら true
     */
    bool IsSteepGround(glm::vec3 normal, const float maxDegree) const;

    /**
     * @brief 地形の急な斜面方向に移動できないように補正する関数
     * @param footPos 足元の位置
     * @param horizontalMove そのフレームでの移動量
     */
    glm::vec3 ConstrainMoveToSteepSlope(glm::vec3 footPos, glm::vec3 horizontalMove) const;

  private:
    std::vector<AABB> boxes_;
    std::vector<const HeightField *> terrains_;
  };

} // namespace gl
