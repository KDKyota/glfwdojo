#include "Terrainmesh.h"

namespace gl {
/*
vertices = 空の vector<Vertex>
for z in 0..行数:
    for x in 0..列数:
        pos    = 格子点のワールド座標（origin + 格子の間隔 * (x,z)、高さは HeightField）
        normal = HeightField.GetNormalAt(pos.x, pos.z)
        uv     = (x / 列数, z / 行数) に繰り返し係数を掛ける
        vertices に追加
indices = 空の vector<unsigned int>
for z in 0..行数-1:
    for x in 0..列数-1:
        左上 = z*列数 + x   （右上は +1、左下は +列数、右下は +列数+1）
        三角形2枚 を追加（巻き方向を床と揃える）
*/

TerrainMeshData TerrainMesh::BuildTerrainMeshconst(HeightField &field, const glm::vec3 &origin, const glm::vec3 &extent,
                                                   uint32_t cellCountX, uint32_t cellCountZ, float uvTileSize) {
    TerrainMeshData data{};
    data.vertices.reserve((cellCountX + 1) * (cellCountZ + 1));
    for (unsigned int i = 0; i < cellCountZ; ++i) {
        for (unsigned int j = 0; j < cellCountX; ++j) {
            glm::vec3 pos = origin + glm::vec3(extent.x * (j / cellCountX),
                                               field.GetHeightAt(field.GetResolutionLength().x * j,
                                                                 field.GetResolutionLength().z * i),
                                               extent.z * (i / cellCountZ));
            glm::vec3 normal = field.GetNormalAt(field.GetResolutionLength().x * j, field.GetResolutionLength().z * i);
            glm::vec2 uv = uvTileSize * glm::vec2(j / cellCountX, i / cellCountZ);
        }
    }
}

} // namespace gl
