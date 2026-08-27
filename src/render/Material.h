#pragma once
#include <memory>
#include "gl/Texture.h"
#include "gl/Shader.h"

/**
 * @brief Blinn-Phong 用の旧マテリアル。gl::PbrMaterial への移行後は未使用。
 */
struct Material {
  private:
    float shininess = 32.0f;

  public:
    std::shared_ptr<Texture> diffuse;
    std::shared_ptr<Texture> specular;
    void bind() const;
    void setShininess(const float shi);
    void setUniforms(gl::Shader &shader) const;
};
