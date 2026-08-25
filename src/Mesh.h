#pragma once
#include "GeometryData.h"
#include "GlHandle.h"
#include "PbrMaterial.h"
#include <vector>

/**
 * @brief 1つの VAO/VBO/EBO と PbrMaterial を持つ、描画可能な最小単位。
 */
class Mesh {
  private:
    gl::VertexArrayHandle VAO_;
    gl::BufferHandle VBO_, EBO_;
    std::vector<gl::Vertex> vertices_;
    std::vector<unsigned int> indices_;
    gl::PbrMaterial material_;
    bool isSkinned_ = false; // そのメッシュがボーンを持つか（スキンメッシュなら true）
    glm::vec3 boundsMin_{0.0f};
    glm::vec3 boundsMax_{0.0f};

    void setupMesh();
    /// 頂点からバインドポーズの AABB を求める。
    void computeBounds();

  public:
    /**
     * @brief 頂点・インデックス・マテリアルから Mesh を構築する。
     *
     * @param vertices 頂点データ。
     * @param indices 描画順のインデックス。
     * @param material 適用する PBR マテリアル。
     * @param isSkinned ボーンを持つメッシュかどうか。
     */
    Mesh(std::vector<gl::Vertex> vertices, std::vector<unsigned int> indices, gl::PbrMaterial material, bool isSkinned);

    bool IsSkinned() const { return isSkinned_; }
    const glm::vec3 &BoundsMin() const { return boundsMin_; }
    const glm::vec3 &BoundsMax() const { return boundsMax_; }

    // GlHandle がコピー禁止・ムーブ可なので、Mesh もそれに従う（std::vector<Mesh> で必要）
    Mesh(Mesh &&) noexcept = default;
    Mesh &operator=(Mesh &&) noexcept = default;

    /**
     * @brief マテリアルを適用して描画する。
     *
     * @param shader 描画に使うシェーダープログラム。
     */
    void Draw(gl::Shader &shader) const;
};
