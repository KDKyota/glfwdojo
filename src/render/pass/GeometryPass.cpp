#include "render/pass/GeometryPass.h"

#include "core/SceneUnits.h"
#include "render/TextureUnits.h"

#include <glm/glm.hpp>

namespace gl {

GeometryPass::GeometryPass()
    : floorShader_("shader.vert", "gbuffer_floor.frag"), wallShader_("wall.vert", "gbuffer_wall.frag"),
      windowShader_("window.vert", "gbuffer_window.frag"), cubeShader_("cube.vert", "gbuffer_cube.frag"),
      modelShader_("gbuffer_model.vert", "gbuffer_model.frag") {
    cubeShader_.use();
    cubeShader_.setInt("diffuseMap", texunit::kDiffuseMap);
    cubeShader_.setInt("normalMap", texunit::kNormalMap);
    cubeShader_.setInt("heightMap", texunit::kHeightMap);

    floorShader_.use();
    floorShader_.setInt("diffuseMap", texunit::kDiffuseMap);

    wallShader_.use();
    wallShader_.setInt("diffuseMap", texunit::kDiffuseMap);
    wallShader_.setInt("normalMap", texunit::kNormalMap);

    windowShader_.use();
    windowShader_.setInt("diffuseMap", texunit::kDiffuseMap);
}

void GeometryPass::Execute(const GBuffer &gbuffer, const SceneGeometry &geometry, const SceneModels &models,
                           const Camera &camera, const RenderSettings &settings, float heightScale,
                           std::size_t windowCount) {
    glViewport(0, 0, gbuffer.Width(), gbuffer.Height());
    glEnable(GL_DEPTH_TEST);
    // ブレンドが有効なままだと アルファ未定義の出力は書き込みが丸ごと消える
    glDisable(GL_BLEND);

    glBindFramebuffer(GL_FRAMEBUFFER, gbuffer.Fbo());
    // 非ゼロだと ssao.frag の「法線がゼロなら背景」判定をすり抜ける
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* cube */
    // POM の穴から裏面が透けるのを防ぐ 片面ポリゴンは消えるので cube の間だけ
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    cubeShader_.use();
    cubeShader_.setVec3("viewPos", camera.GetViewPosition());
    cubeShader_.setMat3("normalMatrix", glm::mat3(1.0f));
    cubeShader_.setFloat("heightScale", heightScale);
    settings.cubeMaterial.applyToShader(cubeShader_);
    geometry.DrawCubes(cubeShader_);
    glDisable(GL_CULL_FACE);

    /* floor */
    floorShader_.use();
    floorShader_.setMat3("normalMatrix", glm::mat3(1.0f));
    settings.floorMaterial.applyToShader(floorShader_);
    floorShader_.setBool("checkerFloor", settings.debugCheckerFloor);
    floorShader_.setBool("checkerInvert", settings.debugCheckerInvert);
    floorShader_.setFloat("checkerTileSize", units::floorTileSize);
    geometry.DrawFloor(floorShader_);

    /* wall */
    wallShader_.use();
    settings.wallMaterial.applyToShader(wallShader_);
    geometry.DrawWalls(wallShader_);

    /* model */
    modelShader_.use();
    models.Draw(modelShader_);

    /* 窓枠 */
    windowShader_.use();
    settings.windowMaterial.applyToShader(windowShader_);
    geometry.DrawWindows(windowShader_, windowCount);
}

} // namespace gl
