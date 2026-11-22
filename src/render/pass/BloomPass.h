#pragma once
#include "gl/GlHandle.h"
#include "gl/Shader.h"
#include "render/targets/HdrTarget.h"
#include "render/SceneGeometry.h"
#include <array>

namespace gl {

/**
 * @brief HDR ターゲットの明るい部分を縦横交互のガウシアンブラーでぼかす
 */
class BloomPass {
  public:
    BloomPass(int width, int height);

    void Execute(const HdrTarget &hdr, const SceneGeometry &geomerty);

    /// 最後に書き込んだ側のテクスチャ Execute() の後にだけ意味がある
    GLuint Result() const {
        return pingpongColorBuffers_[!blurHorizontal_];
    }

  private:
    // MacOS では GLSL の compute shader が使えないので 二つの関数に分ける
    void executeWithCompute(const HdrTarget &hdr);
    void executeWithFragment(const HdrTarget &hdr, const SceneGeometry &geometry);
    int width_, height_;
    Shader shader_; // Bloom のぼかし（Compute）
    std::array<FramebufferHandle, 2> pingpongFBO_;
    std::array<TextureHandle, 2> pingpongColorBuffers_;
    // 縦横を交互に切り替えるフラグ
    bool blurHorizontal_ = true;
};

} // namespace gl
