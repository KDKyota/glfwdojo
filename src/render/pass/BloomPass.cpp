#include "render/pass/BloomPass.h"

#include "gl/RenderTarget.h"

namespace gl {
namespace {

// blur.comp の local_size_x と一致させること
constexpr unsigned int kBlurTile = 256;

} // namespace

BloomPass::BloomPass(int width, int height) : width_(width), height_(height), shader_("blur.comp") {
    shader_.use();
    shader_.setInt("image", 0);

    /* ぼかし処理を書き込むFBO */
    // 縦方向と横方向にガウシアンブラーをかけるので二回のループ
    for (unsigned int i = 0; i < 2; i++) {
        pingpongFBO_[i].create();
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO_[i]);
        // REPEAT だとブラーが反対側の値を拾う
        AttachColorTarget(pingpongColorBuffers_[i], GL_COLOR_ATTACHMENT0, GL_RGBA16F, GL_RGBA, GL_FLOAT, width,
                          height, GL_LINEAR);
        CheckFramebufferComplete("BLOOM");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void BloomPass::Execute(const HdrTarget &hdr) {
    bool first_iteration = true;
    unsigned int amount = 10;
    shader_.use();
    // 前のパスが FBO へ書いた brightColorBuffer_ を imageLoad で読むため 可視化を挟む
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    for (unsigned int i = 0; i < amount; ++i) {
        const GLuint src = first_iteration ? hdr.BrightColor() : pingpongColorBuffers_[!blurHorizontal_].get();
        glBindImageTexture(0, src, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
        glBindImageTexture(1, pingpongColorBuffers_[blurHorizontal_], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
        shader_.setBool("horizontal", blurHorizontal_);

        // ワークグループはぼかす軸に沿って並べる もう一方の軸は1行（1列）につき1グループ
        const unsigned int along = blurHorizontal_ ? width_ : height_;
        const unsigned int lines = blurHorizontal_ ? height_ : width_;
        glDispatchCompute((along + kBlurTile - 1) / kBlurTile, lines, 1);
        // 次のパスが今の書き込みを読むので 完了を待たせる
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        blurHorizontal_ = !blurHorizontal_;
        if (first_iteration)
            first_iteration = false;
    }
    // TonemapPass は結果を imageLoad ではなく sampler で読むので 別のビットが要る
    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
}

} // namespace gl
