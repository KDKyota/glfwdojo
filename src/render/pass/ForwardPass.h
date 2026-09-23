#pragma once
#include "debug/CollisionDebugDraw.h"
#include "gl/Shader.h"
#include "render/RenderSettings.h"
#include "render/SceneGeometry.h"
#include "render/ibl/IblMaps.h"
#include "render/targets/HdrTarget.h"
#include "render/targets/ShadowCubeTargets.h"
#include "scene/Camera.h"

namespace gl {

/**
 * @brief G-Buffer に載せられないもの（ライトキューブ・空・ガラス）を前方描画する
 */
class ForwardPass {
  public:
    ForwardPass();

    /**
     * @brief HDR ターゲットへ重ねて描く
     *
     * @param windowCount 並べ替え済みの窓の枚数
     * @param character 衝突判定の可視化対象 読み込めていなければ nullptr
     */
    void Execute(const HdrTarget &hdr, const SceneGeometry &geometry, const IblMaps &ibl,
                 const ShadowCubeTargets &shadows, const Camera &camera, const RenderSettings &settings,
                 std::size_t windowCount, CollisionDebugDraw &debugDraw, const Character *character);

    /**
     * @brief 光源の位置を示すキューブを描く 平面反射からも呼ぶので公開している
     *
     * @param clipPlane 断片を捨てる境界面 既定値は全ての断片を通す
     */
    void RenderLightCubes(const SceneGeometry &geometry,
                          const glm::vec4 &clipPlane = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

  private:
    void renderSkybox(const SceneGeometry &geometry, const IblMaps &ibl);
    /// ガラス窓を透過（乗算）と反射（加算）の2パスに分けて描く
    void renderTransparentWindows(const SceneGeometry &geometry, const IblMaps &ibl,
                                  const ShadowCubeTargets &shadows, const Camera &camera,
                                  const RenderSettings &settings, std::size_t windowCount);

    Shader lightCubeShader_;
    Shader skyboxShader_;
    Shader transparentWindowShader_;
};

} // namespace gl
