#pragma once
#include "gl/Shader.h"
#include "render/RenderSettings.h"
#include "render/SceneGeometry.h"
#include "render/ibl/IblMaps.h"
#include "render/pass/DeferredLightingPass.h"
#include "render/pass/ForwardPass.h"
#include "render/pass/GeometryPass.h"
#include "render/targets/GBuffer.h"
#include "render/targets/OcclusionTarget.h"
#include "render/targets/ReflectionTarget.h"
#include "render/targets/ShadowCubeTargets.h"
#include "scene/Camera.h"
#include "scene/SceneModels.h"

namespace gl {

/**
 * @brief 床で折り返した鏡像カメラからシーンを描いて ReflectionTarget へ書く
 *
 * 本描画とマテリアルを揃えるため専用シェーダーは持たず 既存の2パスを半解像度で走らせる
 */
class ReflectionPass {
  public:
    ReflectionPass();

    /// 注意: Matrices UBO の view を鏡像に書き換えるので呼び出し側で元に戻すこと
    void Execute(const ReflectionTarget &target, const GBuffer &reflectionGBuffer, GeometryPass &geometryPass,
                 DeferredLightingPass &lightingPass, ForwardPass &forwardPass, const SceneGeometry &geometry,
                 const SceneModels &models,
                 const ShadowCubeTargets &shadows, const OcclusionTarget &ssao, const OcclusionTarget &sdfOcclusion,
                 const IblMaps &ibl, const Camera &camera, const RenderSettings &settings, GLuint matricesUbo,
                 float heightScale, std::size_t windowCount);

  private:
    Shader skyboxShader_;
};

} // namespace gl
