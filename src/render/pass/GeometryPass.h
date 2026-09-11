#pragma once
#include "gl/Shader.h"
#include "render/RenderSettings.h"
#include "render/SceneGeometry.h"
#include "render/targets/GBuffer.h"
#include "scene/Camera.h"
#include "scene/SceneModels.h"

namespace gl {

/**
 * @brief 不透明オブジェクトの幾何情報とマテリアルを G-Buffer へ描く
 */
class GeometryPass {
  public:
    GeometryPass();

    /**
     * @brief 4種類のオブジェクトをそれぞれ専用シェーダーで G-Buffer へ描く
     *
     * @param heightScale Parallax Mapping の強さ
     * @param windowCount 並べ替え済みの窓の枚数
     */
    void Execute(const GBuffer &gbuffer, const SceneGeometry &geometry, const SceneModels &models,
                 const Camera &camera, const RenderSettings &settings, float heightScale, std::size_t windowCount);

  private:
    Shader floorShader_;
    Shader wallShader_;
    Shader windowShader_;
    Shader cubeShader_;
    Shader modelShader_;
};

} // namespace gl
