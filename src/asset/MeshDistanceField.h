#pragma once
#include "asset/Mesh.h"
#include "gl/GlHandle.h"
#include <glm/glm.hpp>
#include <vector>

namespace gl {

/**
 * @brief 焼き込んだメッシュ距離場　GL_TEXTURE_3D と ローカル空間での AABB を持つ
 */
struct MeshDistanceField {
    TextureHandle texture; // GL_TEXTURE_3D, GL_R16F
    // メッシュの AABB に余白を足した範囲　テクスチャの uvw 0〜1 はこの範囲に対応する
    glm::vec3 boundsMin{0.0f};
    glm::vec3 boundsMax{0.0f};
};

/**
 * @brief GPU へ送る前に作った距離場　ディスクへの保存とアップロードの両方を受け渡す
 */
struct BakedDistanceField {
    int resolution = 0;
    glm::vec3 boundsMin{0.0f};
    glm::vec3 boundsMax{0.0f};
    std::vector<float> values; // resolution^3 個の距離の値
};

/**
 * @brief メッシュの距離場を事前に CPU で計算する
 */
BakedDistanceField BakeDistanceField(const Mesh &mesh, int resolution);

/**
 * @brief 作成した距離場を GL_TEXTURE_3D で GPU に送信する
 */
MeshDistanceField UploadMeshDistanceField(const BakedDistanceField &baked);
} // namespace gl
