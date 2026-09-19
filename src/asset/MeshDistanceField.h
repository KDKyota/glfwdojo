#pragma once
#include "asset/Mesh.h"
#include "gl/GlHandle.h"
#include <glm/glm.hpp>

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
 * @brief 符号付き距離場を計算し GL_TEXTURE_3D へアップロードする　起動時に一度だけ呼ぶ想定
 */
MeshDistanceField BakeMeshDistanceField(const Mesh &mesh, int resolution);

} // namespace gl
