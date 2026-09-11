#include "debug/CollisionDebugDraw.h"

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

namespace gl {

CollisionDebugDraw::CollisionDebugDraw() : shader_("debug_line.vert", "debug_line.frag") {
    constexpr int segments = 24;
    constexpr float twoPi = 6.28318530717958647692f;
    // 半径 1 高さ 1 の円柱を作り 描画時にスケールで実寸へ合わせる
    std::vector<glm::vec3> lines;
    for (int i = 0; i < segments; ++i) {
        const float a0 = twoPi * i / segments;
        const float a1 = twoPi * (i + 1) / segments;
        const glm::vec3 bottom0(std::cos(a0), 0.0f, std::sin(a0));
        const glm::vec3 bottom1(std::cos(a1), 0.0f, std::sin(a1));
        const glm::vec3 up(0.0f, 1.0f, 0.0f);

        lines.push_back(bottom0);
        lines.push_back(bottom1);
        lines.push_back(bottom0 + up);
        lines.push_back(bottom1 + up);
        if (i % 6 == 0) {
            lines.push_back(bottom0);
            lines.push_back(bottom0 + up);
        }
    }
    cylinderVertexCount_ = static_cast<int>(lines.size());

    cylinderVAO_.create();
    cylinderVBO_.create();
    glBindVertexArray(cylinderVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, cylinderVBO_);
    glBufferData(GL_ARRAY_BUFFER, lines.size() * sizeof(glm::vec3), lines.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);
    glBindVertexArray(0);
}

void CollisionDebugDraw::Draw(const Character *character) {
    if (!character)
        return;

    glm::mat4 model = glm::translate(glm::mat4(1.0f), character->Position());
    model = glm::scale(model, glm::vec3(character->Radius(), character->Height(), character->Radius()));

    shader_.use();
    shader_.setMat4("model", model);
    shader_.setVec3("color", glm::vec3(0.1f, 1.0f, 0.3f));
    glBindVertexArray(cylinderVAO_);
    glDrawArrays(GL_LINES, 0, cylinderVertexCount_);
    glBindVertexArray(0);
}

} // namespace gl
