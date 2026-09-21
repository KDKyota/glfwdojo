#pragma once
#include "gl/Shader.h"
#include "render/SceneGeometry.h"
#include "render/ibl/IblMaps.h"
#include "render/targets/ReflectionTarget.h"
#include "scene/Camera.h"

namespace gl {

/**
 * @brief 床で折り返した鏡像カメラからシーンを描いて ReflectionTarget へ書く
 */
class ReflectionPass {
  public:
    ReflectionPass();

    /// Matrices UBO の view を鏡像に書き換える
    void Execute(const ReflectionTarget &target, const SceneGeometry &geometry, const IblMaps &ibl, const Camera &camera,
                GLuint matricesUbo);

  private:
    Shader skyboxShader_;
};

} // namespace gl
