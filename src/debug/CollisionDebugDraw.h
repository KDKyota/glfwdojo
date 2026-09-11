#pragma once
#include "gl/GlHandle.h"
#include "gl/Shader.h"
#include "scene/Character.h"

namespace gl {

/**
 * @brief 衝突判定の円柱をワイヤーフレームで重ねて描く
 */
class CollisionDebugDraw {
  public:
    CollisionDebugDraw();

    /// character が nullptr なら何も描かない
    void Draw(const Character *character);

  private:
    Shader shader_;
    VertexArrayHandle cylinderVAO_;
    BufferHandle cylinderVBO_;
    int cylinderVertexCount_ = 0;
};

} // namespace gl
