#pragma once
#include "gl/GlHandle.h"

namespace gl {

/**
 * @brief 鏡像カメラで描いた床の映り込みを受ける FBO 色は後で読み 深度は描画中の深度テストにだけ使う
 */
class ReflectionTarget {
  public:
    ReflectionTarget(int width, int height);
    GLuint Fbo() const { return fbo_; }
    GLuint Color() const { return color_; }
    int Width() const { return width_; }
    int Height() const { return height_; }

  private:
    int width_, height_;
    FramebufferHandle fbo_;
    TextureHandle color_;
    RenderbufferHandle rbo_;
};

} // namespace gl
