#pragma once
#include "gl/Shader.h"
#include "render/SceneGeometry.h"
#include "render/targets/ShadowCubeTargets.h"
#include "scene/SceneModels.h"

namespace gl {

/**
 * @brief 点光源ごとに深度とガラスの透過色をキューブマップへ描く
 */
class ShadowPass {
  public:
    ShadowPass();

    /**
     * @brief 4灯ぶんの深度と透過色を焼く
     *
     * @param windowCount 並べ替え済みの窓の枚数
     * @param staticCasters 床とキューブと壁もシャドウマップへ描くか
     */
    void Execute(const ShadowCubeTargets &targets, const SceneGeometry &geometry, const SceneModels &models,
                 std::size_t windowCount, bool staticCasters);

  private:
    Shader depthShader_;
    Shader colorShader_; // カラー付き透過シャドウ（ガラスの透過色）用
};

} // namespace gl
