#include "render/targets/HdrTarget.h"

#include "gl/RenderTarget.h"

namespace gl {

HdrTarget::HdrTarget(int width, int height) : width_(width), height_(height) {
    fbo_.create();
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    // RGB16F はカラーレンダリング可能が保証されない
    AttachColorTarget(color_, GL_COLOR_ATTACHMENT0, GL_RGBA16F, GL_RGBA, GL_FLOAT, width, height, GL_LINEAR);
    AttachColorTarget(brightColor_, GL_COLOR_ATTACHMENT1, GL_RGBA16F, GL_RGBA, GL_FLOAT, width, height, GL_LINEAR);
    unsigned int attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, attachments);

    AttachDepthStencilBuffer(rbo_, width, height);
    CheckFramebufferComplete("FRAMEBUFFER");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

} // namespace gl
