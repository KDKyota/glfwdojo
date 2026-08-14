#pragma once
#include "Camera.h"
#include "GeometryData.h"
#include "GlHandle.h"
#include "Lighting.h"
#include "Material.h"
#include "Model.h"
#include "PbrMaterial.h"
#include "Shader.h"
#include "TextureCache.h"
#include <array>
#include <memory>
#include <vector>

class Scene {
  public:
    Scene(std::shared_ptr<Camera> camera, int srcWindow, int scrHeight);
    void Render(float deltaTime, float heightScale);

    // ImGui のパネルから直接編集するためのアクセサ
    int &DebugMode() {
        return debugMode_;
    }
    bool &DebugRawOutput() {
        return debugRawOutput_;
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

  private:
    void initMesh();
    void initTextures();
    void initFramebuffer();
    void initGBuffer();
    void initSSAO();
    unsigned int loadTexture(const char *path, bool hasAlpha);
    void initUBO();

    // Render() から順に呼ばれるパス。FBO とテクスチャで繋がっているので順序に意味がある
    void updateTransparentInstances(std::vector<gl::TransparentDraw> &sorted);
    void updateMatricesUBO();
    void renderShadowPasses();
    void renderGeometryPass();
    void renderSSAOPass();
    void blitGeometryDepth();
    void renderDeferredLightingPass();
    void renderForwardPass(const std::vector<gl::TransparentDraw> &sorted);
    void renderBloomBlur();
    void renderToScreen();

    void applyPointLights(gl::Shader &shader);
    void renderCubes(gl::Shader &shader);
    void renderFloor(gl::Shader &shader);
    void renderLightCubes();
    void renderSkybox();
    void renderTransparentWindows(const std::vector<gl::TransparentDraw> &sorted);
    void renderWindow(gl::Shader &shader); // 窓枠用
    void renderWalls(gl::Shader &shader);

    static constexpr unsigned int SHADOW_WIDTH = 1024,
                                  SHADOW_HEIGHT = 1024; // depthCubemap_ 各面の解像度
    int scrWidth_, scrHeight_;
    static constexpr float shadowNearPlane_ = 1.0f; // 光源視点の投影のnear plane（shadowProj計算に使用）
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
        gAlbedoSpec_;                  // gBuffer_にアタッチする3枚のテクスチャ
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
    std::unique_ptr<gl::Shader> shader_;
    std::unique_ptr<gl::Shader> cubeShader_;
    std::unique_ptr<gl::Shader> transparentwindowShader_;
    std::unique_ptr<gl::Shader> lightcubeShader_;
    // std::unique_ptr<gl::Shader> shaderSingleColor_;
    std::unique_ptr<gl::Shader> screenshader_;
    // std::unique_ptr<gl::Shader> glasscubeShader_;
    std::unique_ptr<gl::Shader> skyboxShader_;
    // std::unique_ptr<Model> model_; // 3Dモデル
    std::shared_ptr<Camera> camera_;
    std::unique_ptr<gl::Shader> pointDepthShader_;
    std::unique_ptr<gl::Shader> pointColorShader_; // カラー付き透過シャドウ（ガラスの透過色）用
    std::unique_ptr<gl::Shader> debugDepthShader_;
    std::unique_ptr<gl::Shader> wallShader_;
    std::unique_ptr<gl::Shader> blurShader_; // Boolの際にぼかしを入れるシェーダ
    /* G-Bufferのシェーダ */
    std::unique_ptr<gl::Shader> gbufferFloorShader_;
    std::unique_ptr<gl::Shader> gbufferWallShader_;
    std::unique_ptr<gl::Shader> gbufferWindowShader_;
    std::unique_ptr<gl::Shader> gbufferCubeShader_;
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
    gl::TextureHandle cubemapTexture_;
    // 各テクセルは光源からの正規化距離
    std::array<gl::TextureHandle, 4> depthCubemap_;
    // 各テクセルはガラスを透過した光の色。ガラスを通らない方向は白
    std::array<gl::TextureHandle, 4> shadowColorCubemap_;

    /* ==== UI から実行時に変更する設定（毎フレーム送る） ==== */
    // 対応表は main.cpp の kDebugModes
    int debugMode_ = 0;
    // Bloom・トーンマッピング・ガンマ補正を飛ばす。デバッグ表示には必須
    bool debugRawOutput_ = false;
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
    float ambientStrength_ = 0.5f;             // SSAO が掛かるのはこの項だけ
    float bloomStrength_ = 1.0f;

    float elapsedTime_ = 0.0f;
    float heightScale_ = 0.0f;
    float exposure_ = 1.0f; // HDR 値自体は変えないので Bloom の閾値や AO のコントラストに影響しない

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

    const std::array<gl::PointLight, 4> pointLights_ = {
        // constant / linear / quadratic は逆二乗減衰に移行して未使用（構造体には残置）
        {// position, ambient, diffuse, specular, constant, linear, quadratic
         {glm::vec3(0.0f, 2.0f, 2.2f), glm::vec3(0.0f), glm::vec3(20.0f, 20.0f, 20.0f),
          glm::vec3(10.0f, 10.0f, 10.0f), 1.0f, 0.14f, 0.07f},
         {glm::vec3(-5.0f, 0.8f, -4.0f), glm::vec3(0.0f), glm::vec3(11.0f, 2.0f, 1.25f), glm::vec3(5.5f, 1.0f, 0.6f),
          1.0f, 0.14f, 0.07f},
         {glm::vec3(4.2f, 3.0f, 1.8f), glm::vec3(0.0f), glm::vec3(2.0f, 3.0f, 11.0f), glm::vec3(1.0f, 1.5f, 5.5f),
          1.0f, 0.14f, 0.07f},
         {glm::vec3(-1.5f, 3.0f, -2.2f), glm::vec3(0.0f), glm::vec3(1.75f, 9.0f, 2.5f), glm::vec3(0.9f, 4.5f, 1.25f),
          1.0f, 0.14f, 0.07f}}};

    const std::vector<glm::vec3> windows_pos_ = {glm::vec3(-1.5f, 0.0f, -0.48f), glm::vec3(1.5f, 0.0f, 0.51f),
                                                 glm::vec3(0.0f, 0.0f, 0.7f), glm::vec3(-0.3f, 0.0f, -2.3f),
                                                 glm::vec3(0.5f, 0.0f, -0.6f)};
};
