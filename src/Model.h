#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/glm.hpp>

#include "Mesh.h"
#include "TextureCache.h"

// スキニングはノード階層を辿って行列を合成するため、読み込み時に平坦化せず木のまま保持する
struct ModelNode {
    std::string name;
    glm::mat4 localTransform{1.0f}; // 親ノードからの相対変換
    std::vector<unsigned int> meshIndices;
    std::vector<ModelNode> children;
};

struct BoneInfo {
    int index = 0;
    glm::mat4 offset{1.0f}; // メッシュ空間からボーン空間への変換（aiBone::mOffsetMatrix）
};

class Model {
  public:
    Model(const std::string &path, TextureCache &cache);

    void Draw(gl::Shader &shader, const glm::mat4 &modelMatrix) const;

    /* ---- ここから下はスキニングのための情報 ---- */
    const ModelNode &RootNode() const { return root_; }
    // ルートノードの変換の逆行列。掛け忘れるとモデル全体が変な位置とスケールで出る
    const glm::mat4 &GlobalInverseTransform() const { return globalInverseTransform_; }
    const std::unordered_map<std::string, BoneInfo> &Bones() const { return bones_; }

  private:
    std::vector<Mesh> meshes_;
    ModelNode root_;
    glm::mat4 globalInverseTransform_{1.0f};
    std::unordered_map<std::string, BoneInfo> bones_;
    // 同じ aiMesh を複数のノードが参照していても二重に GPU へ送らないための対応表
    std::unordered_map<unsigned int, unsigned int> meshIndexByAiIndex_;
    std::string path_;
    std::string directory_;
    TextureCache &cache_;

    void loadModel(const std::string &path);
    ModelNode processNode(const aiNode *node, const aiScene *scene);
    Mesh processMesh(const aiMesh *mesh, const aiScene *scene);
    void loadBones(const aiMesh *mesh, std::vector<gl::Vertex> &vertices);
    gl::PbrMaterial loadMaterial(const aiMaterial *mat, const aiScene *scene);
    std::shared_ptr<Texture> loadTexture(const aiMaterial *mat, aiTextureType type, ColorSpace colorSpace,
                                         const aiScene *scene);
    void drawNode(const ModelNode &node, const glm::mat4 &parentTransform, gl::Shader &shader) const;
};
