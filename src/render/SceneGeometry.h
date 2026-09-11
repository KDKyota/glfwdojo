#pragma once
#include "gl/GlHandle.h"
#include "gl/Shader.h"
#include "gl/Texture.h"
#include "gl/TextureCache.h"
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace gl {

/**
 * @brief シーンの静的メッシュとそのテクスチャを持ち 与えられたシェーダーで描画する
 *
 * 同じメッシュを Shadow パスと Geometry パスが別のシェーダーで描くのでシェーダーは持たない
 */
class SceneGeometry {
  public:
    explicit SceneGeometry(TextureCache &cache);

    void DrawCubes(Shader &shader) const;
    void DrawFloor(Shader &shader) const;
    void DrawWalls(Shader &shader) const;
    /// instanceCount には並べ替え済みの窓の枚数を渡す
    void DrawWindows(Shader &shader, std::size_t instanceCount) const;
    /// テクスチャを使わない立方体 ライトキューブと IBL のキャプチャで使う
    void DrawCubeMesh() const;
    void DrawSkyboxMesh() const;
    void DrawScreenQuad() const;

    /// 並べ替えた窓の位置をインスタンス VBO へ書き込む
    void UploadWindowInstances(const std::vector<glm::vec3> &positions) const;

  private:
    /// VAO を組み立て 頂点属性 location を設定する
    void initMesh();
    void initTextures(TextureCache &cache);

    /* メッシュのVAO / VBO / EBO */
    VertexArrayHandle cubeVAO_, planeVAO_, transparentVAO_, quadVAO_, skyboxVAO_, wallVAO_;
    BufferHandle cubeVBO_, planeVBO_, transparentVBO_, quadVBO_, skyboxVBO_, wallVBO_;
    BufferHandle cubeInstanceVBO_;
    BufferHandle transparentInstanceVBO_;
    BufferHandle cubeEBO_, planeEBO_, transparentEBO_, wallEBO_;

    /* Textures */
    std::shared_ptr<Texture> cubeTexture_;
    std::shared_ptr<Texture> cubeNormalMap_;
    std::shared_ptr<Texture> cubeHeightMap_;
    std::shared_ptr<Texture> floorTexture_;
    std::shared_ptr<Texture> transparentTexture_;
    std::shared_ptr<Texture> brickwallTexture_;
    std::shared_ptr<Texture> brickwallNormalTexture_;
};

} // namespace gl
