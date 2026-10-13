#pragma once
#include "render/GeometryData.h"
#include <glm/glm.hpp>
#include <vector>

namespace gl {

// AABB の外側にも距離場を持たせる余白　extent に対する比率
inline constexpr float kBoundsMarginRatio = 0.25f;

/**
 * @brief 符号付き距離場をボクセルの一次元配列として計算する
 *
 * @param vertices,indices 三角形リストのメッシュ
 * @param resolution 一辺の格子点数
 * @param boundsMin,boundsMax 余白を足した後の範囲　メッシュの AABB そのものではない
 * @return resolution^3 個の距離 内側が負 並びは x + y * resolution + z * resolution^2
 */
std::vector<float> BuildSignedDistanceField(const std::vector<Vertex> &vertices,
                                            const std::vector<unsigned int> &indices, int resolution,
                                            const glm::vec3 &boundsMin, const glm::vec3 &boundsMax);

} // namespace gl
