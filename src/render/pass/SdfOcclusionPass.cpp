#include "render/pass/SdfOcclusionPass.h"

#include "core/SceneUnits.h"
#include "render/TextureUnits.h"
#include "scene/SceneLayout.h"

#include <cstddef>
#include <glm/glm.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace gl {
namespace {

// sdf_common.glsl の SDF_MAX_BOXES と一致させること
constexpr int kSdfMaxBoxes = 8;
// sdf_common.glsl が参照する UBO の binding
constexpr GLuint kSceneUboBinding = 2;
// main.cpp の kDebugModes と対応させること
constexpr int kDebugModeStepCount = 16;

} // namespace

SdfOcclusionPass::SdfOcclusionPass(const SceneModels &models)
    : shader_("fragment_quad.vert", "sdf_occlusion.frag"),
      blurShader_("fragment_quad.vert", "sdf_occlusion_blur.frag") {
    uploadSceneUbo(models);

    shader_.use();
    shader_.setInt("gPosition", texunit::kGPosition);
    shader_.setInt("gNormal", texunit::kGNormal);
    shader_.setInt("gAlbedoRoughness", texunit::kSdfGAlbedoRoughness);
    shader_.setInt("texNoise", texunit::kNoise);
    // modelDistanceFields[] は UBO に入らない（sampler は opaque 型）ので 配列の各要素へ個別に設定する
    for (int i = 0; i < texunit::kSdfMaxModels; ++i)
        shader_.setInt("modelDistanceFields[" + std::to_string(i) + "]", texunit::kSdfModelBase + i);

    blurShader_.use();
    blurShader_.setInt("sdfOcclusionInput", 0);
}

void SdfOcclusionPass::uploadSceneUbo(const SceneModels &models) {
    struct SdfSceneBlock { // UBO として送信する構造体
        glm::vec4 boxCenters[kSdfMaxBoxes];
        glm::vec4 wallCenters[2];
        glm::vec4 boxHalfSize;
        glm::vec4 wallHalfSize;
        glm::vec4 sceneParams;
        glm::mat4 modelWorldToLocalMatrices[texunit::kSdfMaxModels];
        glm::vec4 modelBoundsMin[texunit::kSdfMaxModels];
        glm::vec4 modelBoundsMax[texunit::kSdfMaxModels];
        glm::ivec4 modelCounts;
    } block{};

    if (layout::cubePositions.size() > kSdfMaxBoxes)
        throw std::runtime_error("Too many cubes for the SDF UBO");
    constexpr float kUnusedBoxFarAway = 1e5f; // 未使用の箱は遠方へ飛ばす（見えなくする）
    for (auto &center : block.boxCenters) {
        center = glm::vec4(kUnusedBoxFarAway);
    }
    for (std::size_t i = 0; i < layout::cubePositions.size(); ++i)
        block.boxCenters[i] = glm::vec4(layout::cubePositions[i], 0.0f);
    block.boxHalfSize = glm::vec4(0.5f);

    // 厚さゼロの板ポリは内外が定義できないので薄い厚みで近似する
    constexpr float wallHalfThickness = 0.1f;
    constexpr float wallCenterY = (units::floorY + units::wallTopY) * 0.5f;
    constexpr float wallHalfHeight = (units::wallTopY - units::floorY) * 0.5f;
    block.wallCenters[0] = glm::vec4(0.0f, wallCenterY, -units::floorHalfExtent - wallHalfThickness, 0.0f);
    block.wallCenters[1] = glm::vec4(0.0f, wallCenterY, units::floorHalfExtent + wallHalfThickness, 0.0f);
    block.wallHalfSize = glm::vec4(units::floorHalfExtent, wallHalfHeight, wallHalfThickness, 0.0f);

    // w はレイがシーンを確実に抜けきる距離（床の横幅）
    block.sceneParams = glm::vec4(units::floorY, static_cast<float>(layout::cubePositions.size()), 2.0f,
                                  units::floorHalfExtent * 2.0f);

    const std::vector<StaticSdfInstance> sdfInstances = models.CollectStaticSdfInstances();
    if (sdfInstances.size() > static_cast<std::size_t>(texunit::kSdfMaxModels))
        throw std::runtime_error("Too many static SDF models for the SDF UBO");
    // 箱と違い 個数をシェーダーへ渡してループ回数を切るので 未使用枠は読まれず初期値のままでよい
    block.modelCounts = glm::ivec4(static_cast<int>(sdfInstances.size()), 0, 0, 0);

    for (std::size_t i = 0; i < sdfInstances.size(); ++i) {
        const StaticSdfInstance &instance = sdfInstances[i];
        block.modelWorldToLocalMatrices[i] = instance.worldToLocalMatrix;
        block.modelBoundsMin[i] = glm::vec4(instance.boundsMin, 0.0f);
        block.modelBoundsMax[i] = glm::vec4(instance.boundsMax, 0.0f);

        // 焼いたテクスチャは Model が所有し続ける ここでは対応するユニットへバインドするだけ
        // kSdfModelBase 以降は他のパスが触らないので 起動時に一度バインドすれば以後そのまま使える
        glActiveTexture(GL_TEXTURE0 + texunit::kSdfModelBase + static_cast<int>(i));
        glBindTexture(GL_TEXTURE_3D, instance.textureId);
    }
    glActiveTexture(GL_TEXTURE0);

    sceneUBO_.create();
    glBindBuffer(GL_UNIFORM_BUFFER, sceneUBO_);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(block), &block, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, kSceneUboBinding, sceneUBO_);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void SdfOcclusionPass::Execute(const OcclusionTarget &target, const GBuffer &gbuffer, const NoiseTexture &noise,
                               const SceneGeometry &geometry, const Camera &camera, const RenderSettings &settings) {
    glViewport(0, 0, target.Width(), target.Height());
    glBindFramebuffer(GL_FRAMEBUFFER, target.Fbo());
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0 + texunit::kGPosition);
    glBindTexture(GL_TEXTURE_2D, gbuffer.Position());
    glActiveTexture(GL_TEXTURE0 + texunit::kGNormal);
    glBindTexture(GL_TEXTURE_2D, gbuffer.Normal());
    glActiveTexture(GL_TEXTURE0 + texunit::kSdfGAlbedoRoughness);
    glBindTexture(GL_TEXTURE_2D, gbuffer.AlbedoRoughness());
    glActiveTexture(GL_TEXTURE0 + texunit::kNoise);
    glBindTexture(GL_TEXTURE_2D, noise.Get());
    shader_.use();
    shader_.setVec3("viewPos", camera.GetViewPosition());
    // UI から変わる値なので毎フレーム送る
    shader_.setFloat("sdfOcclusionStrength", settings.sdfOcclusionStrength);
    shader_.setBool("debugShowSteps", settings.debugMode == kDebugModeStepCount);
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
