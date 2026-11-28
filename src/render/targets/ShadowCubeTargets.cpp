#include "render/targets/ShadowCubeTargets.h"

#include "gl/RenderTarget.h"
#include "render/ShadowConstants.h"
#include "render/TextureUnits.h"

namespace gl {

ShadowCubeTargets::ShadowCubeTargets() {
    const int lightCount = static_cast<int>(layout::kPointLightCount);
    // 各テクセルに入るのは色ではなく 光源からの正規化距離 [0,1]
    CreateCubemapArray(depthCubemapArray_, GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT, GL_FLOAT, shadow::kMapWidth,
                       lightCount, GL_NEAREST, GL_NEAREST);
    CreateCubemapArray(shadowColorCubemapArray_, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, shadow::kMapWidth, lightCount,
                       GL_LINEAR, GL_LINEAR);

    fbo_.create();
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    // レイヤー付きアタッチメント 配列の全ての面を1つのアタッチメントとして繋ぎ 書き込む面は gl_Layer で選ぶ
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemapArray_, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, shadowColorCubemapArray_, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    CheckFramebufferComplete("SHADOW_CUBE_ARRAY");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowCubeTargets::BindDepthMaps() const {
    glActiveTexture(GL_TEXTURE0 + texunit::kShadowMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, depthCubemapArray_);
}

void ShadowCubeTargets::BindColorMaps() const {
    glActiveTexture(GL_TEXTURE0 + texunit::kShadowColor);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, shadowColorCubemapArray_);
}

} // namespace gl
