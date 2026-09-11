#pragma once
#include "gl/Shader.h"
#include "render/RenderSettings.h"
#include "render/SceneGeometry.h"
#include "render/ibl/IblMaps.h"
#include "render/targets/GBuffer.h"
#include "render/targets/HdrTarget.h"
#include "render/targets/OcclusionTarget.h"
#include "render/targets/ShadowCubeTargets.h"
#include "scene/Camera.h"

namespace gl {

/**
 * @brief G-Buffer と影と遮蔽と IBL を合成して HDR ターゲットへ書く
 */
class DeferredLightingPass {
  public:
    DeferredLightingPass();

    void Execute(const HdrTarget &hdr, const GBuffer &gbuffer, const ShadowCubeTargets &shadows,
                 const OcclusionTarget &ssao, const OcclusionTarget &sdfOcclusion, const IblMaps &ibl,
                 const SceneGeometry &geometry, const Camera &camera, const RenderSettings &settings);

  private:
    Shader shader_;
};

} // namespace gl
