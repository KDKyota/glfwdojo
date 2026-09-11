#include "render/pass/DeferredLightingPass.h"

#include "render/ShadowConstants.h"
#include "render/TextureUnits.h"
#include "scene/SceneLayout.h"

#include <glm/glm.hpp>
#include <string>

namespace gl {

DeferredLightingPass::DeferredLightingPass() : shader_("fragment_quad.vert", "deferred_lighting.frag") {
    shader_.use();
    shader_.setInt("gPosition", texunit::kGPosition);
    shader_.setInt("gNormal", texunit::kGNormal);
    shader_.setInt("gAlbedoRoughness", texunit::kGAlbedoRoughness);
    for (std::size_t i = 0; i < layout::kPointLightCount; ++i)
        shader_.setInt("shadowMap[" + std::to_string(i) + "]", texunit::kShadowMap + static_cast<int>(i));
    shader_.setFloat("farPlane", shadow::kFarPlane);
    shader_.setFloat("shadowMapSize", static_cast<float>(shadow::kMapWidth));
    shader_.setInt("ssao", texunit::kSsao);
    for (std::size_t i = 0; i < layout::kPointLightCount; ++i)
        shader_.setInt("shadowColor[" + std::to_string(i) + "]", texunit::kShadowColor + static_cast<int>(i));
    shader_.setInt("irradianceMap", texunit::kIrradianceMap);
    shader_.setInt("prefilterMap", texunit::kPrefilterMap);
    shader_.setInt("brdfLUT", texunit::kBrdfLut);
    shader_.setInt("sdfOcclusion", texunit::kSdfOcclusion);
}

void DeferredLightingPass::Execute(const HdrTarget &hdr, const GBuffer &gbuffer, const ShadowCubeTargets &shadows,
                                   const OcclusionTarget &ssao, const OcclusionTarget &sdfOcclusion,
                                   const IblMaps &ibl, const SceneGeometry &geometry, const Camera &camera,
                                   const RenderSettings &settings) {
    glViewport(0, 0, hdr.Width(), hdr.Height());
    glBindFramebuffer(GL_FRAMEBUFFER, hdr.Fbo());
    glClear(GL_COLOR_BUFFER_BIT); // 深度は GBuffer::BlitDepthTo() でコピー済み
    glDisable(GL_DEPTH_TEST);

    gbuffer.BindTextures();
    shadows.BindDepthMaps();
    glActiveTexture(GL_TEXTURE0 + texunit::kSsao);
    glBindTexture(GL_TEXTURE_2D, ssao.BlurredBuffer());
    shadows.BindColorMaps();
    glActiveTexture(GL_TEXTURE0 + texunit::kIrradianceMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, ibl.irradianceMap);
    glActiveTexture(GL_TEXTURE0 + texunit::kPrefilterMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, ibl.prefilterMap);
    glActiveTexture(GL_TEXTURE0 + texunit::kBrdfLut);
    glBindTexture(GL_TEXTURE_2D, ibl.brdfLut);
    glActiveTexture(GL_TEXTURE0 + texunit::kSdfOcclusion);
    glBindTexture(GL_TEXTURE_2D, sdfOcclusion.BlurredBuffer());

    shader_.use();
    shader_.setVec3("viewPos", camera.GetViewPosition());
    // UI から変わる値なので毎フレーム送る
    shader_.setInt("debugMode", settings.debugMode);
    shader_.setFloat("ssaoStrength", settings.ssaoStrength);
    shader_.setFloat("ambientStrength", settings.ambientStrength);
    shader_.setFloat("directLightStrength", settings.directLightStrength);
    shader_.setFloat("sdfOcclusionStrength", settings.sdfOcclusionStrength);
    shader_.setFloat("sdfShadowStrength", settings.sdfShadowStrength);
    const float azimuth = glm::radians(settings.sdfDebugAzimuthDegrees);
    const float elevation = glm::radians(settings.sdfDebugElevationDegrees);
    shader_.setVec3(("sdfDebugDir"), {glm::cos(elevation) * glm::cos(azimuth), glm::sin(elevation),
                                      glm::cos(elevation) * glm::sin(azimuth)});
    applyPointLights(shader_, layout::pointLights.data(), layout::pointLights.size());
    geometry.DrawScreenQuad();
    glEnable(GL_DEPTH_TEST);
}

} // namespace gl
