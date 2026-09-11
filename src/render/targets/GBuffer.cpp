#include "render/targets/GBuffer.h"

#include "gl/RenderTarget.h"
#include "render/TextureUnits.h"

namespace gl {

GBuffer::GBuffer(int width, int height) : width_(width), height_(height) {
    fbo_.create();
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

    // GL_RGB16F は禁止 こうしないと GPU によっては書き込めない
    AttachColorTarget(position_, GL_COLOR_ATTACHMENT0, GL_RGBA16F, GL_RGBA, GL_FLOAT, width, height, GL_NEAREST);
    // 法線は [-1,1] の負値を持つので浮動小数点フォーマット
    AttachColorTarget(normal_, GL_COLOR_ATTACHMENT1, GL_RGBA16F, GL_RGBA, GL_FLOAT, width, height, GL_NEAREST);
    // アルベドも roughness も [0,1] なので 8bit で足りる
    AttachColorTarget(albedoRoughness_, GL_COLOR_ATTACHMENT2, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, width, height,
                      GL_NEAREST);

    unsigned int attachments[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
    glDrawBuffers(3, attachments);

    AttachDepthStencilBuffer(depthRBO_, width, height);
    CheckFramebufferComplete("G-BUFFER");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GBuffer::BindTextures() const {
    glActiveTexture(GL_TEXTURE0 + texunit::kGPosition);
    glBindTexture(GL_TEXTURE_2D, position_);
    glActiveTexture(GL_TEXTURE0 + texunit::kGNormal);
    glBindTexture(GL_TEXTURE_2D, normal_);
    glActiveTexture(GL_TEXTURE0 + texunit::kGAlbedoRoughness);
    glBindTexture(GL_TEXTURE_2D, albedoRoughness_);
}

void GBuffer::BlitDepthTo(GLuint dst) const {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo_);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst);
    glBlitFramebuffer(0, 0, width_, height_, 0, 0, width_, height_, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

} // namespace gl
