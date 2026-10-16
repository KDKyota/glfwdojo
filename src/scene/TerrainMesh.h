#pragma once
#include "../render/GeometryData.h"
#include "HeightField.h"
#include <vector>

namespace gl {
struct TerrainMeshData {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

/**
 * @brief ハイトマップの高さ計算結果から地形の頂点とインデックスを作成する
 *
 * @param field 高さと法線を格納する HeightField 型変数
 * @param origin 地形マップの左上原点
 * @param extent 地形マップの大きさ
 * @param cellCountX x 軸方向の格子の数
 * @param cellCountZ z 軸方向の格子の数
 * @param uvTileSize UV の一つ分の大きさ
 */
TerrainMeshData BuildTerrainMeshconst(HeightField &field, const glm::vec3 &origin, const glm::vec3 &extent,
                                      uint32_t cellCountX, uint32_t cellCountZ, float uvTileSize);

} // namespace gl
