#include "Scene.h"
#include <cstddef>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <map>
#include <random>

Scene::Scene(std::shared_ptr<Camera> camera, int scrWidth, int scrHeight)
    : camera_(camera), scrWidth_(scrWidth), scrHeight_(scrHeight) {
  shader_ = std::make_unique<gl::Shader>("shader.vert", "shader.frag");
  // cubeShader_ = std::make_unique<gl::Shader>("cube.vert", "cube.frag");
  lightcubeShader_ =
      std::make_unique<gl::Shader>("light_cube.vert", "light_cube.frag");
  // shaderSingleColor_ = std::make_unique<gl::Shader>("shader.vert",
  // "stencil_single_color.frag"); glasscubeShader_ =
  // std::make_unique<gl::Shader>("glasscube.vert", "glasscube.frag"); model_ =
  // std::make_unique<Model>("resources\\objects\\backpack\\backpack.obj",
  // cache_);
  screenshader_ =
      std::make_unique<gl::Shader>("fragment_quad.vert", "hdr.frag");
  skyboxShader_ = std::make_unique<gl::Shader>("skybox.vert", "skybox.frag");
  transparentwindowShader_ =
      std::make_unique<gl::Shader>("window.vert", "shader.frag");
  pointDepthShader_ = std::make_unique<gl::Shader>("point_shadow_depth.vert",
                                                   "point_shadow_depth.geom",
                                                   "point_shadow_depth.frag");
  debugDepthShader_ =
      std::make_unique<gl::Shader>("fragment_quad.vert", "debug_depth.frag");
  // wallShader_ = std::make_unique<gl::Shader>("wall.vert", "wall.frag");
  blurShader_ = std::make_unique<gl::Shader>("fragment_quad.vert", "blur.frag");

  /* G-Buffer */
  gbufferFloorShader_ =
      std::make_unique<gl::Shader>("shader.vert", "gbuffer_floor.frag");
  gbufferCubeShader_ =
      std::make_unique<gl::Shader>("cube.vert", "gbuffer_cube.frag");
  gbufferWallShader_ =
      std::make_unique<gl::Shader>("wall.vert", "gbuffer_wall.frag");
  deferredLightingShader_ = std::make_unique<gl::Shader>(
      "fragment_quad.vert", "deferred_lighting.frag");

  /* SSAO */
  ssaoShader_ = std::make_unique<gl::Shader>("fragment_quad.vert", "ssao.frag");
  ssaoBlurShader_ =
      std::make_unique<gl::Shader>("fragment_quad.vert", "ssao_blur.frag");

  // cubePositions_ = std::move(cubePositions);

  initMesh();
  initTextures();
  initFramebuffer();
  initUBO();
  initGBuffer();
  initSSAO();
}

void Scene::initMesh() {
  int stride = sizeof(gl::Vertex);

  glGenVertexArrays(1, &cubeVAO_); // cube用のVAO
  glGenBuffers(1, &cubeVBO_);
  glGenBuffers(1, &cubeInstanceVBO_);
  glGenBuffers(1, &cubeEBO_);
  glBindVertexArray(cubeVAO_);
  glBindBuffer(GL_ARRAY_BUFFER, cubeVBO_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(gl::cubeVertices),
               gl::cubeVertices.data(), GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEBO_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(gl::cubeIndices),
               gl::cubeIndices.data(), GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride,
                        (void *)offsetof(gl::Vertex, position));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                        (void *)offsetof(gl::Vertex, normal));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride,
                        (void *)offsetof(gl::Vertex, uv));
  glEnableVertexAttribArray(3);
  glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride,
                        (void *)offsetof(gl::Vertex, tangent));
  glEnableVertexAttribArray(4);
  glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, stride,
                        (void *)offsetof(gl::Vertex, bitangent));
  glBindBuffer(GL_ARRAY_BUFFER, cubeInstanceVBO_);
  glBufferData(GL_ARRAY_BUFFER, cube_pos_.size() * sizeof(glm::vec3),
               cube_pos_.data(), GL_STATIC_DRAW);
  glEnableVertexAttribArray(5);
  glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);
  glVertexAttribDivisor(5, 1); // インスタンスごとに変化する属性
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  glGenVertexArrays(1, &planeVAO_); // 床用のVAO
  glGenBuffers(1, &planeVBO_);
  glGenBuffers(1, &planeEBO_);
  glBindVertexArray(planeVAO_);
  glBindBuffer(GL_ARRAY_BUFFER, planeVBO_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(gl::planeVertices),
               gl::planeVertices.data(), GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, planeEBO_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(gl::planeIndices),
               gl::planeIndices.data(), GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride,
                        (void *)offsetof(gl::Vertex, position));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                        (void *)offsetof(gl::Vertex, normal));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride,
                        (void *)offsetof(gl::Vertex, uv));
  glBindVertexArray(0);

  glGenVertexArrays(1, &transparentVAO_); // 透過窓のVAO
  glGenBuffers(1, &transparentVBO_);
  glGenBuffers(1, &transparentInstanceVBO_);
  glGenBuffers(1, &transparentEBO_);
  glBindVertexArray(transparentVAO_);
  glBindBuffer(GL_ARRAY_BUFFER, transparentVBO_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(gl::transparentVertices),
               gl::transparentVertices.data(), GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, transparentEBO_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(gl::transparentIndices),
               gl::transparentIndices.data(), GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride,
                        (void *)offsetof(gl::Vertex, position));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                        (void *)offsetof(gl::Vertex, normal));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride,
                        (void *)offsetof(gl::Vertex, uv));
  glBindBuffer(GL_ARRAY_BUFFER, transparentInstanceVBO_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * windows_pos_.size(),
               nullptr, GL_DYNAMIC_DRAW);

  glEnableVertexAttribArray(5);
  glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);
  glVertexAttribDivisor(5, 1); // インスタンスごとに変化する属性
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  // skybox
  glGenVertexArrays(1, &skyboxVAO_); // スカイボックスのVAO
  glGenBuffers(1, &skyboxVBO_);
  glBindVertexArray(skyboxVAO_);
  glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO_);
  glBufferData(GL_ARRAY_BUFFER, gl::skyboxVertices.size() * sizeof(glm::vec3),
               gl::skyboxVertices.data(), GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(gl::skyboxVertices[0]),
                        (void *)0);
  glBindVertexArray(0);

  // 壁 (Normal Mapping 用: location 0-4 を使用)
  glGenVertexArrays(1, &wallVAO_);
  glGenBuffers(1, &wallVBO_);
  glGenBuffers(1, &wallEBO_);
  glBindVertexArray(wallVAO_);
  glBindBuffer(GL_ARRAY_BUFFER, wallVBO_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(gl::wallVertices),
               gl::wallVertices.data(), GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wallEBO_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(gl::wallIndices),
               gl::wallIndices.data(), GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride,
                        (void *)offsetof(gl::Vertex, position));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                        (void *)offsetof(gl::Vertex, normal));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride,
                        (void *)offsetof(gl::Vertex, uv));
  glEnableVertexAttribArray(3);
  glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride,
                        (void *)offsetof(gl::Vertex, tangent));
  glEnableVertexAttribArray(4);
  glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, stride,
                        (void *)offsetof(gl::Vertex, bitangent));
  glBindVertexArray(0);

  // screen quad
  glGenVertexArrays(1, &quadVAO_); // スクリーンテクスチャのVAO
  glGenBuffers(1, &quadVBO_);
  glBindVertexArray(quadVAO_);
  glBindBuffer(GL_ARRAY_BUFFER, quadVBO_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(gl::quadVertices), gl::quadVertices,
               GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                        (void *)(2 * sizeof(float)));
  glBindVertexArray(0);

  // glBindVertexArray(VAO_);
  // glBindBuffer(GL_ARRAY_BUFFER, VBO_);
  // glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride,
  // (void*)offsetof(gl::Vertex, position)); glEnableVertexAttribArray(0);

  // glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
  // (void*)offsetof(gl::Vertex, normal)); glEnableVertexAttribArray(1);

  // glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride,
  // (void*)offsetof(gl::Vertex, uv)); glEnableVertexAttribArray(2);
}

void Scene::initTextures() {
  // material_.diffuse = cache_.get("resources\\textures\\container2.png",
  // false); material_.specular =
  // cache_.get("resources\\textures\\container2_specular.png", true);
  cubeTexture_ = cache_.get("resources/textures/bricks2.jpg", true);
  cubeNormalMap_ = cache_.get("resources/textures/bricks2_normal.jpg", true);
  cubeHeightMap_ = cache_.get("resources/textures/bricks2_disp.jpg", true);

  floorTexture_ = cache_.get("resources/textures/wood.png", true);
  transparentTexture_ = cache_.get("resources/textures/window.png", true);
  std::vector<std::string> faces{"resources/textures/skybox/right.jpg",
                                 "resources/textures/skybox/left.jpg",
                                 "resources/textures/skybox/top.jpg",
                                 "resources/textures/skybox/bottom.jpg",
                                 "resources/textures/skybox/front.jpg",
                                 "resources/textures/skybox/back.jpg"};
  cubemapTexture_ = cache_.loadCubemap(faces, false);

  brickwallTexture_ = cache_.get("resources/textures/brickwall.jpg", true);
  brickwallNormalTexture_ =
      cache_.get("resources/textures/brickwall_normal.jpg", true);

  /* cube */
  // use G-Buffer in cube, floor, wall
  gbufferCubeShader_->use();
  gbufferCubeShader_->setInt("diffuseMap", 0);
  gbufferCubeShader_->setInt("normalMap", 1);
  gbufferCubeShader_->setInt("heightMap", 2);
  /* floor */
  gbufferFloorShader_->use();
  gbufferFloorShader_->setInt("diffuseMap", 0);
  /* wall */
  gbufferWallShader_->use();
  gbufferWallShader_->setInt("diffuseMap", 0);
  gbufferWallShader_->setInt("normalMap", 1);
  /* transparent window */
  transparentwindowShader_->use();
  transparentwindowShader_->setInt("texture1", 0);
  for (unsigned int i = 0; i < 4; ++i)
    transparentwindowShader_->setInt("shadowMap[" + std::to_string(i) + "]",
                                     3 + i);
  transparentwindowShader_->setFloat("farPlane", shadowFarPlane_);
  /* screen */
  screenshader_->use();
  screenshader_->setInt("screenTexture", 0);
  screenshader_->setInt("bloomBlur", 1);
  /* skybox */
  skyboxShader_->use();
  skyboxShader_->setInt("skybox", 0);
  /* depth */
  debugDepthShader_->use();
  debugDepthShader_->setInt("depthMap", 0);
  debugDepthShader_->setFloat("near_plane", shadowNearPlane_);
  debugDepthShader_->setFloat("far_plane", shadowFarPlane_);
  // material_.setUniforms(*shader_);
  blurShader_->use();
  blurShader_->setInt("image", 0);
  /* deferred lighting */
  deferredLightingShader_->use();
  deferredLightingShader_->setInt("gPosition", 0);
  deferredLightingShader_->setInt("gNormal", 1);
  deferredLightingShader_->setInt("gAlbedoSpec", 2);
  for (unsigned int i = 0; i < 4; ++i)
    deferredLightingShader_->setInt("shadowMap[" + std::to_string(i) + "]",
                                    3 + i);
  deferredLightingShader_->setFloat("farPlane", shadowFarPlane_);
  // 既存の割り当て（0〜2=G-Buffer, 3〜6=shadowMap）を壊さないよう 7 を使う
  deferredLightingShader_->setInt("ssao", 7);
  deferredLightingShader_->setFloat("ambientStrength", AMBIENT_STRENGTH);
  // ssaoShader_ / ssaoBlurShader_ の uniform は initSSAO() 側で設定する。
  // initTextures() は initSSAO() より先に呼ばれるため、ここではまだ
  // ssaoKernel_ が空で、カーネルを送れないため。
}

void Scene::initFramebuffer() {
  /* framebuffer configuration */
  glGenFramebuffers(
      1, &framebuffer_); // フレームバッファ（通常は *FBO とかに命名するけど…）
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
  // create a color attachment texture（通常のカラーバッファをアタッチメント
  // location=0 -> FragColor）
  glGenTextures(1,
                &textureColorbuffer_); // 最終的に画面に貼り付けるカラーバッファ
  glBindTexture(GL_TEXTURE_2D, textureColorbuffer_);
  // RGB16F はカラーレンダリング可能が保証されていないフォーマットなので RGBA16F
  // を使う（initGBuffer のコメント参照）
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, scrWidth_, scrHeight_, 0, GL_RGBA,
               GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         textureColorbuffer_, 0);
  // 明るい部分のカラーバッファをアタッチメント（location = 1 -> BrightColor）
  glGenTextures(1, &brightColorBuffer_);
  glBindTexture(GL_TEXTURE_2D, brightColorBuffer_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, scrWidth_, scrHeight_, 0, GL_RGBA,
               GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                         brightColorBuffer_, 0);
  unsigned int attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
  glDrawBuffers(2, attachments);

  /* create a renderbuffer object for depth and stencil attachment(we won't be
   * sampling these) */
  glGenRenderbuffers(1, &rbo_); // デプスやステンシルの処理を行う
  glBindRenderbuffer(GL_RENDERBUFFER, rbo_);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, scrWidth_,
                        scrHeight_); // use a single renderbuffer object for
                                     // both a depth AND stencil buffer.
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                            GL_RENDERBUFFER, rbo_); // now actually attach it
  // now that we actually created the framebuffer and added all attachments we
  // want to check if it is actually complete now
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!"
              << std::endl;
  glBindFramebuffer(GL_FRAMEBUFFER, 0); // フレームバッファの初期化処理

  /* ポイントシャドウ用キューブマップFBO */
  // depthMapFBO_: カラーバッファを持たず、深度だけを depthCubemap_
  // に書き込む専用FBO
  glGenFramebuffers(4, depthMapFBO_);
  // depthCubemap_: 6面ぶんの深度テクスチャ。各テクセルには
  // point_shadow_depth.frag が書き込む 「光源からの正規化距離
  // [0,1]」が入る（通常のcubemapのような色情報ではない点に注意）
  glGenTextures(4, depthCubemap_);
  for (unsigned int j = 0; j < 4; ++j)

  {
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap_[j]);
    for (unsigned int i = 0; i < 6; ++i)
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT,
                   SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT,
                   nullptr);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO_[j]);
    // depthCubemap_ をFBOの深度アタッチメントに設定。glFramebufferTexture
    // (Texture2Dではない) を使うことで
    // 6面すべてが1つのアタッチメントとして扱われ、geometry
    // shaderのgl_Layerで面を選択できるようになる
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap_[j],
                         0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  /* ぼかし処理を書き込むFBO */
  glGenFramebuffers(2, pingpongFBO_);
  glGenTextures(2, pingpongColorbuffers_);
  // 縦方向と横方向にガウシアンブラー（ぼかし手法）をかけるのでそのために二回のループ
  for (unsigned int i = 0; i < 2; i++) {
    glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO_[i]);
    glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers_[i]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, scrWidth_, scrHeight_, 0,
                 GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(
        GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
        GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would
                           // otherwise sample repeated texture values!
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           pingpongColorbuffers_[i], 0);
    // also check if framebuffers are complete (no need for depth buffer)
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
      std::cout << "Framebuffer not complete!" << std::endl;
  }
}

void Scene::initUBO() {
  glGenBuffers(1, &matricesUBO_);
  glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO_);
  glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), nullptr,
               GL_STATIC_DRAW);
  glBindBufferBase(GL_UNIFORM_BUFFER, 0, matricesUBO_);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void Scene::initGBuffer() {
  // gBuffer_にバインド
  glGenFramebuffers(1, &gBuffer_);
  glBindFramebuffer(GL_FRAMEBUFFER, gBuffer_);

  // 内部フォーマットに GL_RGB16F を使ってはいけない。
  // OpenGL の必須フォーマット表では RGB16F
  // は「テクスチャとしては必須／カラーレンダリング可能は非必須」に
  // 分類されており、環境によってはテクスチャ作成も glCheckFramebufferStatus
  // も通るのに 書き込みだけが正しく行われない。3成分しか使わなくても RGBA16F
  // を使うこと。
  // gPosition（ワールド座標。負値や1を超える値を持つため浮動小数点フォーマットが必要）
  glGenTextures(1, &gPosition_);
  glBindTexture(GL_TEXTURE_2D, gPosition_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, scrWidth_, scrHeight_, 0, GL_RGBA,
               GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  // SSAO はサンプル点を投影した位置をサンプルするため、画面端では [0,1] の外に出る。
  // デフォルトの GL_REPEAT のままだと画面の反対側の値を拾い、画面端に不自然な遮蔽が出る。
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         gPosition_, 0);
  // gNormal（法線。[-1,1]
  // の負値を保持する必要があるため浮動小数点フォーマット）
  glGenTextures(1, &gNormal_);
  glBindTexture(GL_TEXTURE_2D, gNormal_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, scrWidth_, scrHeight_, 0, GL_RGBA,
               GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                         gNormal_, 0);
  // gAlbedoSpec（rgb=アルベド, a=スペキュラ強度。いずれも [0,1]
  // なので8bitで十分）
  glGenTextures(1, &gAlbedoSpec_);
  glBindTexture(GL_TEXTURE_2D, gAlbedoSpec_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, scrWidth_, scrHeight_, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D,
                         gAlbedoSpec_, 0);

  unsigned int attachments[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
                                 GL_COLOR_ATTACHMENT2};
  glDrawBuffers(3, attachments);

  // --- Depth Buffer ---
  glGenRenderbuffers(1, &gDepthRBO_);
  glBindRenderbuffer(GL_RENDERBUFFER, gDepthRBO_);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, scrWidth_,
                        scrHeight_);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                            GL_RENDERBUFFER, gDepthRBO_);
  // エラーチェック
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    std::cout << "ERROR::G-BUFFER:: Framebuffer is not complete!" << std::endl;
  glBindFramebuffer(GL_FRAMEBUFFER, 0); // フレームバッファをデフォルトに戻す
}

void Scene::initSSAO() {
  std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
  std::default_random_engine generator;

  /* --- サンプルカーネル --- */
  // 接空間（+Z が法線方向）における、半球内のサンプル点のテンプレート。
  ssaoKernel_.reserve(SSAO_KERNEL_SIZE);
  for (unsigned int i = 0; i < SSAO_KERNEL_SIZE; ++i) {
    // z を 0 以上にすることで「球」ではなく「半球」になる。
    // 球にすると平坦な面でも約半分のサンプルが面の裏側に入って遮蔽と判定され、
    // シーン全体が一様に灰色がかってしまう。
    glm::vec3 sample(randomFloats(generator) * 2.0f - 1.0f,
                     randomFloats(generator) * 2.0f - 1.0f,
                     randomFloats(generator));
    sample = glm::normalize(sample);
    sample *= randomFloats(generator); // 半球の表面ではなく内部に散らす

    // 遮蔽への寄与は近い遮蔽物ほど大きいので、限られたサンプル数を原点付近に厚く配分する。
    // 均等に散らすと遠くのサンプルばかりになり、隅の暗さが出ない。
    float scale = static_cast<float>(i) / static_cast<float>(SSAO_KERNEL_SIZE);
    scale = 0.1f + 0.9f * scale * scale; // 二次関数で原点寄りに偏らせる
    sample *= scale;

    ssaoKernel_.push_back(sample);
  }

  /* --- ノイズテクスチャ --- */
  // ピクセルごとにカーネルを法線まわりにランダム回転させるためのベクトル。
  // 全ピクセルで同じカーネルを使うと規則的な縞（バンディング）が出るので、
  // 4x4 をタイル状に敷いて回転させ、その代償のノイズを後段の 4x4 ブラーで打ち消す。
  std::vector<glm::vec3> ssaoNoise;
  ssaoNoise.reserve(16);
  for (unsigned int i = 0; i < 16; ++i) {
    // z = 0 にするのは「Z軸まわりの回転」にしたいから
    ssaoNoise.emplace_back(randomFloats(generator) * 2.0f - 1.0f,
                           randomFloats(generator) * 2.0f - 1.0f, 0.0f);
  }
  glGenTextures(1, &noiseTexture_);
  glBindTexture(GL_TEXTURE_2D, noiseTexture_);
  // 負の値を保持する必要があるため浮動小数点フォーマット。
  // GL_RGBA8 だと [0,1] に丸められて回転ベクトルとして機能しなくなる。
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 4, 4, 0, GL_RGB, GL_FLOAT,
               ssaoNoise.data());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  // タイル状に敷き詰めるので GL_REPEAT が必須
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  /* --- SSAO パスの出力先 --- */
  // 遮蔽率はスカラー [0,1] なので 1チャンネル 8bit で十分
  glGenFramebuffers(1, &ssaoFBO_);
  glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO_);
  glGenTextures(1, &ssaoColorBuffer_);
  glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, scrWidth_, scrHeight_, 0, GL_RED,
               GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         ssaoColorBuffer_, 0);
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    std::cout << "ERROR::SSAO:: Framebuffer is not complete!" << std::endl;

  /* --- ブラーパスの出力先 --- */
  glGenFramebuffers(1, &ssaoBlurFBO_);
  glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO_);
  glGenTextures(1, &ssaoColorBufferBlur_);
  glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, scrWidth_, scrHeight_, 0, GL_RED,
               GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         ssaoColorBufferBlur_, 0);
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    std::cout << "ERROR::SSAO_BLUR:: Framebuffer is not complete!" << std::endl;

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  /* --- シェーダーの uniform 設定 --- */
  // initTextures() ではなくここで設定するのは、カーネルを送るのに
  // 上で生成した ssaoKernel_ が必要なため（initTextures() は initSSAO() より先に呼ばれる）。
  ssaoShader_->use();
  ssaoShader_->setInt("gPosition", 0);
  ssaoShader_->setInt("gNormal", 1);
  ssaoShader_->setInt("texNoise", 2);
  ssaoShader_->setFloat("radius", SSAO_RADIUS);
  ssaoShader_->setFloat("bias", SSAO_BIAS);
  // カーネルは実行中に変化しないので一度だけ送れば十分
  for (unsigned int i = 0; i < SSAO_KERNEL_SIZE; ++i)
    ssaoShader_->setVec3("samples[" + std::to_string(i) + "]", ssaoKernel_[i]);

  ssaoBlurShader_->use();
  ssaoBlurShader_->setInt("ssaoInput", 0);
  ssaoBlurShader_->setFloat("power", SSAO_POWER);
}

void Scene::Render(float deltaTime, float heightScale) {
  elapsedTime_ += deltaTime;
  heightScale_ = heightScale; // Parallax Mapping の強さ（Callbacks.cpp の
                              // processInput で矢印キーにより更新される）

  /* 透過窓をカメラからの距離でソート */
  std::vector<gl::TransparentDraw> sorted;
  for (unsigned int i = 0; i < windows_pos_.size(); ++i)
    sorted.push_back(
        {glm::length(camera_->GetViewPosition() - windows_pos_[i]), i});
  std::sort(sorted.begin(), sorted.end(),
            [](const gl::TransparentDraw &a, const gl::TransparentDraw &b) {
              return a.distance > b.distance;
            });

  glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
  glEnable(GL_DEPTH_TEST);
  pointDepthShader_->use();
  for (unsigned int j = 0; j < 4; ++j) {
    // ポイントシャドウ用: 光源から6方向へのライト空間行列を計算
    // lightPos:
    // シャドウを落とす点光源の位置。6方向すべての視点(lookAt)の原点になる
    glm::vec3 lightPos = pointLights_[j].position;
    // shadowProj:
    // 立方体の1面をちょうど覆う画角(90度)の透視投影行列。6面共通で使い回す
    // near/far は shadowNearPlane_ / shadowFarPlane_ を使用（far
    // は深度正規化の基準にもなる）
    glm::mat4 shadowProj = glm::perspective(
        glm::radians(90.0f), (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT,
        shadowNearPlane_, shadowFarPlane_);
    // shadowTransforms: cubemapの+X,-X,+Y,-Y,+Z,-Zの6面それぞれに対応する
    // view*projection 行列 この配列を point_shadow_depth.geom の
    // shadowMatrices[6] にそのまま渡す
    std::vector<glm::mat4> shadowTransforms;
    shadowTransforms.push_back(
        shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1, 0, 0),
                                 glm::vec3(0, -1, 0)));
    shadowTransforms.push_back(
        shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1, 0, 0),
                                 glm::vec3(0, -1, 0)));
    shadowTransforms.push_back(
        shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0, 1, 0),
                                 glm::vec3(0, 0, 1)));
    shadowTransforms.push_back(
        shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0, -1, 0),
                                 glm::vec3(0, 0, -1)));
    shadowTransforms.push_back(
        shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0, 0, 1),
                                 glm::vec3(0, -1, 0)));
    shadowTransforms.push_back(
        shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0, 0, -1),
                                 glm::vec3(0, -1, 0)));

    /* ここから、シャドウデプスの作成 */
    /* ── Pass 1: Point Shadow Depth Pass ── */
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO_[j]);
    glClear(GL_DEPTH_BUFFER_BIT);
    // shadowTransforms[6] を geometry shader の uniform 配列 shadowMatrices[6]
    // に渡す
    for (int i = 0; i < 6; ++i)
      pointDepthShader_->setMat4("shadowMatrices[" + std::to_string(i) + "]",
                                 shadowTransforms[i]);
    // フラグメントシェーダー側で距離を正規化する際の基準値（shader.frag側の
    // farPlane と揃える）
    pointDepthShader_->setFloat("farPlane", shadowFarPlane_);
    // フラグメントシェーダー側で「光源からの距離」を計算するための光源位置
    pointDepthShader_->setVec3("lightPos", lightPos);
    renderFloor(*pointDepthShader_);
    renderCubes(*pointDepthShader_);
    renderWalls(*pointDepthShader_);
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  /* -- deferred shading Geometry pass --*/
  glViewport(0, 0, scrWidth_, scrHeight_); // viewport を元に戻す
  glEnable(GL_DEPTH_TEST);
  // G-Buffer
  // に入るのは「色」ではなく座標・法線という幾何情報なので、絶対にブレンドしてはいけない。
  // Window.cpp で透過窓のために glEnable(GL_BLEND) されたままだと、
  // gPosition / gNormal は out vec3（アルファ成分が未定義＝実質0）なので
  // src.rgb * 0 + dst.rgb * 1 となって書き込みが丸ごと消え、G-Buffer
  // がクリア値のままになる。
  glDisable(GL_BLEND);

  /* write view / projection into UBO*/
  glm::mat4 view = camera_->GetViewMatrix();
  glm::mat4 projection =
      glm::perspective(glm::radians(camera_->GetZoomValue()),
                       (float)scrWidth_ / (float)scrHeight_, 0.1f, 100.0f);
  glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO_);
  glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4),
                  glm::value_ptr(view));
  glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4),
                  glm::value_ptr(projection));
  glBindBuffer(GL_UNIFORM_BUFFER, 0);

  glBindFramebuffer(GL_FRAMEBUFFER, gBuffer_);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  /* cube */
  // cube は閉じた立体なので裏面を描く必要がない。カリングしておくと、
  // Parallax Occlusion Mapping の discard でシルエット付近に穴が開いたときに
  // 内側（反対側の面）が透けて見えるアーティファクトを防げる。
  // 床・壁・透過窓・スカイボックスは片面／両面ポリゴンでカリングすると消えるため、cube
  // の描画中だけ有効にする。
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  gbufferCubeShader_->use();
  gbufferCubeShader_->setVec3("viewPos", camera_->GetViewPosition());
  gbufferCubeShader_->setMat3("normalMatrix", glm::mat3(1.0f));
  gbufferCubeShader_->setFloat("heightScale", heightScale_);
  renderCubes(*gbufferCubeShader_);
  glDisable(GL_CULL_FACE);

  /* floor */
  gbufferFloorShader_->use();
  gbufferFloorShader_->setMat3("normalMatrix", glm::mat3(1.0f));
  renderFloor(*gbufferFloorShader_);

  /* wall */
  gbufferWallShader_->use();
  renderWalls(*gbufferWallShader_);

  /* -- SSAO pass -- */
  // G-Buffer さえあれば計算できるので、Geometryパスの直後に置いている。
  // フルスクリーンクワッドを1枚描くだけなので深度テストは不要。
  glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO_);
  glClear(GL_COLOR_BUFFER_BIT);
  glDisable(GL_DEPTH_TEST);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, gPosition_);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, gNormal_);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, noiseTexture_);
  ssaoShader_->use();
  glBindVertexArray(quadVAO_);
  glDrawArrays(GL_TRIANGLES, 0, 6);

  /* -- SSAO blur pass -- */
  // 4x4 のノイズをタイル状に敷いた代償として出る格子模様を、4x4 の平均で打ち消す
  glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO_);
  glClear(GL_COLOR_BUFFER_BIT);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer_);
  ssaoBlurShader_->use();
  glBindVertexArray(quadVAO_);
  glDrawArrays(GL_TRIANGLES, 0, 6);

  glEnable(GL_DEPTH_TEST);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  /* copy Geometry pass's depthMap to framebuffer_ (to test for transparent
   * window ex.)*/
  glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer_);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer_);
  glBlitFramebuffer(0, 0, scrWidth_, scrHeight_, 0, 0, scrWidth_, scrHeight_,
                    GL_DEPTH_BUFFER_BIT, GL_NEAREST);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  /* -- Deferred shading Lighting Pass -- */
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
  glClear(GL_COLOR_BUFFER_BIT); // 深度は上でコピー済みなのでクリアしない
  glDisable(GL_DEPTH_TEST);     // フルスクリーンクワッドなので深度テスト不要
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, gPosition_);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, gNormal_);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, gAlbedoSpec_);
  for (unsigned int j = 0; j < 4; ++j) {
    glActiveTexture(GL_TEXTURE3 + j);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap_[j]);
  }
  // ブラー後のAOをユニット7へ（3〜6は shadowMap が使っている）
  glActiveTexture(GL_TEXTURE7);
  glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur_);
  deferredLightingShader_->use();
  deferredLightingShader_->setVec3("viewPos", camera_->GetViewPosition());
  // UI から変更される設定は毎フレーム送る。
  // initTextures() で一度だけ送ると、値を変えても反映されない。
  deferredLightingShader_->setInt("debugMode", debugMode_);
  deferredLightingShader_->setFloat("ssaoStrength", ssaoStrength_);
  applyPointLights(*deferredLightingShader_);
  glBindVertexArray(quadVAO_);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glEnable(GL_DEPTH_TEST);

  /* ── Pass 2: Main Pass ── （For transparent window and skybox, light cube
   * cannot be used G-Buffer）*/
  glViewport(0, 0, scrWidth_, scrHeight_);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
  glEnable(GL_DEPTH_TEST);

  /* ライトキューブ（FBO バインド中に描画）*/
  renderLightCubes();

  /* スカイボックス */
  renderSkybox();

  /* 透過窓（ブレンドが必要なのはここだけ。Geometryパスの冒頭で無効化しているので、描画中だけ有効にする）*/
  glEnable(GL_BLEND);
  renderTransparentWindows(sorted);
  glDisable(GL_BLEND);

  /* blur bright fragments with two - pass Gaussian blur */
  glDisable(GL_DEPTH_TEST);
  bool first_iteration = true;
  unsigned int amount = 10;
  blurShader_->use();
  for (unsigned int i = 0; i < amount; ++i) {
    glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO_[horizontal_]);
    blurShader_->setInt("horizontal", horizontal_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, first_iteration
                                     ? brightColorBuffer_
                                     : pingpongColorbuffers_[!horizontal_]);
    glBindVertexArray(quadVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    horizontal_ = !horizontal_;
    if (first_iteration)
      first_iteration = false;
  }

  /* ── スクリーンクワッド ── */
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClear(GL_COLOR_BUFFER_BIT);

  // [DEBUG] デプスマップを画面全体に表示して確認したい場合はここをアンコメント
  // debugDepthShader_->use();
  // glBindVertexArray(quadVAO_);
  // glActiveTexture(GL_TEXTURE0);
  // glBindTexture(GL_TEXTURE_2D, depthmapTexture_);
  // glDrawArrays(GL_TRIANGLES, 0, 6);

  screenshader_->use();
  screenshader_->setFloat("exposure", exposure_);
  screenshader_->setBool("debugRawOutput", debugRawOutput_);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, textureColorbuffer_);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(
      GL_TEXTURE_2D,
      pingpongColorbuffers_[!horizontal_]); // ブラー結果の最終バッファ

  glBindVertexArray(quadVAO_);
  glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Scene::applyPointLights(gl::Shader &shader) {
  for (const auto &pointLight : pointLights_)
    pointLight.applyToShader(
        shader, "pointLights[" +
                    std::to_string(&pointLight - pointLights_.data()) + "]");
}

void Scene::renderCubes(gl::Shader &shader) {
  cubeTexture_->bind(0);
  cubeNormalMap_->bind(1);
  cubeHeightMap_->bind(2);
  glBindVertexArray(cubeVAO_);
  shader.setMat4("model", glm::mat4(1.0f));
  glDrawElementsInstanced(GL_TRIANGLES, gl::cubeIndices.size(), GL_UNSIGNED_INT,
                          0, cube_pos_.size());
}

void Scene::renderFloor(gl::Shader &shader) {
  floorTexture_->bind(0);
  glBindVertexArray(planeVAO_);
  shader.setMat4("model", glm::mat4(1.0f));
  glDrawElements(GL_TRIANGLES, gl::planeIndices.size(), GL_UNSIGNED_INT, 0);
}

void Scene::renderLightCubes() {
  lightcubeShader_->use();
  glBindVertexArray(cubeVAO_);
  for (const auto &pointLight : pointLights_) {
    lightcubeShader_->setVec3("lightColor", pointLight.diffuse);
    glm::mat4 lightModel = glm::translate(glm::mat4(1.0f), pointLight.position);
    lightModel = glm::scale(lightModel, glm::vec3(0.2f));
    lightcubeShader_->setMat4("model", lightModel);
    glDrawElements(GL_TRIANGLES, gl::cubeIndices.size(), GL_UNSIGNED_INT, 0);
  }
}

void Scene::renderSkybox() {
  glDepthFunc(GL_LEQUAL);
  skyboxShader_->use();
  glBindVertexArray(skyboxVAO_);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture_);
  glDrawArrays(GL_TRIANGLES, 0, 36);
  glBindVertexArray(0);
  glDepthFunc(GL_LESS);
}

void Scene::renderTransparentWindows(
    const std::vector<gl::TransparentDraw> &sorted) {
  transparent_positions_.clear();
  for (const auto &draw : sorted)
    transparent_positions_.push_back(windows_pos_[draw.index]);

  glBindBuffer(GL_ARRAY_BUFFER, transparentInstanceVBO_);
  glBufferSubData(GL_ARRAY_BUFFER, 0,
                  sizeof(glm::vec3) * transparent_positions_.size(),
                  transparent_positions_.data());
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  transparentwindowShader_->use();
  transparentwindowShader_->setVec3("viewPos", camera_->GetViewPosition());
  transparentwindowShader_->setMat3("normalMatrix", glm::mat3(1.0f));
  transparentwindowShader_->setFloat("material.shininess", 32.0f);
  applyPointLights(*transparentwindowShader_);
  glBindVertexArray(transparentVAO_);
  transparentTexture_->bind(0);
  glDrawElementsInstanced(GL_TRIANGLES, gl::transparentIndices.size(),
                          GL_UNSIGNED_INT, 0, transparent_positions_.size());
}

void Scene::renderWalls(gl::Shader &shader) {
  brickwallTexture_->bind(0);
  brickwallNormalTexture_->bind(1);
  // shadowMap(白ライト用)はPass
  // 2の冒頭でユニット3にバインド済みなのでここでは何もしない

  glBindVertexArray(wallVAO_);
  shader.setMat4("model", glm::mat4(1.0f));
  glDrawElements(GL_TRIANGLES, gl::wallIndices.size(), GL_UNSIGNED_INT, 0);
}

Scene::~Scene() {
  glDeleteVertexArrays(1, &cubeVAO_);
  glDeleteVertexArrays(1, &planeVAO_);
  glDeleteVertexArrays(1, &transparentVAO_);
  glDeleteVertexArrays(1, &quadVAO_);
  glDeleteVertexArrays(1, &skyboxVAO_);
  glDeleteVertexArrays(1, &wallVAO_);
  glDeleteBuffers(1, &cubeVBO_);
  glDeleteBuffers(1, &cubeInstanceVBO_);
  glDeleteBuffers(1, &planeVBO_);
  glDeleteBuffers(1, &transparentVBO_);
  glDeleteBuffers(1, &transparentInstanceVBO_);
  glDeleteBuffers(1, &quadVBO_);
  glDeleteBuffers(1, &skyboxVBO_);
  glDeleteBuffers(1, &wallVBO_);
  glDeleteBuffers(1, &cubeEBO_);
  glDeleteBuffers(1, &planeEBO_);
  glDeleteBuffers(1, &transparentEBO_);
  glDeleteBuffers(1, &wallEBO_);
  glDeleteFramebuffers(1, &framebuffer_);
  glDeleteFramebuffers(4, depthMapFBO_);
  glDeleteTextures(1, &textureColorbuffer_);
  glDeleteTextures(1, &cubemapTexture_);
  glDeleteTextures(4, depthCubemap_);
  glDeleteRenderbuffers(1, &rbo_);
  glDeleteBuffers(1, &matricesUBO_);
  glDeleteTextures(1, &brightColorBuffer_);
  glDeleteFramebuffers(2, pingpongFBO_);
  glDeleteTextures(2, pingpongColorbuffers_);
  glDeleteFramebuffers(1, &gBuffer_);
  glDeleteTextures(1, &gPosition_);
  glDeleteTextures(1, &gNormal_);
  glDeleteTextures(1, &gAlbedoSpec_);
  glDeleteRenderbuffers(1, &gDepthRBO_);
  glDeleteFramebuffers(1, &ssaoFBO_);
  glDeleteFramebuffers(1, &ssaoBlurFBO_);
  glDeleteTextures(1, &ssaoColorBuffer_);
  glDeleteTextures(1, &ssaoColorBufferBlur_);
  glDeleteTextures(1, &noiseTexture_);
}
