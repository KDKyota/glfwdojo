#include "render/pass/TonemapPass.h"

#include "render/ShadowConstants.h"
#include "render/TextureUnits.h"

namespace gl {

TonemapPass::TonemapPass()
    : shader_("fragment_quad.vert", "hdr.frag"), debugDepthShader_("fragment_quad.vert", "debug_depth.frag") {
    shader_.use();
    shader_.setInt("screenTexture", texunit::kScreenTexture);
    shader_.setInt("bloomBlur", texunit::kBloomBlur);

    debugDepthShader_.use();
    debugDepthShader_.setInt("depthMap", 0);
    debugDepthShader_.setFloat("nearPlane", shadow::kNearPlane);
    debugDepthShader_.setFloat("farPlane", shadow::kFarPlane);
}

void TonemapPass::Execute(const HdrTarget &hdr, GLuint bloomBlur, const SceneGeometry &geometry,
                          const RenderSettings &settings) {
    glViewport(0, 0, hdr.Width(), hdr.Height());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // 深度は clear していないので 有効なままだと前フレームの値で全画面クアッドが弾かれる
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);

    // [DEBUG] デプスマップを画面全体に表示して確認したい場合はここをアンコメント
    // debugDepthShader_.use();
    // glActiveTexture(GL_TEXTURE0);
    // glBindTexture(GL_TEXTURE_2D, depthmapTexture_);
    // geometry.DrawScreenQuad();

    shader_.use();
    shader_.setFloat("exposure", settings.exposure);
    shader_.setBool("debugRawOutput", settings.debugRawOutput);
    shader_.setFloat("bloomStrength", settings.bloomStrength);
    glActiveTexture(GL_TEXTURE0 + texunit::kScreenTexture);
    glBindTexture(GL_TEXTURE_2D, hdr.Color());
    glActiveTexture(GL_TEXTURE0 + texunit::kBloomBlur);
    glBindTexture(GL_TEXTURE_2D, bloomBlur);

    geometry.DrawScreenQuad();
}

} // namespace gl
