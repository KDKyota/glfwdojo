#pragma once
#include "gl/GlHandle.h"
#include "gl/NoiseTexture.h"
#include "gl/Shader.h"
#include "render/RenderSettings.h"
#include "render/SceneGeometry.h"
#include "render/targets/GBuffer.h"
#include "render/targets/OcclusionTarget.h"
#include "scene/Camera.h"
#include "scene/SceneModels.h"

namespace gl {

/**
 * @brief SSAO が届かない数m規模の遮蔽を SDF のレイマーチで求める 拡散と鏡面の両方を半解像度で書く
 */
class SdfOcclusionPass {
  public:
    /// models の静的メッシュ距離場は起動時（このパスの初期化時）に一度だけ UBO・テクスチャユニットへ焼く
    explicit SdfOcclusionPass(const SceneModels &models);

    void Execute(const OcclusionTarget &target, const GBuffer &gbuffer, const NoiseTexture &noise,
                 const SceneGeometry &geometry, const Camera &camera, const RenderSettings &settings);

  private:
    /// シーン形状（箱・静的メッシュの距離場）を UBO とテクスチャユニットへ焼く 
    void uploadSceneUbo(const SceneModels &models);

    Shader shader_;
    Shader blurShader_;
    BufferHandle sceneUBO_;
};

} // namespace gl
