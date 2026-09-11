#pragma once
#include "gl/GlHandle.h"

namespace gl {

/**
 * @brief 接空間のサンプル列を画素ごとに回転させる 4x4 のランダムテクスチャ
 *
 * SSAO と SDF 遮蔽が同じものをタイル状に敷いて共有する
 */
class NoiseTexture {
  public:
    NoiseTexture();

    GLuint Get() const {
        return texture_;
    }

  private:
    TextureHandle texture_;
};

} // namespace gl
