#include "render/pass/ShadowPass.h"

#include "render/ShadowConstants.h"
#include "render/TextureUnits.h"
#include "scene/SceneLayout.h"

#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>

namespace gl {

ShadowPass::ShadowPass()
    : depthShader_("point_shadow_depth.vert", "point_shadow_depth.geom", "point_shadow_depth.frag"),
      // vert / geom は深度パスと共用し frag だけ差し替える
      colorShader_("point_shadow_depth.vert", "point_shadow_depth.geom", "point_shadow_color.frag") {
    depthShader_.use();
    depthShader_.setInt("diffuseMap", texunit::kDiffuseMap);
    colorShader_.use();
    colorShader_.setInt("diffuseMap", texunit::kDiffuseMap);
}

// 深度とガラスの透過色を同じ FBO へ glDrawBuffer で書き込み先を切り替えて作る
void ShadowPass::Execute(const ShadowCubeTargets &targets, const SceneGeometry &geometry, const SceneModels &models,
                         std::size_t windowCount, bool staticCasters) {
    glViewport(0, 0, shadow::kMapWidth, shadow::kMapHeight);
    glEnable(GL_DEPTH_TEST);
    for (std::size_t j = 0; j < layout::kPointLightCount; ++j) {
        glm::vec3 lightPos = layout::pointLights[j].position;
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f),
                                                (float)shadow::kMapWidth / (float)shadow::kMapHeight,
                                                shadow::kNearPlane, shadow::kFarPlane);
        std::vector<glm::mat4> shadowTransforms;
        shadowTransforms.push_back(shadowProj *
                                   glm::lookAt(lightPos, lightPos + glm::vec3(1, 0, 0), glm::vec3(0, -1, 0)));
        shadowTransforms.push_back(shadowProj *
                                   glm::lookAt(lightPos, lightPos + glm::vec3(-1, 0, 0), glm::vec3(0, -1, 0)));
        shadowTransforms.push_back(shadowProj *
                                   glm::lookAt(lightPos, lightPos + glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)));
        shadowTransforms.push_back(shadowProj *
                                   glm::lookAt(lightPos, lightPos + glm::vec3(0, -1, 0), glm::vec3(0, 0, -1)));
        shadowTransforms.push_back(shadowProj *
                                   glm::lookAt(lightPos, lightPos + glm::vec3(0, 0, 1), glm::vec3(0, -1, 0)));
        shadowTransforms.push_back(shadowProj *
                                   glm::lookAt(lightPos, lightPos + glm::vec3(0, 0, -1), glm::vec3(0, -1, 0)));

        /* ── Pass 1: Point Shadow Depth Pass ── */
        glBindFramebuffer(GL_FRAMEBUFFER, targets.Fbo(j));
        // カラーサブパスと交互に使うので ループ内で毎回 use() しないと uniform の送り先がずれる
        depthShader_.use();
        glDrawBuffer(GL_NONE);
        glClear(GL_DEPTH_BUFFER_BIT);
        for (int i = 0; i < 6; ++i)
            depthShader_.setMat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
        depthShader_.setFloat("farPlane", shadow::kFarPlane);
        depthShader_.setVec3("lightPos", lightPos);
        depthShader_.setBool("useAlphaTest", false);
        if (staticCasters) { // 静的形状の影は SDF が担当する
            geometry.DrawFloor(depthShader_);
            geometry.DrawCubes(depthShader_);
            geometry.DrawWalls(depthShader_);
        }
        models.Draw(depthShader_);
        depthShader_.setBool("useAlphaTest", true);
        geometry.DrawWindows(depthShader_, windowCount);

        /* ── Pass 1.5: Point Shadow Color Pass ── */
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // ガラスを通らない方向 = 減衰なし
        glClear(GL_COLOR_BUFFER_BIT);
        // 深度テストは残したまま書き込みだけ止め 不透明物より奥のガラスを弾く
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ZERO, GL_SRC_COLOR);
        colorShader_.use();
        for (int i = 0; i < 6; ++i)
            colorShader_.setMat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
        colorShader_.setFloat("farPlane", shadow::kFarPlane);
        colorShader_.setVec3("lightPos", lightPos);
        geometry.DrawWindows(colorShader_, windowCount);
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
        glDrawBuffer(GL_NONE);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

} // namespace gl
