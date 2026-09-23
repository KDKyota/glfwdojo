#pragma once
#include "gl/GlHandle.h"
#include "gl/RenderTarget.h"

namespace gl {

/**
 * @brief 鏡像カメラで描いた床の映り込みを受け取る FBO
 */
class ReflectionTarget {
  public:
    ReflectionTarget(int width, int height);
    GLuint Fbo() const { return fbo_; }
    TargetView View() const { return {fbo_, width_, height_}; }
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
