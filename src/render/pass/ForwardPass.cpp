#include "render/pass/ForwardPass.h"

#include "render/ShadowConstants.h"
#include "render/TextureUnits.h"
#include "scene/SceneLayout.h"

#include <glm/gtc/matrix_transform.hpp>
#include <string>

namespace gl {

ForwardPass::ForwardPass()
    : lightCubeShader_("light_cube.vert", "light_cube.frag"), skyboxShader_("skybox.vert", "skybox.frag"),
      transparentWindowShader_("window.vert", "glass.frag") {
    transparentWindowShader_.use();
    transparentWindowShader_.setInt("diffuseMap", texunit::kDiffuseMap);
    for (std::size_t i = 0; i < layout::kPointLightCount; ++i)
        transparentWindowShader_.setInt("shadowMap[" + std::to_string(i) + "]",
                                        texunit::kShadowMap + static_cast<int>(i));
    // deferredLightingShader_ と割り当てを揃える
    for (std::size_t i = 0; i < layout::kPointLightCount; ++i)
        transparentWindowShader_.setInt("shadowColor[" + std::to_string(i) + "]",
                                        texunit::kShadowColor + static_cast<int>(i));
    transparentWindowShader_.setInt("prefilterMap", texunit::kPrefilterMap);
    transparentWindowShader_.setInt("brdfLUT", texunit::kBrdfLut);
    transparentWindowShader_.setFloat("farPlane", shadow::kFarPlane);
    transparentWindowShader_.setFloat("shadowMapSize", static_cast<float>(shadow::kMapWidth));

    skyboxShader_.use();
    skyboxShader_.setInt("skybox", 0);
}

void ForwardPass::Execute(const HdrTarget &hdr, const SceneGeometry &geometry, const IblMaps &ibl,
                          const ShadowCubeTargets &shadows, const Camera &camera, const RenderSettings &settings,
                          std::size_t windowCount, CollisionDebugDraw &debugDraw, const Character *character) {
    glViewport(0, 0, hdr.Width(), hdr.Height());
    glBindFramebuffer(GL_FRAMEBUFFER, hdr.Fbo());
    glEnable(GL_DEPTH_TEST);

    renderLightCubes(geometry);
    renderSkybox(geometry, ibl);
    if (settings.debugCollision)
        debugDraw.Draw(character);

    /* 透過窓（ブレンドが必要なのはここだけ Geometryパスの冒頭で無効化しているので 描画中だけ有効にする）*/
    glEnable(GL_BLEND);
    renderTransparentWindows(geometry, ibl, shadows, camera, settings, windowCount);
    glDisable(GL_BLEND);
}

void ForwardPass::renderLightCubes(const SceneGeometry &geometry) {
    lightCubeShader_.use();
    for (const auto &pointLight : layout::pointLights) {
        lightCubeShader_.setVec3("lightColor", pointLight.diffuse);
        glm::mat4 lightModel = glm::translate(glm::mat4(1.0f), pointLight.position);
        lightModel = glm::scale(lightModel, glm::vec3(0.2f));
        lightCubeShader_.setMat4("model", lightModel);
        geometry.DrawCubeMesh();
    }
}

void ForwardPass::renderSkybox(const SceneGeometry &geometry, const IblMaps &ibl) {
    glDepthFunc(GL_LEQUAL);
    skyboxShader_.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, ibl.envCubemap);
    geometry.DrawSkyboxMesh();
    glDepthFunc(GL_LESS);
}

void ForwardPass::renderTransparentWindows(const SceneGeometry &geometry, const IblMaps &ibl,
                                           const ShadowCubeTargets &shadows, const Camera &camera,
                                           const RenderSettings &settings, std::size_t windowCount) {
    transparentWindowShader_.use();
    transparentWindowShader_.setVec3("viewPos", camera.GetViewPosition());
    transparentWindowShader_.setMat3("normalMatrix", glm::mat3(1.0f));
    transparentWindowShader_.setFloat("ambientStrength", settings.ambientStrength);
    transparentWindowShader_.setFloat("directLightStrength", settings.directLightStrength);
    transparentWindowShader_.setFloat("sdfOcclusionStrength", settings.sdfOcclusionStrength);
    transparentWindowShader_.setFloat("sdfShadowStrength", settings.sdfShadowStrength);
    settings.glassMaterial.applyToShader(transparentWindowShader_);
    shadows.BindDepthMaps();
    shadows.BindColorMaps();
    glActiveTexture(GL_TEXTURE0 + texunit::kPrefilterMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, ibl.prefilterMap);
    glActiveTexture(GL_TEXTURE0 + texunit::kBrdfLut);
    glBindTexture(GL_TEXTURE_2D, ibl.brdfLut);
    applyPointLights(transparentWindowShader_, layout::pointLights.data(), layout::pointLights.size());

    glDepthMask(GL_FALSE);
    glBlendFunc(GL_ZERO, GL_SRC_COLOR);
    transparentWindowShader_.setBool("reflectionPass", false);
    geometry.DrawWindows(transparentWindowShader_, windowCount);

    glBlendFunc(GL_ONE, GL_ONE);
    transparentWindowShader_.setBool("reflectionPass", true);
    geometry.DrawWindows(transparentWindowShader_, windowCount);
    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

} // namespace gl
