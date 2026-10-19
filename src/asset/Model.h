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
#include "asset/MeshDistanceField.h"
#include "asset/MeshDistanceFieldCache.h"
#include "gl/TextureCache.h"

// スキニングはノード階層を辿って行列を合成するため 読み込み時に平坦化せず木のまま保持する
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

/// キーフレーム1つ 時刻は現実の秒ではなくアニメーションの tick 単位
template <typename T> struct AnimationKey {
    float time = 0.0f;
    T value{};
};

/// 静的メッシュの距離場で、SDF 遮蔽物として使う
struct StaticMeshDistanceField {
    std::shared_ptr<const gl::MeshDistanceField> field;
    glm::mat4 nodeToModelRoot;
};

/// 特定の時刻における 1 ノードの姿勢 NodeAnimation のキー列を補間して得る
struct NodePose {
    glm::vec3 translation{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};
};

/// ボーンの姿勢を表す 1フレーム分の情報（クリップ間のクロスフェードに使う）
using Pose = std::unordered_map<std::string, NodePose>;

/// 1ノードのキーフレーム列（aiNodeAnim に対応）
struct NodeAnimation {
    std::vector<AnimationKey<glm::vec3>> positions;
    std::vector<AnimationKey<glm::quat>> rotations;
    std::vector<AnimationKey<glm::vec3>> scales;
};

/// アニメーション1本
struct Animation {
    std::string name;
    float duration = 0.0f;                                   // tick 単位の長さ
    float ticksPerSecond = 25.0f;                            // 1秒 = 25 ticks -> 1tics = 40ms
    std::unordered_map<std::string, NodeAnimation> channels; // ノード名 -> キー列
};

// UBO のサイズを決め打ちするための上限 gbuffer_model.vert / point_shadow_depth.vert の MAX_BONES と一致させる
inline constexpr int kMaxBones = 128;
// ボーンパレット用の UBO のバインディング 0 は Scene の Matrices が使っている
inline constexpr unsigned int kBoneUBOBinding = 1;

inline constexpr float kBlendDuration = 0.1f; // クロスフェードの時間（秒）

/**
 * @brief Assimp で glTF/glb モデルを読み込み Mesh の集合とスキニング用のボーン情報を保持する
 */
class Model {
  public:
    /**
     * @brief モデルを読み込む
     *
     * @param path モデルファイルのパス
     * @param[in,out] cache テクスチャの共有キャッシュ 読み込んだテクスチャが登録される
     * @param[in,out] sdfCache 距離場の共有キャッシュ 焼いた距離場が登録される
     */
    Model(const std::string &path, TextureCache &cache, gl::MeshDistanceFieldCache &sdfCache);

    /**
     * @brief ノード階層を辿って全メッシュを描画する
     *
     * @param shader 描画に使うシェーダープログラム
     * @param modelMatrix Local → World への変換行列
     */
    void Draw(gl::Shader &shader, const glm::mat4 &modelMatrix) const;

    /**
     * @brief 再生位置を進め ボーン行列を作り直す
     *
     * @param deltaTime 前フレームからの経過秒
     */
    void UpdateAnimation(float deltaTime);

    /// バインドポーズでの高さを返す
    float Height() const { return boundsMax_.y - boundsMin_.y; }

    bool HasAnimation() const { return activeAnimation_ >= 0; }

    /* ---- ここから下はスキニングのための情報 ---- */
    const ModelNode &RootNode() const { return root_; }
    // ルートノードの変換の逆行列 スキニング行列の計算に掛ける
    const glm::mat4 &GlobalInverseTransform() const { return globalInverseTransform_; }
    const std::unordered_map<std::string, BoneInfo> &Bones() const { return bones_; }

    // SDF 遮蔽物として使う 静的メッシュ（ボーン無し）ぶんの距離場
    const std::vector<StaticMeshDistanceField> &StaticDistanceFields() const { return staticDistanceFields_; }

    /**
     * @brief 名前でアニメーションを切り替え 現在のポーズからクロスフェードする
     *
     * 再生中と同じ名前なら何もしない 見つからなければ標準エラーへ報告して続行する
     * @param name 再生するアニメーションの名前
     * @param isLoopAnimation false なら1回だけ再生する
     * @param playbackSpeed 再生速度の倍率
     */
    void PlayAnimation(const std::string &name, bool isLoopAnimation = true, float playbackSpeed = 1.0f);

    /// 再生中のアニメーションが終了したかどうかを返す ループ再生中は常に false
    bool isAnimationFinished() const;  

  private:
    std::vector<Mesh> meshes_;
    ModelNode root_;
    glm::mat4 globalInverseTransform_{1.0f};
    std::unordered_map<std::string, BoneInfo> bones_;
    // 同じ aiMesh を複数のノードが参照しても二重に送らないための Assimp 番号 → meshes_ 添字の対応表
    std::unordered_map<unsigned int, unsigned int> meshIndexByAiIndex_;
    std::string path_;
    std::string directory_;
    TextureCache &cache_;
    gl::MeshDistanceFieldCache &sdfCache_;
    std::vector<glm::mat4> boneMatrices_;
    glm::vec3 boundsMin_{0.0f};
    glm::vec3 boundsMax_{0.0f};
    // デフォルトブロックの uniform 上限（GL の保証は 1024 component）を避けるため UBO で送る
    gl::BufferHandle boneUBO_;
    std::vector<StaticMeshDistanceField> staticDistanceFields_;

    std::vector<Animation> animations_;
    int activeAnimation_ = -1;   // 再生中のアニメーション -1 でなし
    float animationTime_ = 0.0f; // アニメーションの再生時間
    bool isLoopAnimation_ = true; // ループ再生するかどうか
    float playbackSpeed_ = 1.0f; // 再生速度の倍率

    // アニメーションのクロスフェード用
    Pose currentPose_;
    Pose blendSourcePose_;
    float blendElapsedTime_ = 0.0f; // ticksではなく秒
    float blendWeight_ = 0.0f;      // 0.0～1.0 クロスフェードの重み

    /// Assimp でシーンを読み込む
    void loadModel(const std::string &path);
    /// aiNode を ModelNode へ変換する
    ModelNode processNode(const aiNode *node, const aiScene *scene);
    /// aiMesh から Mesh を組み立てる
    Mesh processMesh(const aiMesh *mesh, const aiScene *scene);
    /**
     * @brief aiMesh のボーン情報を頂点へ書き込む
     *
     * @param[in,out] vertices ボーン ID と重みを書き足す頂点列
     */
    void loadBones(const aiMesh *mesh, std::vector<gl::Vertex> &vertices);
    /// aiMaterial から PbrMaterial を組み立てる
    gl::PbrMaterial loadMaterial(const aiMaterial *mat, const aiScene *scene);
    std::shared_ptr<Texture> loadTexture(const aiMaterial *mat, aiTextureType type, ColorSpace colorSpace,
                                         const aiScene *scene);
    /**
     * @brief ノードを描画する
     * @param node 描画するノード
     * @param parentTransform このノードの親までの累積変換
     * @param skinnedWorldTransform スキンメッシュに使うワールド変換
     * （modelMatrix * root_.localTransform 全ノード共通）
     * @param shader 描画に使うシェーダ
     */
    void drawNode(const ModelNode &node, const glm::mat4 &parentTransform, const glm::mat4 &skinnedWorldTransform,
                  gl::Shader &shader) const;
    /// boneMatrices_ を UBO へ書き込む
    void uploadBoneMatrices();
    /// バインドポーズの AABB をノード階層をたどって求める
    void accumulateBounds(const ModelNode &node, const glm::mat4 &parentTransform);
    /// ノード階層をたどり 静的メッシュ（ボーン無し）ごとに距離場を焼く 起動時に一度だけ呼ぶ想定
    void buildStaticDistanceFields(const ModelNode &node, const glm::mat4 &parentTransform);
    /// aiAnimation をすべて読み込む
    void loadAnimations(const aiScene *scene);
    /**
     * @brief ノードの姿勢を取得する
     *
     * @param time アニメーションの tick 単位の時刻（秒ではない）
     */
    NodePose nodePose(const ModelNode &node, float time) const;
    /// NodePose を行列に変換する
    glm::mat4 nodePoseToMatrix(const NodePose &pose) const;

    /**
     * @brief ノード階層をたどり 各ボーンの最終変換行列を計算する
     *
     * @param time アニメーションの tick 単位の時刻（秒ではない）
     */
    void updateBoneMatrices(const ModelNode &node, const glm::mat4 &parentTransform, float time);
};
