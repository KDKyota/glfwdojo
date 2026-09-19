#pragma once
#include "gl/GlHandle.h"

namespace gl {

/**
 * @brief 遮蔽率を書くパスの出力先 本体とブラー後の2枚を持つ
 *
 * @param debugName FBO が不完全だったときに報告へ添える名前
 * @param channelCount 1 なら R のみ 2 なら RG（SDF 遮蔽が拡散と鏡面を1枚に持つ）
 */
class OcclusionTarget {
  public:
    OcclusionTarget(int width, int height, const char *debugName, int channelCount = 1);

    GLuint Fbo() const {
        return fbo_;
    }
    GLuint BlurFbo() const {
        return blurFbo_;
    }
    GLuint Buffer() const {
        return buffer_;
    }
    GLuint BlurredBuffer() const {
        return blurredBuffer_;
    }
    int Width() const {
        return width_;
    }
    int Height() const {
        return height_;
    }

  private:
    int width_, height_;
    FramebufferHandle fbo_, blurFbo_;
    TextureHandle buffer_, blurredBuffer_; // GL_R8 か GL_RG8
};

} // namespace gl
