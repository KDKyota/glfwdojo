#include "render/pass/ReflectionPass.h"
#include "core/SceneUnits.h"
#include "render/RenderView.h"
#include <glm/gtc/type_ptr.hpp>

namespace gl {
namespace {

/// 床に対する反射の変換行列
glm::mat4 MakeMirrorMatrix(float planeY) {
    glm::mat4 mirror(1.0f);
    mirror[1][1] = -1.0f;         // 行列反転させる
    mirror[3][1] = 2.0f * planeY; // y = planeY で折り返すための平行移動
    return mirror;
}
/* 鏡面反射の変換行列
 * (1,  0, 0, 0)
 * (0, -1, 0, 2planeY)
 * (0,  0, 1, 0)
 * (0,  0, 0, 1)
 */

// 注意: 反射面ちょうどの断片は距離 0 で残るため 平面をわずかに持ち上げて床自身を切る側に入れる
constexpr float kFloorClipBias = 0.001f;
/// 床より上だけを反射に残す境界面
const glm::vec4 kFloorClipPlane(0.0f, 1.0f, 0.0f, -(units::floorY + kFloorClipBias));

} // namespace

ReflectionPass::ReflectionPass() : skyboxShader_("skybox.vert", "skybox.frag") {
    skyboxShader_.use();
    skyboxShader_.setInt("skybox", 0);
}

void ReflectionPass::Execute(const ReflectionTarget &target, const GBuffer &reflectionGBuffer,
                             GeometryPass &geometryPass, DeferredLightingPass &lightingPass,
                             const SceneGeometry &geometry, const SceneModels &models,
                             const ShadowCubeTargets &shadows, const OcclusionTarget &ssao,
                             const OcclusionTarget &sdfOcclusion, const IblMaps &ibl, const Camera &camera,
                             const RenderSettings &settings, GLuint matricesUbo, float heightScale,
                             std::size_t windowCount) {
    const glm::mat4 mirror = MakeMirrorMatrix(units::floorY);
    // 先にワールドを折り返してから通常の view をかける
    const RenderView mirroredView{camera.GetViewMatrix() * mirror,
                                  glm::vec3(mirror * glm::vec4(camera.GetViewPosition(), 1.0f))};

    glBindBuffer(GL_UNIFORM_BUFFER, matricesUbo);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(mirroredView.view));
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // 画面空間の遮蔽はメインビュー基準で作られているので鏡像では無効化する
    RenderSettings reflectionSettings = settings;
    reflectionSettings.ssaoStrength = 0.0f;
    reflectionSettings.sdfOcclusionStrength = 0.0f;
    reflectionSettings.sdfShadowStrength = 0.0f;
    reflectionSettings.debugMode = 0; // デバッグ表示は本描画だけに効かせる

    // 注意: 鏡像は三角形の巻き順が反転するのでカリングの表裏を入れ替える
    glFrontFace(GL_CW);
    // 注意: ここで描くシェーダーは全て gl_ClipDistance[0] を書くこと
    glEnable(GL_CLIP_DISTANCE0);
    geometryPass.Execute(reflectionGBuffer, geometry, models, mirroredView, reflectionSettings, heightScale,
                         windowCount, kFloorClipPlane);
    glDisable(GL_CLIP_DISTANCE0);
    glFrontFace(GL_CCW);

    reflectionGBuffer.BlitDepthTo(target.Fbo());
    lightingPass.Execute(target.View(), reflectionGBuffer, shadows, ssao, sdfOcclusion, ibl, geometry, mirroredView,
                         reflectionSettings);

    glViewport(0, 0, target.Width(), target.Height());
    glBindFramebuffer(GL_FRAMEBUFFER, target.Fbo());
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    skyboxShader_.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, ibl.envCubemap);
    geometry.DrawSkyboxMesh();
    glDepthFunc(GL_LESS);
}
} // namespace gl
