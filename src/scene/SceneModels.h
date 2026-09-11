#pragma once
#include "asset/Model.h"
#include "gl/Shader.h"
#include "gl/TextureCache.h"
#include "scene/Character.h"
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace gl {

/**
 * @brief 読み込んだモデルとその配置行列 および操作対象のキャラクターを持つ
 *
 * Shadow パスと Geometry パスの両方が同じモデル列を別のシェーダーで描く
 */
class SceneModels {
  public:
    /// SceneLayout の modelSpawns に従って各モデルを読み込む
    explicit SceneModels(TextureCache &cache);

    /// アニメーションを進める 操作対象は待機モーションが無いので停止中は進めない
    void UpdateAnimation(float deltaTime);

    /// 操作対象のモデル行列を現在の位置と向きから作り直す
    void SyncPlayerMatrix();

    void Draw(Shader &shader) const;

    // 操作対象 読み込めていなければ nullptr
    Character *PlayerCharacter() {
        return character_.get();
    }

    // 三人称カメラの追従先 対象のモデルが読み込めていなければ nullptr
    const glm::vec3 *FollowTargetPosition() const {
        return character_ ? &character_->Position() : nullptr;
    }

  private:
    std::vector<std::unique_ptr<Model>> models_; // テクスチャやボーン・アニメーションなどの描画情報を持つ
    // models_ と添字が一対一で対応する 読み込みに失敗したモデルは両方に積まれない
    std::vector<glm::mat4> modelMatrices_; // 描画用のモデルのデータ（どこにどの向きで描くか）
    std::unique_ptr<Character> character_; // キャラクターのゲーム上の状態
    // 操作対象のモデル 読み込めていなければ -1
    int playerModelIndex_ = -1;
    // 操作対象の正面軸の補正とスケール 毎フレーム yaw を左から掛けて使う
    glm::mat4 playerBaseTransform_{1.0f}; // yaw 以外の回転とスケールを畳んだ行列
};

} // namespace gl
