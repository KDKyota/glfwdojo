#pragma once
#include "gl/Shader.h"
#include "render/RenderSettings.h"
#include "render/RenderView.h"
#include "render/SceneGeometry.h"
#include "render/targets/GBuffer.h"
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
     * @param clipPlane 断片を捨てる境界面 既定値は全ての断片を通す
     */
    void Execute(const GBuffer &gbuffer, const SceneGeometry &geometry, const SceneModels &models,
                 const RenderView &view, const RenderSettings &settings, float heightScale, std::size_t windowCount,
                 const glm::vec4 &clipPlane = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

  private:
    Shader floorShader_;
    Shader wallShader_;
    Shader windowShader_;
    Shader cubeShader_;
    Shader modelShader_;
};

} // namespace gl
