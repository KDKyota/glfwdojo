#include "render/targets/ReflectionTarget.h"

#include "gl/RenderTarget.h"

gl::ReflectionTarget::ReflectionTarget(int width, int height) : width_(width), height_(height) {
    fbo_.create();
    downSampleFbo_.create();
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    AttachColorTarget(color_, GL_COLOR_ATTACHMENT0, GL_RGBA16F, GL_RGBA, GL_FLOAT, width, height, GL_LINEAR);
    // roughness に応じたぼかしを textureLod で引くためミップ付きに変える AttachColorTarget 直後は GL_LINEAR のまま
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 5); // ミップマップを作る最大レベルを指定 注意：deferred_lighting.frag の最大レベルと一致すること
    glGenerateMipmap(GL_TEXTURE_2D);
    AttachDepthStencilBuffer(rbo_, width, height);
    CheckFramebufferComplete("REFLECTION");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
