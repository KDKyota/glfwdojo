#include "render/SceneGeometry.h"

#include "render/GeometryData.h"
#include "render/TextureUnits.h"
#include "scene/SceneLayout.h"

#include <cstddef>

namespace gl {

SceneGeometry::SceneGeometry(TextureCache &cache) {
    initMesh();
    initTextures(cache);
}

void SceneGeometry::initMesh() {
    int stride = sizeof(Vertex);

    /* cube */
    cubeVAO_.create();
    cubeVBO_.create();
    cubeInstanceVBO_.create();
    cubeEBO_.create();
    glBindVertexArray(cubeVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEBO_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(Vertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(Vertex, uv));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(Vertex, tangent));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(Vertex, bitangent));
    glBindBuffer(GL_ARRAY_BUFFER, cubeInstanceVBO_);
    glBufferData(GL_ARRAY_BUFFER, layout::cubePositions.size() * sizeof(glm::vec3), layout::cubePositions.data(),
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);
    // location 5 はインスタンスごとに進む
    glVertexAttribDivisor(5, 1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    /* 床 */
    planeVAO_.create();
    planeVBO_.create();
    planeEBO_.create();
    glBindVertexArray(planeVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, planeEBO_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(planeIndices), planeIndices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(Vertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(Vertex, uv));
    glBindVertexArray(0);

    /* 透過窓 */
    transparentVAO_.create();
    transparentVBO_.create();
    transparentInstanceVBO_.create();
    transparentEBO_.create();
    glBindVertexArray(transparentVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, transparentVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, transparentEBO_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(transparentIndices), transparentIndices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(Vertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(Vertex, uv));
    glBindBuffer(GL_ARRAY_BUFFER, transparentInstanceVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * layout::windowPositions.size(), nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);
    glVertexAttribDivisor(5, 1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    /* skybox */
    skyboxVAO_.create();
    skyboxVBO_.create();
    glBindVertexArray(skyboxVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO_);
    glBufferData(GL_ARRAY_BUFFER, skyboxVertices.size() * sizeof(glm::vec3), skyboxVertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(skyboxVertices[0]), (void *)0);
    glBindVertexArray(0);

    /* 壁 */
    // Normal Mapping のため location 0-4 をすべて使う
    wallVAO_.create();
    wallVBO_.create();
    wallEBO_.create();
    glBindVertexArray(wallVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, wallVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(wallVertices), wallVertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wallEBO_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(wallIndices), wallIndices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(Vertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(Vertex, uv));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(Vertex, tangent));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(Vertex, bitangent));
    glBindVertexArray(0);

    /* screen quad */
    quadVAO_.create();
    quadVBO_.create();
    glBindVertexArray(quadVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glBindVertexArray(0);
}

void SceneGeometry::initTextures(TextureCache &cache) {
    cubeTexture_ = cache.get("resources/textures/bricks2.jpg", true, ColorSpace::SRGB);
    cubeNormalMap_ = cache.get("resources/textures/bricks2_normal.jpg", true, ColorSpace::Linear);
    cubeHeightMap_ = cache.get("resources/textures/bricks2_disp.jpg", true, ColorSpace::Linear);

    floorTexture_ = cache.get("resources/textures/wood.png", true, ColorSpace::SRGB);
    transparentTexture_ = cache.get("resources/textures/window.png", true, ColorSpace::SRGB);
    brickwallTexture_ = cache.get("resources/textures/brickwall.jpg", true, ColorSpace::SRGB);
    brickwallNormalTexture_ = cache.get("resources/textures/brickwall_normal.jpg", true, ColorSpace::Linear);
}

void SceneGeometry::DrawCubes(Shader &shader) const {
    cubeTexture_->bind(texunit::kDiffuseMap);
    cubeNormalMap_->bind(texunit::kNormalMap);
    cubeHeightMap_->bind(texunit::kHeightMap);
    glBindVertexArray(cubeVAO_);
    shader.setMat4("model", glm::mat4(1.0f));
    glDrawElementsInstanced(GL_TRIANGLES, cubeIndices.size(), GL_UNSIGNED_INT, 0, layout::cubePositions.size());
}

void SceneGeometry::DrawFloor(Shader &shader) const {
    floorTexture_->bind(texunit::kDiffuseMap);
    glBindVertexArray(planeVAO_);
    shader.setMat4("model", glm::mat4(1.0f));
    glDrawElements(GL_TRIANGLES, planeIndices.size(), GL_UNSIGNED_INT, 0);
}

void SceneGeometry::DrawWalls(Shader &shader) const {
    brickwallTexture_->bind(texunit::kDiffuseMap);
    brickwallNormalTexture_->bind(texunit::kNormalMap);

    glBindVertexArray(wallVAO_);
    shader.setMat4("model", glm::mat4(1.0f));
    glDrawElements(GL_TRIANGLES, wallIndices.size(), GL_UNSIGNED_INT, 0);
}

void SceneGeometry::DrawWindows(Shader &shader, std::size_t instanceCount) const {
    shader.setMat4("model", glm::mat4(1.0f));
    shader.setMat3("normalMatrix", glm::mat3(1.0f));

    glBindVertexArray(transparentVAO_);
    transparentTexture_->bind(texunit::kDiffuseMap);
    glDrawElementsInstanced(GL_TRIANGLES, transparentIndices.size(), GL_UNSIGNED_INT, 0,
                            static_cast<GLsizei>(instanceCount));
}

void SceneGeometry::DrawCubeMesh() const {
    glBindVertexArray(cubeVAO_);
    glDrawElements(GL_TRIANGLES, cubeIndices.size(), GL_UNSIGNED_INT, 0);
}

void SceneGeometry::DrawSkyboxMesh() const {
    glBindVertexArray(skyboxVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void SceneGeometry::DrawScreenQuad() const {
    glBindVertexArray(quadVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void SceneGeometry::UploadWindowInstances(const std::vector<glm::vec3> &positions) const {
    glBindBuffer(GL_ARRAY_BUFFER, transparentInstanceVBO_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3) * positions.size(), positions.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

} // namespace gl
