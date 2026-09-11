#include "render/pass/SdfOcclusionPass.h"

#include "core/SceneUnits.h"
#include "render/TextureUnits.h"
#include "scene/SceneLayout.h"

#include <cstddef>
#include <glm/glm.hpp>
#include <stdexcept>

namespace gl {
namespace {

// sdf_common.glsl の SDF_MAX_BOXES と一致させること
constexpr int kSdfMaxBoxes = 8;
// sdf_common.glsl が参照する UBO の binding
constexpr GLuint kSceneUboBinding = 2;

} // namespace

SdfOcclusionPass::SdfOcclusionPass()
    : shader_("fragment_quad.vert", "sdf_occlusion.frag"),
      blurShader_("fragment_quad.vert", "sdf_occlusion_blur.frag") {
    uploadSceneUbo();

    shader_.use();
    shader_.setInt("gPosition", texunit::kGPosition);
    shader_.setInt("gNormal", texunit::kGNormal);
    shader_.setInt("texNoise", texunit::kNoise);

    blurShader_.use();
    blurShader_.setInt("sdfOcclusionInput", 0);
}

void SdfOcclusionPass::uploadSceneUbo() {
    struct SdfSceneBlock {
        glm::vec4 boxCenters[kSdfMaxBoxes];
        glm::vec4 wallCenters[2];
        glm::vec4 boxHalfSize;
        glm::vec4 wallHalfSize;
        glm::vec4 sceneParams;
    } block{};

    if (layout::cubePositions.size() > kSdfMaxBoxes)
        throw std::runtime_error("Too many cubes for the SDF UBO");
    for (std::size_t i = 0; i < layout::cubePositions.size(); ++i)
        block.boxCenters[i] = glm::vec4(layout::cubePositions[i], 0.0f);
    block.boxHalfSize = glm::vec4(0.5f);

    // 厚さゼロの板ポリは内外が定義できないので薄い箱で近似する
    constexpr float wallHalfThickness = 0.1f;
    constexpr float wallCenterY = (units::floorY + units::wallTopY) * 0.5f;
    constexpr float wallHalfHeight = (units::wallTopY - units::floorY) * 0.5f;
    block.wallCenters[0] = glm::vec4(0.0f, wallCenterY, -units::floorHalfExtent - wallHalfThickness, 0.0f);
    block.wallCenters[1] = glm::vec4(0.0f, wallCenterY, units::floorHalfExtent + wallHalfThickness, 0.0f);
    block.wallHalfSize = glm::vec4(units::floorHalfExtent, wallHalfHeight, wallHalfThickness, 0.0f);

    // w はレイがシーンを確実に抜けきる距離（床の横幅）
    block.sceneParams = glm::vec4(units::floorY, static_cast<float>(layout::cubePositions.size()), 2.0f,
                                  units::floorHalfExtent * 2.0f);

    sceneUBO_.create();
    glBindBuffer(GL_UNIFORM_BUFFER, sceneUBO_);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(block), &block, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, kSceneUboBinding, sceneUBO_);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void SdfOcclusionPass::Execute(const OcclusionTarget &target, const GBuffer &gbuffer, const NoiseTexture &noise,
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

    /* -- blur pass -- */
    // 回転が残したノイズを 4x4 の平均で均す 実質のサンプル数がここで増える
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
