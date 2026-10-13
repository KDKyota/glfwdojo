#pragma once
#include "gl/RenderTarget.h"
#include "gl/Shader.h"
#include "render/RenderSettings.h"
#include "render/RenderView.h"
#include "render/SceneGeometry.h"
#include "render/ibl/IblMaps.h"
#include "render/targets/GBuffer.h"
#include "render/targets/OcclusionTarget.h"
#include "render/targets/ShadowCubeTargets.h"

namespace gl {

/**
 * @brief G-Buffer と影と遮蔽と IBL を合成して HDR ターゲットへ書く
 */
class DeferredLightingPass {
  public:
    DeferredLightingPass();

    /**
     * @brief G-Buffer と影と遮蔽と IBL を合成して HDR ターゲットへ書く
     *
     * @param target 合成結果の出力先
     * @param reflectionColor 床に合成する平面反射のテクスチャ 0 なら床は通常の IBL のまま
     */
    void Execute(const TargetView &target, const GBuffer &gbuffer, const ShadowCubeTargets &shadows,
                 const OcclusionTarget &ssao, const OcclusionTarget &sdfOcclusion, const IblMaps &ibl,
                 const SceneGeometry &geometry, const RenderView &view, const RenderSettings &settings,
                 GLuint reflectionColor = 0);

  private:
    Shader shader_;
};

} // namespace gl
