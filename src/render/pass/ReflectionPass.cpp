#include "render/pass/ReflectionPass.h"
#include "core/SceneUnits.h"
#include "gl/Shader.h"
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
} // namespace

ReflectionPass::ReflectionPass() : skyboxShader_("skybox.vert", "skybox.frag") {
    skyboxShader_.use();
    skyboxShader_.setInt("skybox", 0);
}

void ReflectionPass::Execute(const ReflectionTarget &target, const SceneGeometry &geometry, const IblMaps &ibl,
                             const Camera &camera, GLuint matricesUbo) {
    // 先にワールドを折り返してから通常の view をかける
    const glm::mat4 mirroredView = camera.GetViewMatrix() * MakeMirrorMatrix(units::floorY);
    glBindBuffer(GL_UNIFORM_BUFFER, matricesUbo);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(mirroredView));
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glViewport(0, 0, target.Width(), target.Height());
    glBindFramebuffer(GL_FRAMEBUFFER, target.Fbo());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    glDepthFunc(GL_LEQUAL);
    skyboxShader_.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, ibl.envCubemap);
    geometry.DrawSkyboxMesh();
    glDepthFunc(GL_LESS);
}
} // namespace gl
