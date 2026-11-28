#pragma once
#include "gl/GlHandle.h"
#include "scene/SceneLayout.h"

namespace gl {

/// @brief 全点光源の深度と ガラスを透過した光の色をキューブマップ配列で持つ
// 光源 j の面 f は配列の layer-face j * 6 + f に入る
class ShadowCubeTargets {
  public:
    ShadowCubeTargets();

    GLuint Fbo() const {
        return fbo_;
    }

    /// @brief 深度キューブマップ配列を texunit::kShadowMap に割り当てる
    void BindDepthMaps() const;
    /// @brief 透過色キューブマップ配列を texunit::kShadowColor に割り当てる
    void BindColorMaps() const;

  private:
    FramebufferHandle fbo_;
    // 各テクセルは光源からの正規化距離
    TextureHandle depthCubemapArray_;
    // 各テクセルはガラスを透過した光の色 ガラスを通らない方向は白
    TextureHandle shadowColorCubemapArray_;
};

} // namespace gl
