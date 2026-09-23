#pragma once
#include <glm/glm.hpp>

namespace gl {

/// 描画に使う視点 Camera の位置と姿勢では表せない鏡像の視点も渡せるようにする
struct RenderView {
    glm::mat4 view;
    glm::vec3 position;
};

} // namespace gl
