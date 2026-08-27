#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "asset/Mesh.h"
#include "gl/TextureCache.h"

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

/// キーフレーム1つ。時刻は現実の秒ではなくアニメーションの tick 単位
template <typename T>
struct AnimationKey {
    float time = 0.0f;
    T value{};
};

/// 1ノードのキーフレーム列（aiNodeAnim に対応）
struct NodeAnimation {
    std::vector<AnimationKey<glm::vec3>> positions;
    std::vector<AnimationKey<glm::quat>> rotations;
    std::vector<AnimationKey<glm::vec3>> scales;
};

/// アニメーション1本
struct Animation {
    std::string name;
    float duration = 0.0f; // tick 単位の長さ
    float ticksPerSecond = 25.0f; // 1秒 = 25 ticks -> 1tics = 40ms
    std::unordered_map<std::string, NodeAnimation> channels; // ノード名 -> キー列
};

// UBO のサイズを決め打ちするための上限。gbuffer_model.vert / point_shadow_depth.vert の MAX_BONES と一致させる
inline constexpr int kMaxBones = 128;
// ボーンパレット用の UBO のバインディング。0 は Scene の Matrices が使っている
inline constexpr unsigned int kBoneUBOBinding = 1;

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

    /**
     * @brief 再生位置を進め、ボーン行列を作り直す
     *
     * @param deltaTime 前フレームからの経過秒
     */
    void UpdateAnimation(float deltaTime);

    /// バインドポーズでの高さを返す。
    float Height() const {
        return boundsMax_.y - boundsMin_.y;
    }

    bool HasAnimation() const { return activeAnimation_ >= 0; }

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
    glm::vec3 boundsMin_{0.0f};
    glm::vec3 boundsMax_{0.0f};
    // デフォルトブロックの uniform 上限（GL の保証は 1024 component）を避けるため UBO で送る
    gl::BufferHandle boneUBO_;

    std::vector<Animation> animations_;
    int activeAnimation_ = -1; // 再生中のアニメーション -1 でなし
    float animationTime_ = 0.0f; // アニメーションの再生時間

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
    /// boneMatrices_ を UBO へ書き込む。
    void uploadBoneMatrices();
    /// バインドポーズの AABB をノード階層をたどって求める。
    void accumulateBounds(const ModelNode &node, const glm::mat4 &parentTransform);
    /// aiAnimation をすべて読み込む
    void loadAnimations(const aiScene *scene);
    /// チャンネルがあれば時刻 time のローカル変換を作り、なければバインドポーズを返す
    glm::mat4 nodeTransform(const ModelNode &node, float time) const;
    /// ノード階層をたどり、各ボーンの最終変換行列を計算する
    void updateBoneMatrices(const ModelNode &node, const glm::mat4 &parentTransform, float time);
};
