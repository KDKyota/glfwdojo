#pragma once
#include "gl/GlHandle.h"

namespace gl {

/**
 * @brief Deferred Shading の幾何情報を受け取る3枚のテクスチャと FBO
 */
class GBuffer {
  public:
    GBuffer(int width, int height);

    /// 3枚を texunit::kGPosition から連番で割り当てる
    void BindTextures() const;

    /// 深度を dst の FBO へコピーする 前方描画が不透明物と前後判定するために要る
    void BlitDepthTo(GLuint dst) const;

    GLuint Fbo() const {
        return fbo_;
    }
    GLuint Position() const {
        return position_;
    }
    GLuint Normal() const {
        return normal_;
    }
    int Width() const {
        return width_;
    }
    int Height() const {
        return height_;
    }

  private:
    int width_, height_;
    FramebufferHandle fbo_;
    TextureHandle position_, normal_, albedoRoughness_; // fbo_ にアタッチする3枚のテクスチャ
    RenderbufferHandle depthRBO_;                       // fbo_ 用の深度バッファ
};

} // namespace gl
