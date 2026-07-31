#pragma once
#include "Camera.h"
#include "GeometryData.h"
#include "Lighting.h"
#include "Material.h"
#include "Model.h"
#include "Shader.h"
#include "TextureCache.h"
#include <memory>
#include <vector>

class Scene {
public:
  Scene(std::shared_ptr<Camera> camera, int srcWindow, int scrHeight);
  ~Scene();
  void Render(float deltaTime, float heightScale);

  // ImGui のパネルから直接編集するためのアクセサ
  int &DebugMode() { return debugMode_; }
  bool &DebugRawOutput() { return debugRawOutput_; }
  float &SsaoStrength() { return ssaoStrength_; }
  float &Exposure() { return exposure_; }
  float &AmbientStrength() { return ambientStrength_; }
  float &BloomStrength() { return bloomStrength_; }

private:
  void initMesh();
  void initTextures();
  void initFramebuffer();
  void initGBuffer();
  void initSSAO();
  unsigned int loadTexture(const char *path, bool hasAlpha);
  void initUBO();
  void applyPointLights(gl::Shader &shader);
  void renderCubes(gl::Shader &shader);
  void renderFloor(gl::Shader &shader);
  void renderLightCubes();
  void renderSkybox();
  void renderTransparentWindows(const std::vector<gl::TransparentDraw> &sorted);
  void renderWalls(gl::Shader &shader);

  static constexpr unsigned int SHADOW_WIDTH = 1024,
                                SHADOW_HEIGHT =
                                    1024; // depthCubemap_ 各面の解像度
  int scrWidth_, scrHeight_;
  static constexpr float shadowNearPlane_ =
      1.0f; // 光源視点の投影のnear plane（shadowProj計算に使用）
  static constexpr float shadowFarPlane_ =
      50.0f; // 光源視点の投影のfar plane。シェーダー側の farPlane uniform

  /* メッシュのVAO / VBO / EBO */
  unsigned int cubeVAO_, planeVAO_, cubeVBO_, planeVBO_, transparentVAO_,
      transparentVBO_, quadVAO_, quadVBO_, skyboxVAO_, skyboxVBO_;
  unsigned int lightVAO_, lightVBO_;
  unsigned int cubeInstanceVBO_;
  unsigned int transparentInstanceVBO_;
  unsigned int cubeEBO_, planeEBO_, transparentEBO_;
  unsigned int wallVAO_, wallVBO_, wallEBO_;
  /* フレームバッファ */
  unsigned int framebuffer_, textureColorbuffer_, rbo_; // メンバ変数として持つ
  unsigned int
      depthMapFBO_[4]; // point shadow
                       // のデプスパス専用フレームバッファ（depthCubemap_
                       // をアタッチ）
  unsigned int pingpongFBO_
      [2]; // HDRレンダリング後のガウシアンブラー用フレームバッファ（pingpongColorbuffers_

  unsigned int pingpongColorbuffers_
      [2]; // HDRレンダリング後のガウシアンブラー用テクスチャ（pingpongFBO_

  unsigned int
      brightColorBuffer_; // HDRレンダリング後の明るい部分だけを抽出するためのテクスチャ（pingpongFBO_

  /* UBO */
  unsigned int matricesUBO_;
  /* G-Buffer */
  unsigned int gBuffer_;
  unsigned int gPosition_, gNormal_,
      gAlbedoSpec_;        // gBuffer_にアタッチする3枚のテクスチャ
  unsigned int gDepthRBO_; // gBuffer_ 用の深度バッファ

  Material material_;
  TextureCache cache_;

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
  std::unique_ptr<gl::Shader> debugDepthShader_;
  std::unique_ptr<gl::Shader> wallShader_;
  std::unique_ptr<gl::Shader> blurShader_; // Boolの際にぼかしを入れるシェーダ
  /* G-Bufferのシェーダ */
  std::unique_ptr<gl::Shader> gbufferFloorShader_;
  std::unique_ptr<gl::Shader> gbufferWallShader_;
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
  unsigned int cubemapTexture_;
  unsigned int depthCubemap_
      [4]; // ポイントシャドウ用キューブマップ（各テクセルは光源からの正規化距離

  /* ==== UI から実行時に変更する設定（毎フレーム送る） ==== */
  // 0=通常 / 1=ライト0のシャドウ / 2=shadowMap[0]の生値 / 3=Albedo
  // 4=Normal / 5=Position / 6=4分割 / 7=SSAO
  int debugMode_ = 0;
  // Bloom・トーンマッピング・ガンマ補正を飛ばす。デバッグ表示には必須
  bool debugRawOutput_ = false;
  // SSAO の効き具合（0.0 = 無効, 1.0 = そのまま適用）
  float ssaoStrength_ = 1.0f;

  /* SSAO */
  unsigned int ssaoFBO_, ssaoBlurFBO_;
  unsigned int ssaoColorBuffer_, ssaoColorBufferBlur_; // GL_R8
  unsigned int noiseTexture_;                          // 4x4 GL_RGBA16F
  std::vector<glm::vec3> ssaoKernel_;                  // 接空間のサンプル点

  std::unique_ptr<gl::Shader> ssaoShader_;
  std::unique_ptr<gl::Shader> ssaoBlurShader_;

  static constexpr unsigned int SSAO_KERNEL_SIZE = 64; // 遮蔽物とみなす近傍の半径。暗くなる帯の幅を決める。目安は物体サイズの
                                                       // 0.2〜1.0 倍
  static constexpr float SSAO_RADIUS =
      0.6f; // 自己遮蔽によるアクネ対策。radius を変えたら比例させること
  static constexpr float SSAO_BIAS =
      0.03f; // AO のコントラスト。pow(ao, SSAO_POWER)。実用範囲は 1.5〜3.0
  static constexpr float SSAO_POWER =
      2.0f; // シーン全体の環境光。SSAO が掛かるのはこの項だけなので、0 にすると
            // AO も見えなくなる
  float ambientStrength_ = 0.3f;
  // Bloom の合成強度。0.0 で完全に無効化できる
  float bloomStrength_ = 1.0f;

  float elapsedTime_ = 0.0f;
  float heightScale_ =
      0.0f; // HDR tone mapping の明るさ調整。HDR値自体は変えないので、Bloom
            // の閾値や飽和に影響せず、AO
            // や影のコントラストを保ったまま明るくできる
  float exposure_ = 0.5f;
  int viewLoc_ = -1; // viewのローケーション番号(初期値-1)
  bool blurEnable_ = true;
  bool horizontal_ = true;
  std::vector<glm::vec3>
      transparent_positions_; // 透過オブジェクトのモデル行列を格納する配列

  static constexpr float animationTime_ = 5.0f;
  std::vector<glm::vec3> cubePositions_;

  /* シーン固有の配置データ */
  const std::vector<glm::vec3> cube_pos_ = {
      glm::vec3(-1.0f, 0.0f, -1.0f), glm::vec3(2.0f, 0.0f, 0.0f),
      glm::vec3(0.0f, 0.0f, 2.5f), // 近接配置（中心間 1.2 = 隙間 0.2）
      glm::vec3(1.2f, 0.0f, 2.5f),   glm::vec3(-1.0f, 1.0f, -1.0f), // 積み重ね
      glm::vec3(0.0f, 0.0f, -20.0f), // z=-25 壁の前
      glm::vec3(0.0f, 0.0f, 20.0f),  // z=+25 壁の前
  };

  const std::array<gl::PointLight, 4> pointLights_ = {
      {// position, ambient, diffuse, specular, constant, linear, quadratic
       {glm::vec3(0.0f, 2.0f, 2.2f), glm::vec3(0.0f),
        glm::vec3(4.0f, 4.0f, 4.0f), glm::vec3(2.0f, 2.0f, 2.0f), 1.0f, 0.14f,
        0.07f},
       {glm::vec3(-5.0f, 0.8f, -4.0f), glm::vec3(0.0f),
        glm::vec3(2.2f, 0.4f, 0.25f), glm::vec3(1.1f, 0.2f, 0.12f), 1.0f, 0.14f,
        0.07f},
       {glm::vec3(4.2f, 3.0f, 1.8f), glm::vec3(0.0f),
        glm::vec3(0.4f, 0.6f, 2.2f), glm::vec3(0.2f, 0.3f, 1.1f), 1.0f, 0.14f,
        0.07f},
       {glm::vec3(-1.5f, 3.0f, -2.2f), glm::vec3(0.0f),
        glm::vec3(0.35f, 1.8f, 0.5f), glm::vec3(0.18f, 0.9f, 0.25f), 1.0f,
        0.14f, 0.07f}}};

  // 透過オブジェクトの位置
  const std::vector<glm::vec3> windows_pos_ = {
      glm::vec3(-1.5f, 0.0f, -0.48f), glm::vec3(1.5f, 0.0f, 0.51f),
      glm::vec3(0.0f, 0.0f, 0.7f), glm::vec3(-0.3f, 0.0f, -2.3f),
      glm::vec3(0.5f, 0.0f, -0.6f)};
};
