#pragma once
#include <glm/glm.hpp>

/**
 * @brief オブジェクトの Local → World 変換（TRS）を保持し、Model/Normal Matrix を遅延計算する。
 */
class Transform {
  private:
    glm::vec3 position_;
    glm::vec3 rotation_;
    glm::vec3 scale_;
    glm::mat4 modelMatrix_;
    // Model Matrix の逆転置行列。非一様スケールでも法線を正しく変換するために必要。
    glm::mat3 normalMatrix_;
    bool dirty_ = true;

    /// modelMatrix_ と normalMatrix_ を再計算する。
    void rebuild();

  public:
    void setPosition(glm::vec3 p);
    void setRotation(glm::vec3 r);
    void setScale(glm::vec3 s);

    /// Model Matrix を返す。
    const glm::mat4 &getModelMatrix();
    /// Normal Matrix を返す。
    const glm::mat3 &getNormalMatrix();
};
