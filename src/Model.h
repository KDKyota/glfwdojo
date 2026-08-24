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

inline constexpr int kMaxBones = 128; // uniform 配列で送るので上限を決めておく

/**
 * @brief Assimp で glTF/glb モデルを読み込み、Mesh の集合とスキニング用のボーン情報を保持する。
 */
class Model {
  public:
    /**
     * @brief モデルを読み込む。
     *
     * @param path モデルファイルのパス。
     * @param cache テクスチャの多重ロードを避けるための共有キャッシュ。
     */
    Model(const std::string &path, TextureCache &cache);

    /**
     * @brief ノード階層を辿って全メッシュを描画する。
     *
     * @param shader 描画に使うシェーダープログラム。
     * @param modelMatrix Local → World への変換行列。
     */
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
    std::unordered_map<unsigned int, unsigned int> meshIndexByAiIndex_; // map の一つ目のint が　Assimp の番号 二つ目が meshes_ の添え字
    std::string path_;
    std::string directory_;
    TextureCache &cache_;
    std::vector<glm::mat4> boneMatrices_;

    /// Assimp でシーンを読み込む。
    void loadModel(const std::string &path);
    /// aiNode を ModelNode へ変換する。
    ModelNode processNode(const aiNode *node, const aiScene *scene);
    /// aiMesh から Mesh を組み立てる。
    Mesh processMesh(const aiMesh *mesh, const aiScene *scene);
    /// aiMesh のボーン情報を頂点へ書き込む。
    void loadBones(const aiMesh *mesh, std::vector<gl::Vertex> &vertices);
    /// aiMaterial から PbrMaterial を組み立てる。
    gl::PbrMaterial loadMaterial(const aiMaterial *mat, const aiScene *scene);
    std::shared_ptr<Texture> loadTexture(const aiMaterial *mat, aiTextureType type, ColorSpace colorSpace,
                                         const aiScene *scene);
    /**
     * @brief ノードを描画する
     * @param [in] node 今書こうとしているノード
     * @param [in] parentTransform このノードの親までの累積変換
     * @param [in] skinnedWorldTransform スキンメッシュに使うワールド変換（modelMatrix * root_.localTransform、全ノード共通）
     * @param [in] shader 描画に使うシェーダ
     */
    void drawNode(const ModelNode &node, const glm::mat4 &parentTransform, const glm::mat4 &skinnedWorldTransform, gl::Shader &shader) const;
    /// ノード階層をたどり、各ボーンの最終変換行列を計算する
    void updateBoneMatrices(const ModelNode &node, const glm::mat4 &parentTransform);
};
