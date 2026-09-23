#include "render/targets/ReflectionTarget.h"

#include "gl/RenderTarget.h"

gl::ReflectionTarget::ReflectionTarget(int width, int height) : width_(width), height_(height) {
    fbo_.create();
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    AttachColorTarget(color_, GL_COLOR_ATTACHMENT0, GL_RGBA16F, GL_RGBA, GL_FLOAT, width, height, GL_LINEAR);
    // roughness に応じたぼかしを textureLod で引くためミップ付きに変える AttachColorTarget 直後は GL_LINEAR のまま
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    AttachDepthStencilBuffer(rbo_, width, height);
    CheckFramebufferComplete("REFLECTION");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
