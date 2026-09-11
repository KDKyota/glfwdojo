#include "render/pass/SsaoPass.h"

#include "render/TextureUnits.h"

#include <glm/glm.hpp>
#include <random>
#include <string>
#include <vector>

namespace gl {
namespace {

constexpr unsigned int kKernelSize = 64;
constexpr float kRadius = 0.6f; // 遮蔽を探す半径 目安は物体サイズの 0.2〜1.0 倍
constexpr float kBias = 0.03f;  // 自己遮蔽によるアクネ対策 半径に比例させる
constexpr float kPower = 2.0f;  // AO のコントラスト 実用範囲は 1.5〜3.0

} // namespace

SsaoPass::SsaoPass()
    : shader_("fragment_quad.vert", "ssao.frag"), blurShader_("fragment_quad.vert", "ssao_blur.frag") {
    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
    std::default_random_engine generator;

    /* --- サンプルカーネル --- */
    // 接空間（+Z が法線方向）における 半球内のサンプル点のテンプレート
    std::vector<glm::vec3> kernel;
    kernel.reserve(kKernelSize);
    for (unsigned int i = 0; i < kKernelSize; ++i) {
        glm::vec3 sample(randomFloats(generator) * 2.0f - 1.0f, randomFloats(generator) * 2.0f - 1.0f,
                         randomFloats(generator));
        sample = glm::normalize(sample);
        sample *= randomFloats(generator); // 半球の表面ではなく内部に散らす

        float scale = static_cast<float>(i) / static_cast<float>(kKernelSize);
        scale = 0.1f + 0.9f * scale * scale; // 二次関数で原点寄りに偏らせる
        sample *= scale;

        kernel.push_back(sample);
    }

    shader_.use();
    shader_.setInt("gPosition", texunit::kGPosition);
    shader_.setInt("gNormal", texunit::kGNormal);
    shader_.setInt("texNoise", texunit::kNoise);
    shader_.setFloat("radius", kRadius);
    shader_.setFloat("bias", kBias);
    // カーネルは実行中に変化しないので一度だけ送れば十分
    for (unsigned int i = 0; i < kKernelSize; ++i)
        shader_.setVec3("samples[" + std::to_string(i) + "]", kernel[i]);

    blurShader_.use();
    blurShader_.setInt("ssaoInput", 0);
    blurShader_.setFloat("power", kPower);
}

void SsaoPass::Execute(const OcclusionTarget &target, const GBuffer &gbuffer, const NoiseTexture &noise,
                       const SceneGeometry &geometry) {
    glViewport(0, 0, target.Width(), target.Height());
    glBindFramebuffer(GL_FRAMEBUFFER, target.Fbo());
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0 + texunit::kGPosition);
    glBindTexture(GL_TEXTURE_2D, gbuffer.Position());
    glActiveTexture(GL_TEXTURE0 + texunit::kGNormal);
    glBindTexture(GL_TEXTURE_2D, gbuffer.Normal());
    glActiveTexture(GL_TEXTURE0 + texunit::kNoise);
    glBindTexture(GL_TEXTURE_2D, noise.Get());
    shader_.use();
    geometry.DrawScreenQuad();

    /* -- SSAO blur pass -- */
    // 4x4 のノイズをタイル状に敷いた代償の格子模様を 同じ 4x4 の平均で打ち消す
    glBindFramebuffer(GL_FRAMEBUFFER, target.BlurFbo());
    glClear(GL_COLOR_BUFFER_BIT);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, target.Buffer());
    blurShader_.use();
    geometry.DrawScreenQuad();

    glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

} // namespace gl
