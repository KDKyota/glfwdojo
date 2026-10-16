#include "TerrainMesh.h"
#include <stdexcept>

namespace gl {

TerrainMeshData BuildTerrainMesh(HeightField &field, const glm::vec3 &extent, uint32_t cellCountX, uint32_t cellCountZ,
                                 float uvTileSize) {
    if (cellCountX == 0 || cellCountZ == 0) {
        throw std::invalid_argument("cellCountX and cellCountZ must be greater than 0");
    }
    TerrainMeshData data{};
    data.vertices.reserve((cellCountX + 1) * (cellCountZ + 1));
    for (unsigned int i = 0; i <= cellCountZ; ++i) {
        for (unsigned int j = 0; j <= cellCountX; ++j) {
            glm::vec3 pos{};
            // ワールド座標の x, z を計算してから、その値を y の計算に利用する
            pos.x = field.GetOrigin().x + extent.x * static_cast<float>(j) / static_cast<float>(cellCountX);
            pos.z = field.GetOrigin().z + extent.z * static_cast<float>(i) / static_cast<float>(cellCountZ);
            pos.y = field.GetHeightAt(pos.x, pos.z);
            glm::vec3 normal = field.GetNormalAt(pos.x, pos.z);
            glm::vec2 uv = uvTileSize * glm::vec2(static_cast<float>(j) / static_cast<float>(cellCountX),
                                                  static_cast<float>(i) / static_cast<float>(cellCountZ));
            data.vertices.push_back({pos, normal, uv});
        }
    }

    data.indices.reserve(cellCountX * cellCountZ * 6);
    for (unsigned int i = 0; i < cellCountZ; ++i) {
        for (unsigned int j = 0; j < cellCountX; ++j) {
            unsigned int topLeft = i * (cellCountX + 1) + j;
            unsigned int topRight = topLeft + 1;
            unsigned int bottomLeft = topLeft + (cellCountX + 1);
            unsigned int bottomRight = bottomLeft + 1;

            data.indices.push_back(topLeft);
            data.indices.push_back(bottomLeft);
            data.indices.push_back(topRight);
            data.indices.push_back(topRight);
            data.indices.push_back(bottomLeft);
            data.indices.push_back(bottomRight);
        }
    }

    return data;
}

} // namespace gl
