#pragma once
#include "gl/Shader.h"
#include "render/RenderSettings.h"
#include "render/SceneGeometry.h"
#include "render/targets/HdrTarget.h"

namespace gl {

/**
 * @brief トーンマッピングとガンマ補正をかけてデフォルト FBO へ出力する
 */
class TonemapPass {
  public:
    TonemapPass();

    /**
     * @brief HDR と Bloom を合成して画面へ描く
     *
     * @param bloomBlur BloomPass がぼかした結果
     */
    void Execute(const HdrTarget &hdr, GLuint bloomBlur, const SceneGeometry &geometry,
                 const RenderSettings &settings);

  private:
    Shader shader_;
    Shader debugDepthShader_;
};

} // namespace gl
