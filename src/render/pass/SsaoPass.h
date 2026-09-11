#pragma once
#include "gl/NoiseTexture.h"
#include "gl/Shader.h"
#include "render/SceneGeometry.h"
#include "render/targets/GBuffer.h"
#include "render/targets/OcclusionTarget.h"

namespace gl {

/**
 * @brief G-Buffer から近距離の遮蔽率を求め ノイズを均すブラーまで掛ける
 */
class SsaoPass {
  public:
    SsaoPass();

    void Execute(const OcclusionTarget &target, const GBuffer &gbuffer, const NoiseTexture &noise,
                 const SceneGeometry &geometry);

  private:
    Shader shader_;
    Shader blurShader_;
};

} // namespace gl
