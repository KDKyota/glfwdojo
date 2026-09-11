#pragma once
#include "core/FrameArena.h"
#include "debug/CollisionDebugDraw.h"
#include "debug/GpuProfiler.h"
#include "gl/GlHandle.h"
#include "gl/NoiseTexture.h"
#include "gl/TextureCache.h"
#include "render/GeometryData.h"
#include "render/RenderSettings.h"
#include "render/SceneGeometry.h"
#include "render/ibl/IblMaps.h"
#include "render/pass/BloomPass.h"
#include "render/pass/DeferredLightingPass.h"
#include "render/pass/ForwardPass.h"
#include "render/pass/GeometryPass.h"
#include "render/pass/SdfOcclusionPass.h"
#include "render/pass/ShadowPass.h"
#include "render/pass/SsaoPass.h"
#include "render/pass/TonemapPass.h"
#include "render/targets/GBuffer.h"
#include "render/targets/HdrTarget.h"
#include "render/targets/OcclusionTarget.h"
#include "render/targets/ShadowCubeTargets.h"
#include "scene/Camera.h"
#include "scene/Collision.h"
#include "scene/SceneModels.h"
#include <glm/glm.hpp>
#include <memory>
#include <vector>

/**
 * @brief シーンのジオメトリ・ライト・レンダーパイプライン全体を所有し 1フレームの描画を統括する
 */
class Scene {
  public:
    /**
     * @brief シーンを初期化する
     *
     * @param camera 描画に使うカメラ
     * @param scrWidth,scrHeight 画面解像度
     */
    Scene(std::shared_ptr<Camera> camera, int scrWidth, int scrHeight);

    /**
     * @brief 1フレーム分の描画を行う
     *
     * @param deltaTime 前フレームからの経過時間
     * @param heightScale Parallax Mapping の強さ
     */
    void Render(float deltaTime, float heightScale);

    // ImGui のパネルから直接編集する
    gl::RenderSettings &Settings() {
        return settings_;
    }

    // 三人称カメラの追従先 対象のモデルが読み込めていなければ nullptr
    const glm::vec3 *FollowTargetPosition() const {
        return models_.FollowTargetPosition();
    }

    // 操作対象 読み込めていなければ nullptr
    Character *PlayerCharacter() {
        return models_.PlayerCharacter();
    }

    const gl::CollisionWorld &Colliders() const {
        return colliders_;
    }

    const gl::GpuProfiler &Profiler() const {
        return profiler_;
    }

  private:
    /// 壁と立方体から衝突判定用の直方体を作る
    void initColliders();
    /// 全パスが参照する View/Projection の UBO を確保する
    void initMatricesUBO();
    /// 透明オブジェクトをカメラ距離でソートしてインスタンス VBO へ送る
    void updateTransparentInstances();
    /// View/Projection 行列を UBO へ書き込む
    void updateMatricesUBO();

    int scrWidth_, scrHeight_;
    std::shared_ptr<Camera> camera_;
    TextureCache cache_;
    gl::RenderSettings settings_;

    gl::CollisionWorld colliders_; // 壁と立方体を直方体として持つ
    gl::SceneGeometry geometry_;
    gl::SceneModels models_;
    gl::NoiseTexture noise_; // SSAO と SDF 遮蔽が共有する

    /* レンダーターゲット パス同士はこれを介して繋がる */
    gl::GBuffer gBuffer_;
    gl::HdrTarget hdrTarget_;
    gl::ShadowCubeTargets shadowTargets_;
    gl::OcclusionTarget ssaoTarget_;
    gl::OcclusionTarget sdfOcclusionTarget_;
    gl::IblMaps iblMaps_;

    /* Render() から順に呼ばれるパス テクスチャで繋がっているので順序に意味がある */
    gl::ShadowPass shadowPass_;
    gl::GeometryPass geometryPass_;
    gl::SsaoPass ssaoPass_;
    gl::SdfOcclusionPass sdfOcclusionPass_;
    gl::DeferredLightingPass deferredLightingPass_;
    gl::ForwardPass forwardPass_;
    gl::BloomPass bloomPass_;
    gl::TonemapPass tonemapPass_;

    gl::CollisionDebugDraw collisionDebugDraw_;
    gl::BufferHandle matricesUBO_;
    gl::GpuProfiler profiler_;

    // frameArena_ の見積もり 用途を増やしたら内訳を1行足すこと
    //  updateTransparentInstances(): TransparentDraw × 窓の上限数
    static constexpr std::size_t kMaxTransparentWindows = 8; // 現在6枚 増減の余地を見て余裕を持たせる
    static constexpr std::size_t kFrameArenaBytes = sizeof(gl::TransparentDraw) * kMaxTransparentWindows;

    // 寿命が1フレームのデータ用 汎用アロケータの毎フレーム確保/解放を避ける
    gl::FrameArena frameArena_;

    std::vector<glm::vec3> transparentPositions_; // 奥から手前へ並べ替えた窓の位置
};
