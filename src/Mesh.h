#pragma once
#include "GeometryData.h"
#include "GlHandle.h"
#include "PbrMaterial.h"
#include <vector>

class Mesh {
  private:
    gl::VertexArrayHandle VAO_;
    gl::BufferHandle VBO_, EBO_;
    std::vector<gl::Vertex> vertices_;
    std::vector<unsigned int> indices_;
    gl::PbrMaterial material_;
    bool isSkinned_ = false; // そのメッシュがボーンを持つか（スキンメッシュなら true）

    void setupMesh();

  public:
    Mesh(std::vector<gl::Vertex> vertices, std::vector<unsigned int> indices, gl::PbrMaterial material, bool isSkinned);

    bool IsSkinned() const { return isSkinned_; }

    // GlHandle がコピー禁止・ムーブ可なので、Mesh もそれに従う（std::vector<Mesh> で必要）
    Mesh(Mesh &&) noexcept = default;
    Mesh &operator=(Mesh &&) noexcept = default;

    void Draw(gl::Shader &shader) const;
};
