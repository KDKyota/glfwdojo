#include "Mesh.h"

Mesh::Mesh(std::vector<gl::Vertex> vertices, std::vector<unsigned int> indices, gl::PbrMaterial material, bool isSkinned)
    : vertices_(std::move(vertices)), indices_(std::move(indices)), material_(std::move(material)), isSkinned_(isSkinned) {
    setupMesh();
}

/// VAO を組み立て、頂点属性 location を設定する。
void Mesh::setupMesh() {
    VAO_.create();
    VBO_.create();
    EBO_.create();

    glBindVertexArray(VAO_);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_);

    glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(gl::Vertex), vertices_.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_.size() * sizeof(unsigned int), indices_.data(), GL_STATIC_DRAW);

    const GLsizei stride = sizeof(gl::Vertex);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(gl::Vertex, position));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(gl::Vertex, normal));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(gl::Vertex, uv));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(gl::Vertex, tangent));

    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(gl::Vertex, bitangent));

    // location 5 は使わない。point_shadow_depth.vert の aOffset（インスタンス位置）が
    // そこを読み、有効化していなければ既定値 (0,0,0) になるという依存に合わせている
    glEnableVertexAttribArray(6);
    // 整数として渡すので I 付き。GL_INT を glVertexAttribPointer で送ると float に変換されて壊れる
    glVertexAttribIPointer(6, MAX_BONE_INFLUENCE, GL_INT, stride, (void *)offsetof(gl::Vertex, m_BoneIDs));

    glEnableVertexAttribArray(7);
    glVertexAttribPointer(7, MAX_BONE_INFLUENCE, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(gl::Vertex, m_Weights));

    glBindVertexArray(0);
}

void Mesh::Draw(gl::Shader &shader) const {
    material_.bindMaps(shader);
    material_.applyToShader(shader);
    glBindVertexArray(VAO_);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices_.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
