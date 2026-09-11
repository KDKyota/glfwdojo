#include "render/targets/OcclusionTarget.h"

#include "gl/RenderTarget.h"

#include <string>

namespace gl {

OcclusionTarget::OcclusionTarget(int width, int height, const char *debugName) : width_(width), height_(height) {
    // 遮蔽率はスカラー [0,1] なので 1チャンネル 8bit
    fbo_.create();
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    AttachColorTarget(buffer_, GL_COLOR_ATTACHMENT0, GL_R8, GL_RED, GL_FLOAT, width, height, GL_LINEAR);
    CheckFramebufferComplete(debugName);

    blurFbo_.create();
    glBindFramebuffer(GL_FRAMEBUFFER, blurFbo_);
    AttachColorTarget(blurredBuffer_, GL_COLOR_ATTACHMENT0, GL_R8, GL_RED, GL_FLOAT, width, height, GL_LINEAR);
    CheckFramebufferComplete((std::string(debugName) + "_BLUR").c_str());

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

} // namespace gl
