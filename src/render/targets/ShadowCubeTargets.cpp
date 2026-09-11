#include "render/targets/ShadowCubeTargets.h"

#include "gl/RenderTarget.h"
#include "render/ShadowConstants.h"
#include "render/TextureUnits.h"

namespace gl {

ShadowCubeTargets::ShadowCubeTargets() {
    for (std::size_t j = 0; j < layout::kPointLightCount; ++j) {
        fbo_[j].create();
        // 各テクセルに入るのは色ではなく 光源からの正規化距離 [0,1]
        CreateCubemap(depthCubemap_[j], GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT, GL_FLOAT, shadow::kMapWidth,
                      GL_NEAREST, GL_NEAREST);
        CreateCubemap(shadowColorCubemap_[j], GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, shadow::kMapWidth, GL_LINEAR,
                      GL_LINEAR);

        glBindFramebuffer(GL_FRAMEBUFFER, fbo_[j]);
        // glFramebufferTexture なら6面が1アタッチメントになり gl_Layer で面を選べる
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap_[j], 0);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, shadowColorCubemap_[j], 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowCubeTargets::BindDepthMaps() const {
    for (std::size_t j = 0; j < layout::kPointLightCount; ++j) {
        glActiveTexture(GL_TEXTURE0 + texunit::kShadowMap + static_cast<GLenum>(j));
        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap_[j]);
    }
}

void ShadowCubeTargets::BindColorMaps() const {
    for (std::size_t j = 0; j < layout::kPointLightCount; ++j) {
        glActiveTexture(GL_TEXTURE0 + texunit::kShadowColor + static_cast<GLenum>(j));
        glBindTexture(GL_TEXTURE_CUBE_MAP, shadowColorCubemap_[j]);
    }
}

} // namespace gl
