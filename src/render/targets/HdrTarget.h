#pragma once
#include "gl/GlHandle.h"

namespace gl {

/**
 * @brief ライティング結果を HDR のまま受け取る FBO 明るい部分を2枚目へ分離する
 */
class HdrTarget {
  public:
    HdrTarget(int width, int height);

    GLuint Fbo() const {
        return fbo_;
    }
    GLuint Color() const {
        return color_;
    }
    // Bloom のぼかし元
    GLuint BrightColor() const {
        return brightColor_;
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
    TextureHandle color_;       // location=0 -> FragColor
    TextureHandle brightColor_; // location=1 -> BrightColor
    RenderbufferHandle rbo_;    // 深度とステンシル サンプリングしないのでレンダーバッファ
};

} // namespace gl
