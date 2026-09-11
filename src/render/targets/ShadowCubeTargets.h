#pragma once
#include "gl/GlHandle.h"
#include "scene/SceneLayout.h"
#include <array>

namespace gl {

/**
 * @brief 点光源ごとの深度キューブマップと ガラスを透過した光の色
 *
 * カラーを持たない通常の Point Shadow と違い 1つの FBO へ深度と色を書き分ける
 */
class ShadowCubeTargets {
  public:
    ShadowCubeTargets();

    GLuint Fbo(std::size_t light) const {
        return fbo_[light];
    }

    /// 深度キューブマップを texunit::kShadowMap から連番で割り当てる
    void BindDepthMaps() const;
    /// 透過色キューブマップを texunit::kShadowColor から連番で割り当てる
    void BindColorMaps() const;

  private:
    std::array<FramebufferHandle, layout::kPointLightCount> fbo_;
    // 各テクセルは光源からの正規化距離
    std::array<TextureHandle, layout::kPointLightCount> depthCubemap_;
    // 各テクセルはガラスを透過した光の色 ガラスを通らない方向は白
    std::array<TextureHandle, layout::kPointLightCount> shadowColorCubemap_;
};

} // namespace gl
