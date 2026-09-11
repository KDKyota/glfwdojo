#include "render/Scene.h"

#include "core/SceneUnits.h"
#include "render/GeometryData.h"
#include "render/ibl/IblBaker.h"
#include "scene/SceneLayout.h"

#include <algorithm>
#include <cstddef>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {
// 全パスが view / projection を読む binding
constexpr GLuint kMatricesUboBinding = 0;
} // namespace

Scene::Scene(std::shared_ptr<Camera> camera, int scrWidth, int scrHeight)
    : scrWidth_(scrWidth), scrHeight_(scrHeight), camera_(camera), geometry_(cache_), models_(cache_),
      gBuffer_(scrWidth, scrHeight), hdrTarget_(scrWidth, scrHeight),
      ssaoTarget_(scrWidth, scrHeight, "SSAO"),
      // SDF は数m規模の低周波な遮蔽しか拾わないので半解像度で足りる
      sdfOcclusionTarget_(scrWidth / 2, scrHeight / 2, "SDF_OCCLUSION"),
      // skyboxVAO と screen quad が必要なので geometry_ より後に置くこと
      iblMaps_(gl::BakeIblMaps(geometry_, scrWidth, scrHeight)), bloomPass_(scrWidth, scrHeight) {
    frameArena_.Init(kFrameArenaBytes);
    // 既定の 0.5 では環境が高いミップまでぼけ 浅い角度で白い靄になる
    settings_.glassMaterial.roughness = 0.08f;

    initColliders();
    initMatricesUBO();
    profiler_.Init();
}

void Scene::initColliders() {
    constexpr float half = gl::units::floorHalfExtent;
    // 平面のままだと厚みが 0 で押し出す向きが決まらないので外側へ伸ばす
    constexpr float wallThickness = 5.0f;

    colliders_.Add({{-half, gl::units::floorY, -half - wallThickness}, {half, gl::units::wallTopY, -half}});
    colliders_.Add({{-half, gl::units::floorY, half}, {half, gl::units::wallTopY, half + wallThickness}});

    // cubeVertices は ±0.5 なので中心から半分ずつ広げる
    constexpr float cubeHalf = 0.5f;
    for (const glm::vec3 &center : gl::layout::cubePositions)
        colliders_.Add({center - glm::vec3(cubeHalf), center + glm::vec3(cubeHalf)});
}

void Scene::initMatricesUBO() {
    matricesUBO_.create();
    glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO_);
    glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), nullptr, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, kMatricesUboBinding, matricesUBO_);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void Scene::Render(float deltaTime, float heightScale) {
    models_.SyncPlayerMatrix();
    models_.UpdateAnimation(deltaTime);

    // 前フレームの frameArena_ 確保をここで無効化する（末尾ではなく先頭でリセット）
    frameArena_.Reset();
    updateTransparentInstances();

    profiler_.BeginFrame();

    const std::size_t windowCount = transparentPositions_.size();

    // [1] 光源視点の深度とガラスの透過色（4灯ぶん）
    profiler_.Measure(gl::GpuPass::Shadow, [&] {
        shadowPass_.Execute(shadowTargets_, geometry_, models_, windowCount, settings_.shadowMapStaticCasters);
    });
    updateMatricesUBO(); // [2] view / projection を UBO へ 以降の全パスが参照する SDFの UBO は定数の集まりなのでupdateしない
    // [3] 不透明物の幾何情報を G-Buffer へ
    profiler_.Measure(gl::GpuPass::Geometry, [&] {
        geometryPass_.Execute(gBuffer_, geometry_, models_, *camera_, settings_, heightScale, windowCount);
    });
    // [4] G-Buffer から遮蔽率を求めてブラーまで
    profiler_.Measure(gl::GpuPass::Ssao, [&] { ssaoPass_.Execute(ssaoTarget_, gBuffer_, noise_, geometry_); });
    // [4.5] SSAO が届かない数m規模の遮蔽を SDF から求める
    profiler_.Measure(gl::GpuPass::SdfOcclusion,
                      [&] { sdfOcclusionPass_.Execute(sdfOcclusionTarget_, gBuffer_, noise_, geometry_); });
    // [5] G-Buffer の深度を hdrTarget_ へ複製（前方描画の深度テスト用）
    profiler_.Measure(gl::GpuPass::BlitDepth, [&] { gBuffer_.BlitDepthTo(hdrTarget_.Fbo()); });
    // [6] G-Buffer + 影 + AO を合成
    profiler_.Measure(gl::GpuPass::Lighting, [&] {
        deferredLightingPass_.Execute(hdrTarget_, gBuffer_, shadowTargets_, ssaoTarget_, sdfOcclusionTarget_,
                                      iblMaps_, geometry_, *camera_, settings_);
    });
    // [7] G-Buffer に入れられないもの（ライトキューブ・空・ガラス）
    profiler_.Measure(gl::GpuPass::Forward, [&] {
        forwardPass_.Execute(hdrTarget_, geometry_, iblMaps_, shadowTargets_, *camera_, settings_, windowCount,
                             collisionDebugDraw_, models_.PlayerCharacter());
    });
    // [8] 明るい部分をぼかして Bloom の素材を作る
    profiler_.Measure(gl::GpuPass::Bloom, [&] { bloomPass_.Execute(hdrTarget_); });
    // [9] トーンマッピングとガンマ補正をしてデフォルトFBOへ
    profiler_.Measure(gl::GpuPass::ToScreen,
                      [&] { tonemapPass_.Execute(hdrTarget_, bloomPass_.Result(), geometry_, settings_); });

    profiler_.EndFrame();
}

// 現在のガラスは乗算／加算ブレンドなので この並べ替えは正しさには影響しない
void Scene::updateTransparentInstances() {
    const auto &windowPositions = gl::layout::windowPositions;
    auto sorted = frameArena_.Allocate<gl::TransparentDraw>(windowPositions.size());
    for (unsigned int i = 0; i < windowPositions.size(); ++i)
        sorted.data[i] = {glm::length(camera_->GetViewPosition() - windowPositions[i]), i};
    std::sort(sorted.begin(), sorted.end(),
              [](const gl::TransparentDraw &a, const gl::TransparentDraw &b) { return a.distance > b.distance; });
    transparentPositions_.clear();
    for (const auto &draw : sorted)
        transparentPositions_.push_back(windowPositions[draw.index]);

    geometry_.UploadWindowInstances(transparentPositions_);
}

void Scene::updateMatricesUBO() {
    glm::mat4 view = camera_->GetViewMatrix();
    glm::mat4 projection =
        glm::perspective(glm::radians(camera_->GetZoomValue()), (float)scrWidth_ / (float)scrHeight_, 0.1f, 100.0f);
    glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO_);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(view));
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(projection));
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}
