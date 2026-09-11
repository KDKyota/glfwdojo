#pragma once
#include "gl/GlHandle.h"
#include "gl/NoiseTexture.h"
#include "gl/Shader.h"
#include "render/SceneGeometry.h"
#include "render/targets/GBuffer.h"
#include "render/targets/OcclusionTarget.h"

namespace gl {

/**
 * @brief SSAO が届かない数m規模の遮蔽を SDF のレイマーチで求める
 */
class SdfOcclusionPass {
  public:
    SdfOcclusionPass();

    void Execute(const OcclusionTarget &target, const GBuffer &gbuffer, const NoiseTexture &noise,
                 const SceneGeometry &geometry);

  private:
    /// シーン形状を箱の集合として UBO へ焼く レイマーチ中は変化しない
    void uploadSceneUbo();

    Shader shader_;
    Shader blurShader_;
    BufferHandle sceneUBO_;
};

} // namespace gl
