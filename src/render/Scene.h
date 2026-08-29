#pragma once
#include "scene/Camera.h"
#include "scene/Character.h"
#include "scene/Collision.h"
#include "core/FrameArena.h"
#include "render/GeometryData.h"
#include "gl/GlHandle.h"
#include "debug/GpuProfiler.h"
#include "render/Lighting.h"
#include "render/Material.h"
#include "asset/Model.h"
#include "render/PbrMaterial.h"
#include "gl/Shader.h"
#include "gl/TextureCache.h"
#include <array>
#include <memory>
#include <vector>

/**
 * @brief シーンのジオメトリ・ライト・レンダーパイプライン全体を所有し、1フレームの描画を統括する。
 */
class Scene {
  public:
    /**
     * @brief シーンを初期化する。
     *
     * @param camera 描画に使うカメラ。
     * @param srcWindow,scrHeight 画面解像度。
     */
    Scene(std::shared_ptr<Camera> camera, int srcWindow, int scrHeight);

    /**
     * @brief 1フレーム分の描画を行う。
     *
     * @param deltaTime 前フレームからの経過時間。
     * @param heightScale Parallax Mapping の強さ。
     */
    void Render(float deltaTime, float heightScale);

    // ImGui のパネルから直接編集するためのアクセサ
    int &DebugMode() {
        return debugMode_;
    }
    bool &DebugRawOutput() {
        return debugRawOutput_;
    }
    bool &DebugCheckerFloor() {
        return debugCheckerFloor_;
    }
    bool &DebugCheckerInvert() {
        return debugCheckerInvert_;
    }
    float &SsaoStrength() {
        return ssaoStrength_;
    }
    float &Exposure() {
        return exposure_;
    }
    float &AmbientStrength() {
        return ambientStrength_;
    }
    float &BloomStrength() {
        return bloomStrength_;
    }
    gl::PbrMaterial &CubeMaterial() {
        return cubeMaterial_;
    }
    gl::PbrMaterial &FloorMaterial() {
        return floorMaterial_;
    }
    gl::PbrMaterial &WallMaterial() {
        return wallMaterial_;
    }
    gl::PbrMaterial &WindowMaterial() {
        return windowMaterial_;
    }
    gl::PbrMaterial &GlassMaterial() {
        return glassMaterial_;
    }
    // 三人称カメラの追従先。対象のモデルが読み込めていなければ nullptr
    const glm::vec3 *FollowTargetPosition() const {
        return character_ ? &character_->Position() : nullptr;
    }

    // 操作対象 読み込めていなければ nullptr
    Character *PlayerCharacter() {
        return character_.get();
    }

    const gl::CollisionWorld &Colliders() const {
        return colliders_;
    }

    bool &DebugCollision() {
        return debugCollision_;
    }

    const gl::GpuProfiler &Profiler() const {
        return profiler_;
    }

  private:
    void initMesh();
    void initTextures();
    void initModels();
    /// 壁と立方体から衝突判定用の直方体を作る。
    void initColliders();
    /// デバッグ表示用の円柱ワイヤーフレームを作る。
    void initDebugShapes();
    /// 衝突判定の円柱を描く。
    void renderDebugCollision();
    /// 操作対象のモデル行列を現在の位置と向きから作り直す。
    void updatePlayerModelMatrix();
    void initFramebuffer();
    void initGBuffer();
    void initSSAO();
    void initIBL();
    unsigned int loadTexture(const char *path, bool hasAlpha);
    void initUBO();

    // Render() から順に呼ばれるパス。FBO とテクスチャで繋がっているので順序に意味がある
    /// 透明オブジェクトをカメラ距離でソートする（結果は frameArena_ 上に確保する）。
    gl::ArraySpan<gl::TransparentDraw> updateTransparentInstances();
    /// View/Projection 行列を UBO へ書き込む。
    void updateMatricesUBO();
    /// Point Light のシャドウマップを描く。
    void renderShadowPasses();
    /// 不透明オブジェクトを G-Buffer へ描く。
    void renderGeometryPass();
    /// SSAO を計算する。
    void renderSSAOPass();
    /// G-Buffer の深度をデフォルト FBO へコピーする。
    void blitGeometryDepth();
    /// Deferred Shading のライティングを合成する。
    void renderDeferredLightingPass();
    /// G-Buffer に書けないオブジェクトを Forward Shading で描画する。
    void renderForwardPass(gl::ArraySpan<gl::TransparentDraw> sorted);
    /// Bloom 用のぼかしを作る。
    void renderBloomBlur();
    /// トーンマッピングとガンマ補正をかけて画面へ出力する。
    void renderToScreen();

    void applyPointLights(gl::Shader &shader);
    void renderCubes(gl::Shader &shader);
    void renderFloor(gl::Shader &shader);
    void renderLightCubes();
    void renderSkybox();
    void renderTransparentWindows(gl::ArraySpan<gl::TransparentDraw> sorted);
    void renderWindow(gl::Shader &shader); // 窓枠用
    void renderWalls(gl::Shader &shader);
    void renderModels(gl::Shader &shader);

    static constexpr unsigned int SHADOW_WIDTH = 1024,
                                  SHADOW_HEIGHT = 1024; // depthCubemap_ 各面の解像度
    int scrWidth_, scrHeight_;
    // 1.0 だと光源から1m以内の物体が影を落とさなくなる。深度は実距離を farPlane で割って
    // 書くので、near を小さくしても精度は落ちない
    static constexpr float shadowNearPlane_ = 0.1f;
    static constexpr float shadowFarPlane_ = 50.0f; // 光源視点の投影のfar plane。シェーダー側の farPlane uniform

    /* メッシュのVAO / VBO / EBO */
    gl::VertexArrayHandle cubeVAO_, planeVAO_, transparentVAO_, quadVAO_, skyboxVAO_, wallVAO_;
    gl::BufferHandle cubeVBO_, planeVBO_, transparentVBO_, quadVBO_, skyboxVBO_, wallVBO_;
    gl::BufferHandle cubeInstanceVBO_;
    gl::BufferHandle transparentInstanceVBO_;
    gl::BufferHandle cubeEBO_, planeEBO_, transparentEBO_, wallEBO_;
    /* フレームバッファ */
    gl::FramebufferHandle framebuffer_;
    gl::TextureHandle textureColorbuffer_;
    gl::RenderbufferHandle rbo_;
    // point shadow のデプスパス専用（depthCubemap_ と shadowColorCubemap_ をアタッチ）
    std::array<gl::FramebufferHandle, 4> depthMapFBO_;
    std::array<gl::FramebufferHandle, 2> pingpongFBO_;
    std::array<gl::TextureHandle, 2> pingpongColorbuffers_;
    gl::TextureHandle brightColorBuffer_;

    /* UBO */
    gl::BufferHandle matricesUBO_;
    /* G-Buffer */
    gl::FramebufferHandle gBuffer_;
    gl::TextureHandle gPosition_, gNormal_,
        gAlbedoRoughness_;             // gBuffer_にアタッチする3枚のテクスチャ
    gl::RenderbufferHandle gDepthRBO_; // gBuffer_ 用の深度バッファ

    Material material_;
    TextureCache cache_;

    /* PBR マテリアル（G-Buffer を書く4種類のオブジェクトに対応） */
    gl::PbrMaterial cubeMaterial_;
    gl::PbrMaterial floorMaterial_;
    gl::PbrMaterial wallMaterial_;
    gl::PbrMaterial windowMaterial_;
    // ガラスは誘電体なので metallic は 0 のまま（UI にも出さない）
    gl::PbrMaterial glassMaterial_;

    /* Shaders */
    std::unique_ptr<gl::Shader> cubeShader_;
    std::unique_ptr<gl::Shader> transparentwindowShader_;
    std::unique_ptr<gl::Shader> lightcubeShader_;
    // std::unique_ptr<gl::Shader> shaderSingleColor_;
    std::unique_ptr<gl::Shader> screenshader_;
    // std::unique_ptr<gl::Shader> glasscubeShader_;
    std::unique_ptr<gl::Shader> skyboxShader_;
    gl::GpuProfiler profiler_;

    // frameArena_ 用の見積もり
    // 用途を増やしたら内訳をここに1行足すこと！
    //  updateTransparentInstances(): TransparentDraw × 窓の上限数
    static constexpr std::size_t kMaxTransparentWindows = 8; // 現在は6枚だが、増減の余地を見て少し余裕を持たせる
    static constexpr std::size_t kFrameArenaBytes = sizeof(gl::TransparentDraw) * kMaxTransparentWindows;

    // 寿命が1フレームのデータ用。汎用アロケータの毎フレーム確保/解放を避ける
    gl::FrameArena frameArena_;

    std::vector<std::unique_ptr<Model>> models_; // テクスチャやボーン・アニメーションなどの描画情報を持つ
    std::unique_ptr<Character> character_;       // キャラクターのゲーム上の状態
    gl::CollisionWorld colliders_;               // 壁と立方体を直方体として持つ
    // 操作対象のモデル 読み込めていなければ -1
    int playerModelIndex_ = -1;
    // 操作対象の正面軸の補正とスケール 毎フレーム yaw を左から掛けて使う
    glm::mat4 playerBaseTransform_{1.0f}; // モデル行列は通常 T・R_yaw・R・S 最後の二つは最初から変わらないので一つにする
    // models_ と添字が一対一で対応する。読み込みに失敗したモデルは両方に積まれない
    std::vector<glm::mat4> modelMatrices_; // 描画用のモデルのデータ（どこにどの向きで描くか）
    std::shared_ptr<Camera> camera_;
    std::unique_ptr<gl::Shader> pointDepthShader_;
    std::unique_ptr<gl::Shader> pointColorShader_; // カラー付き透過シャドウ（ガラスの透過色）用
    std::unique_ptr<gl::Shader> debugDepthShader_;
    std::unique_ptr<gl::Shader> wallShader_;
    std::unique_ptr<gl::Shader> blurShader_; // Bloom のぼかし（Compute）
    // blur.comp の local_size_x と一致させること
    static constexpr unsigned int BLUR_TILE = 256;
    std::unique_ptr<gl::Shader> debugLineShader_; // 衝突判定の可視化用
    gl::VertexArrayHandle debugCylinderVAO_;
    gl::BufferHandle debugCylinderVBO_;
    int debugCylinderVertexCount_ = 0;
    /* G-Bufferのシェーダ */
    std::unique_ptr<gl::Shader> gbufferFloorShader_;
    std::unique_ptr<gl::Shader> gbufferWallShader_;
    std::unique_ptr<gl::Shader> gbufferWindowShader_;
    std::unique_ptr<gl::Shader> gbufferCubeShader_;
    std::unique_ptr<gl::Shader> gbufferModelShader_;
    std::unique_ptr<gl::Shader> deferredLightingShader_;

    // gl::DirectionalLight directionalLight_;
    // gl::SpotLight spotlight_;

    /* Textures */
    std::shared_ptr<Texture> cubeTexture_;
    std::shared_ptr<Texture> cubeNormalMap_;
    std::shared_ptr<Texture> cubeHeightMap_;
    std::shared_ptr<Texture> floorTexture_;
    std::shared_ptr<Texture> transparentTexture_;
    std::shared_ptr<Texture> brickwallTexture_;
    std::shared_ptr<Texture> brickwallNormalTexture_;
    // 各テクセルは光源からの正規化距離
    std::array<gl::TextureHandle, 4> depthCubemap_;
    // 各テクセルはガラスを透過した光の色。ガラスを通らない方向は白
    std::array<gl::TextureHandle, 4> shadowColorCubemap_;

    /* IBL */
    static constexpr unsigned int ENV_CUBEMAP_SIZE = 512;
    // 畳み込み後は極めて低周波なので、解像度を上げても情報が増えない
    static constexpr unsigned int IRRADIANCE_SIZE = 32;
    gl::TextureHandle hdrTexture_; // 正距円筒図法のまま読み込んだ元画像
    gl::TextureHandle envCubemap_; // 上を6面へ焼き直したもの。背景と IBL の共通ソース
    gl::TextureHandle irradianceMap_;
    // ミップの各レベルが roughness 0.0 / 0.25 / 0.5 / 0.75 / 1.0 に対応する
    static constexpr unsigned int PREFILTER_SIZE = 128;
    static constexpr unsigned int PREFILTER_MIP_LEVELS = 5;
    static constexpr unsigned int BRDF_LUT_SIZE = 512;
    gl::TextureHandle prefilterMap_;
    gl::TextureHandle brdfLUT_;
    gl::FramebufferHandle captureFBO_;
    gl::RenderbufferHandle captureRBO_;
    std::unique_ptr<gl::Shader> equirectToCubemapShader_;
    std::unique_ptr<gl::Shader> irradianceShader_;
    std::unique_ptr<gl::Shader> prefilterShader_;
    std::unique_ptr<gl::Shader> brdfLUTShader_;

    /* ==== UI から実行時に変更する設定（毎フレーム送る） ==== */
    // 対応表は main.cpp の kDebugModes
    int debugMode_ = 0;
    // Bloom・トーンマッピング・ガンマ補正を飛ばす。デバッグ表示には必須
    bool debugRawOutput_ = false;
    bool debugCheckerFloor_ = false;
    bool debugCheckerInvert_ = false;
    // 衝突判定の円柱をワイヤーフレームで重ねて描く
    bool debugCollision_ = false;
    // SSAO の効き具合（0.0 = 無効, 1.0 = そのまま適用）
    float ssaoStrength_ = 1.0f;

    /* SSAO */
    gl::FramebufferHandle ssaoFBO_, ssaoBlurFBO_;
    gl::TextureHandle ssaoColorBuffer_, ssaoColorBufferBlur_; // GL_R8
    gl::TextureHandle noiseTexture_;                          // 4x4 GL_RGBA16F
    std::vector<glm::vec3> ssaoKernel_;                       // 接空間のサンプル点

    std::unique_ptr<gl::Shader> ssaoShader_;
    std::unique_ptr<gl::Shader> ssaoBlurShader_;

    static constexpr unsigned int SSAO_KERNEL_SIZE = 64;
    static constexpr float SSAO_RADIUS = 0.6f; // 遮蔽を探す半径。目安は物体サイズの 0.2〜1.0 倍
    static constexpr float SSAO_BIAS = 0.03f;  // 自己遮蔽によるアクネ対策。RADIUS に比例させる
    static constexpr float SSAO_POWER = 2.0f;  // AO のコントラスト。実用範囲は 1.5〜3.0
    float ambientStrength_ = 0.18f;            // SSAO が掛かるのはこの項だけ
    float bloomStrength_ = 1.0f;

    float elapsedTime_ = 0.0f;
    float heightScale_ = 0.0f;
    float exposure_ = 2.0f; // HDR 値自体は変えないので Bloom の閾値や AO のコントラストに影響しない

    int viewLoc_ = -1; // viewのローケーション番号(初期値-1)
    bool blurEnable_ = true;
    bool horizontal_ = true;
    std::vector<glm::vec3> transparent_positions_; // 透過オブジェクトのモデル行列を格納する配列

    static constexpr float animationTime_ = 5.0f;
    std::vector<glm::vec3> cubePositions_;

    /* シーン固有の配置データ */
    const std::vector<glm::vec3> cube_pos_ = {
        glm::vec3(-1.0f, 0.0f, -1.0f),
        glm::vec3(2.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 2.5f), // 近接配置（中心間 1.2 = 隙間 0.2）
        glm::vec3(1.2f, 0.0f, 2.5f),
        glm::vec3(-1.0f, 1.0f, -1.0f), // 積み重ね
        glm::vec3(0.0f, 0.0f, -20.0f), // z=-25 壁の前
        glm::vec3(0.0f, 0.0f, 20.0f),  // z=+25 壁の前
    };

    // 見つからないモデルは読み飛ばす
    struct ModelSpawn {
        std::string path;
        glm::vec3 position;
        glm::vec3 rotationDegrees{0.0f}; // 軸の向きが通常とは違うモデルを立たせるための補正
        float scale;
        bool followTarget = false; // 三人称カメラが注視するモデル
    };
    const std::vector<ModelSpawn> modelSpawns_ = {
        {"resources/publishable-objects/DamagedHelmet.glb", glm::vec3(-3.0f, gl::units::floorY + 1.0f, -3.0f), glm::vec3(0.0f), 1.0f},
        {"resources/characters/RiggedSimple.glb", glm::vec3(0.0f, gl::units::floorY, -3.0f), glm::vec3(0.0f), 1.0f},
        {"resources/characters/CesiumMan.glb", glm::vec3(3.0f, gl::units::floorY, -3.0f), glm::vec3(0.0f), 1.0f, true},
    };

    const std::array<gl::PointLight, 4> pointLights_ = {
        // constant / linear / quadratic は逆二乗減衰に移行して未使用（構造体には残置）
        {// position, ambient, diffuse, specular, constant, linear, quadratic
         {glm::vec3(0.0f, 2.0f, 2.2f), glm::vec3(0.0f), glm::vec3(20.0f, 20.0f, 20.0f),
          glm::vec3(10.0f, 10.0f, 10.0f), 1.0f, 0.14f, 0.07f},
         // TODO: 裏面ライティングの検証中のみ移動。元の位置は (-5.0f, 0.8f, -4.0f)
         {glm::vec3(-14.5f, 1.5f, -11.0f), glm::vec3(0.0f), glm::vec3(11.0f, 2.0f, 1.25f),
          glm::vec3(5.5f, 1.0f, 0.6f), 1.0f, 0.14f, 0.07f},
         {glm::vec3(4.2f, 3.0f, 1.8f), glm::vec3(0.0f), glm::vec3(2.0f, 3.0f, 11.0f), glm::vec3(1.0f, 1.5f, 5.5f),
          1.0f, 0.14f, 0.07f},
         {glm::vec3(-1.5f, 3.0f, -2.2f), glm::vec3(0.0f), glm::vec3(1.75f, 9.0f, 2.5f), glm::vec3(0.9f, 4.5f, 1.25f),
          1.0f, 0.14f, 0.07f}}};

    // これは検証用なので後々消してもいい
    const std::vector<glm::vec3> windows_pos_ = {glm::vec3(-1.5f, 0.0f, -0.48f), glm::vec3(1.5f, 0.0f, 0.51f),
                                                 glm::vec3(0.0f, 0.0f, 0.7f), glm::vec3(-0.3f, 0.0f, -2.3f),
                                                 glm::vec3(0.5f, 0.0f, -0.6f), glm::vec3(-15.0f, 0.0f, -8.0f)};
};
