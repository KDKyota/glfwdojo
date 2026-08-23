#include "Scene.h"
#include "Camera.h"
#include "GeometryData.h"
#include <cstddef>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <random>
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <stb_image.h>

Scene::Scene(std::shared_ptr<Camera> camera, int scrWidth, int scrHeight)
    : camera_(camera), scrWidth_(scrWidth), scrHeight_(scrHeight) {
    lightcubeShader_ = std::make_unique<gl::Shader>("light_cube.vert", "light_cube.frag");
    screenshader_ = std::make_unique<gl::Shader>("fragment_quad.vert", "hdr.frag");
    skyboxShader_ = std::make_unique<gl::Shader>("skybox.vert", "skybox.frag");
    transparentwindowShader_ = std::make_unique<gl::Shader>("window.vert", "glass.frag");
    pointDepthShader_ =
        std::make_unique<gl::Shader>("point_shadow_depth.vert", "point_shadow_depth.geom", "point_shadow_depth.frag");
    // vert / geom は深度パスと共用し、frag だけ差し替える
    pointColorShader_ =
        std::make_unique<gl::Shader>("point_shadow_depth.vert", "point_shadow_depth.geom", "point_shadow_color.frag");
    debugDepthShader_ = std::make_unique<gl::Shader>("fragment_quad.vert", "debug_depth.frag");
    blurShader_ = std::make_unique<gl::Shader>("fragment_quad.vert", "blur.frag");

    /* G-Buffer */
    gbufferFloorShader_ = std::make_unique<gl::Shader>("shader.vert", "gbuffer_floor.frag");
    gbufferCubeShader_ = std::make_unique<gl::Shader>("cube.vert", "gbuffer_cube.frag");
    gbufferModelShader_ = std::make_unique<gl::Shader>("gbuffer_model.vert", "gbuffer_model.frag");
    gbufferWallShader_ = std::make_unique<gl::Shader>("wall.vert", "gbuffer_wall.frag");
    gbufferWindowShader_ = std::make_unique<gl::Shader>("window.vert", "gbuffer_window.frag");
    deferredLightingShader_ = std::make_unique<gl::Shader>("fragment_quad.vert", "deferred_lighting.frag");

    /* SSAO */
    ssaoShader_ = std::make_unique<gl::Shader>("fragment_quad.vert", "ssao.frag");
    ssaoBlurShader_ = std::make_unique<gl::Shader>("fragment_quad.vert", "ssao_blur.frag");

    /* IBL */
    equirectToCubemapShader_ =
        std::make_unique<gl::Shader>("cubemap_capture.vert", "equirectangular_to_cubemap.frag");
    irradianceShader_ = std::make_unique<gl::Shader>("cubemap_capture.vert", "irradiance_convolution.frag");
    prefilterShader_ = std::make_unique<gl::Shader>("cubemap_capture.vert", "prefilter.frag");
    brdfLUTShader_ = std::make_unique<gl::Shader>("fragment_quad.vert", "brdf_lut.frag");

    // cubePositions_ = std::move(cubePositions);

    initMesh();
    initTextures();
    initModels();
    initFramebuffer();
    initUBO();
    initGBuffer();
    initSSAO();
    initIBL(); // skyboxVAO_ が必要なので初期化の最後
}

void Scene::initIBL() {
    /* --- 正距円筒図法の HDR を読み込む --- */
    stbi_set_flip_vertically_on_load(true);
    int width, height, nrComponents;
    float *data = stbi_loadf("resources/textures/skybox/studio_small_03_4k.hdr", &width, &height, &nrComponents, 0);
    if (!data) {
        std::cout << "ERROR::IBL:: Failed to load HDR environment map" << std::endl;
        return;
    }
    hdrTexture_.create();
    glBindTexture(GL_TEXTURE_2D, hdrTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(data);

    /* --- 6面ぶんの共通設定 --- */
    const glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    const glm::mat4 captureViews[6] = {
        glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))};

    captureFBO_.create();
    captureRBO_.create();
    /* 事前計算中だけ変える GL 状態 末尾のベース状態への復帰と対にすること */
    // 立方体の内側から見るので、通常のカリングでは面が消える
    glDisable(GL_CULL_FACE);
    // BRDF LUT は out vec2 でアルファが未定義。切らないと GL_SRC_ALPHA が 0 になり書き込みが消える
    glDisable(GL_BLEND);
    glBindVertexArray(skyboxVAO_);

    /* --- equirectangular -> cubemap --- */
    // こちらは描き込み先なので、3成分フォーマットを選んではいけない
    envCubemap_.create();
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap_);
    for (unsigned int i = 0; i < 6; ++i)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA16F, ENV_CUBEMAP_SIZE, ENV_CUBEMAP_SIZE, 0, GL_RGBA,
                     GL_FLOAT, nullptr);
    // prefilter がサンプルの粗密に応じてミップを引くので、ミップ付きにしておく
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO_);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, ENV_CUBEMAP_SIZE, ENV_CUBEMAP_SIZE);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO_);

    equirectToCubemapShader_->use();
    equirectToCubemapShader_->setInt("equirectangularMap", 0);
    equirectToCubemapShader_->setMat4("projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrTexture_);

    glViewport(0, 0, ENV_CUBEMAP_SIZE, ENV_CUBEMAP_SIZE);
    for (unsigned int i = 0; i < 6; ++i) {
        equirectToCubemapShader_->setMat4("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap_,
                               0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    // ミップの中身を埋める。prefilter がこれを引く
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap_);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    /* --- cubemap -> irradiance --- */
    irradianceMap_.create();
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap_);
    for (unsigned int i = 0; i < 6; ++i)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA16F, IRRADIANCE_SIZE, IRRADIANCE_SIZE, 0, GL_RGBA,
                     GL_FLOAT, nullptr);
    // 面の継ぎ目で色が飛ばないよう線形補間とエッジクランプにする
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, IRRADIANCE_SIZE, IRRADIANCE_SIZE);

    irradianceShader_->use();
    irradianceShader_->setInt("environmentMap", 0);
    irradianceShader_->setMat4("projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap_);

    glViewport(0, 0, IRRADIANCE_SIZE, IRRADIANCE_SIZE);
    for (unsigned int i = 0; i < 6; ++i) {
        irradianceShader_->setMat4("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                               irradianceMap_, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    /* --- cubemap -> prefilter（roughness ごとにミップへ焼く） --- */
    prefilterMap_.create();
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap_);
    for (unsigned int i = 0; i < 6; ++i)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA16F, PREFILTER_SIZE, PREFILTER_SIZE, 0, GL_RGBA,
                     GL_FLOAT, nullptr);
    // roughness の連続変化をミップ間の補間で表現するので TRILINEAR が必須
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP); // 各レベルの領域だけ確保させる

    prefilterShader_->use();
    prefilterShader_->setInt("environmentMap", 0);
    prefilterShader_->setMat4("projection", captureProjection);
    prefilterShader_->setFloat("envResolution", static_cast<float>(ENV_CUBEMAP_SIZE));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap_);

    for (unsigned int mip = 0; mip < PREFILTER_MIP_LEVELS; ++mip) {
        const unsigned int mipSize = PREFILTER_SIZE >> mip;
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO_);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipSize, mipSize);
        glViewport(0, 0, mipSize, mipSize);

        const float roughness = static_cast<float>(mip) / static_cast<float>(PREFILTER_MIP_LEVELS - 1);
        prefilterShader_->setFloat("roughness", roughness);
        for (unsigned int i = 0; i < 6; ++i) {
            prefilterShader_->setMat4("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                                   prefilterMap_, mip);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
    }
    glBindVertexArray(0);

    /* --- BRDF LUT（環境にも材質の色にも依存しない普遍的な表） --- */
    brdfLUT_.create();
    glBindTexture(GL_TEXTURE_2D, brdfLUT_);
    // 返すのはスケールとバイアスの2値なので2成分で足りる
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, BRDF_LUT_SIZE, BRDF_LUT_SIZE, 0, GL_RG, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, BRDF_LUT_SIZE, BRDF_LUT_SIZE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUT_, 0);
    glViewport(0, 0, BRDF_LUT_SIZE, BRDF_LUT_SIZE);
    brdfLUTShader_->use();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindVertexArray(quadVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::IBL:: Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, scrWidth_, scrHeight_);

    /* 冒頭で変えた GL 状態を戻す */
    glEnable(GL_BLEND);
    // カリングのベース状態は無効。ここで有効にすると床・壁・空が消える
}

void Scene::initMesh() {
    int stride = sizeof(gl::Vertex);

    /* cube */
    cubeVAO_.create();
    cubeVBO_.create();
    cubeInstanceVBO_.create();
    cubeEBO_.create();
    glBindVertexArray(cubeVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(gl::cubeVertices), gl::cubeVertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEBO_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(gl::cubeIndices), gl::cubeIndices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(gl::Vertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(gl::Vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(gl::Vertex, uv));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(gl::Vertex, tangent));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(gl::Vertex, bitangent));
    glBindBuffer(GL_ARRAY_BUFFER, cubeInstanceVBO_);
    glBufferData(GL_ARRAY_BUFFER, cube_pos_.size() * sizeof(glm::vec3), cube_pos_.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);
    glVertexAttribDivisor(5, 1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    /* 床 */
    planeVAO_.create();
    planeVBO_.create();
    planeEBO_.create();
    glBindVertexArray(planeVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(gl::planeVertices), gl::planeVertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, planeEBO_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(gl::planeIndices), gl::planeIndices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(gl::Vertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(gl::Vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(gl::Vertex, uv));
    glBindVertexArray(0);

    /* 透過窓 */
    transparentVAO_.create();
    transparentVBO_.create();
    transparentInstanceVBO_.create();
    transparentEBO_.create();
    glBindVertexArray(transparentVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, transparentVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(gl::transparentVertices), gl::transparentVertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, transparentEBO_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(gl::transparentIndices), gl::transparentIndices.data(),
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(gl::Vertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(gl::Vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(gl::Vertex, uv));
    glBindBuffer(GL_ARRAY_BUFFER, transparentInstanceVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * windows_pos_.size(), nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);
    glVertexAttribDivisor(5, 1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    /* skybox */
    skyboxVAO_.create();
    skyboxVBO_.create();
    glBindVertexArray(skyboxVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO_);
    glBufferData(GL_ARRAY_BUFFER, gl::skyboxVertices.size() * sizeof(glm::vec3), gl::skyboxVertices.data(),
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(gl::skyboxVertices[0]), (void *)0);
    glBindVertexArray(0);

    /* 壁 */
    // Normal Mapping のため location 0-4 をすべて使う
    wallVAO_.create();
    wallVBO_.create();
    wallEBO_.create();
    glBindVertexArray(wallVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, wallVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(gl::wallVertices), gl::wallVertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wallEBO_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(gl::wallIndices), gl::wallIndices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(gl::Vertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(gl::Vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(gl::Vertex, uv));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(gl::Vertex, tangent));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(gl::Vertex, bitangent));
    glBindVertexArray(0);

    /* screen quad */
    quadVAO_.create();
    quadVBO_.create();
    glBindVertexArray(quadVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(gl::quadVertices), gl::quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glBindVertexArray(0);
}

void Scene::initModels() {
    for (const ModelSpawn &spawn : modelSpawns_) {
        try {
            models_.push_back(std::make_unique<Model>(spawn.path, cache_));
        } catch (const std::exception &e) {
            std::cout << "Skipped model: " << spawn.path << " (" << e.what() << ")" << std::endl;
            continue;
        }
        const glm::mat4 translated = glm::translate(glm::mat4(1.0f), spawn.position);
        modelMatrices_.push_back(glm::scale(translated, glm::vec3(spawn.scale)));
        if (spawn.followTarget) {
            followTargetPosition_ = spawn.position;
            hasFollowTarget_ = true;
        }
    }
    // NOTE: 後で消す
    for (const std::unique_ptr<Model> &model : models_) {
        if (!model->HasBones()) {
            continue;
        }
        model->UpdateBonePalette();
        float maxDeviation = 0.0f;
        for (const glm::mat4 &m : model->BonePalette()) {
            for (int c = 0; c < 4; ++c) {
                for (int r = 0; r < 4; ++r) {
                    const float expected = (c == r) ? 1.0f : 0.0f;
                    maxDeviation = std::max(maxDeviation, std::abs(m[c][r] - expected));
                }
            }
        }
        std::cout << "palette deviation = " << maxDeviation << std::endl;
    }
}

void Scene::initTextures() {
    cubeTexture_ = cache_.get("resources/textures/bricks2.jpg", true, ColorSpace::SRGB);
    cubeNormalMap_ = cache_.get("resources/textures/bricks2_normal.jpg", true, ColorSpace::Linear);
    cubeHeightMap_ = cache_.get("resources/textures/bricks2_disp.jpg", true, ColorSpace::Linear);

    floorTexture_ = cache_.get("resources/textures/wood.png", true, ColorSpace::SRGB);
    transparentTexture_ = cache_.get("resources/textures/window.png", true, ColorSpace::SRGB);
    brickwallTexture_ = cache_.get("resources/textures/brickwall.jpg", true, ColorSpace::SRGB);
    brickwallNormalTexture_ = cache_.get("resources/textures/brickwall_normal.jpg", true, ColorSpace::Linear);

    /* cube */
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
    gbufferWindowShader_->use();
    gbufferWindowShader_->setInt("diffuseMap", 0);
    transparentwindowShader_->use();
    transparentwindowShader_->setInt("texture1", 0);
    for (unsigned int i = 0; i < 4; ++i)
        transparentwindowShader_->setInt("shadowMap[" + std::to_string(i) + "]", 3 + i);
    // deferredLightingShader_ と割り当てを揃える
    for (unsigned int i = 0; i < 4; ++i)
        transparentwindowShader_->setInt("shadowColor[" + std::to_string(i) + "]", 8 + i);
    transparentwindowShader_->setInt("prefilterMap", 13);
    transparentwindowShader_->setInt("brdfLUT", 14);
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
    pointDepthShader_->use();
    pointDepthShader_->setInt("diffuseMap", 0);
    pointColorShader_->use();
    pointColorShader_->setInt("diffuseMap", 0);
    // material_.setUniforms(*shader_);
    blurShader_->use();
    blurShader_->setInt("image", 0);
    /* deferred lighting */
    deferredLightingShader_->use();
    deferredLightingShader_->setInt("gPosition", 0);
    deferredLightingShader_->setInt("gNormal", 1);
    deferredLightingShader_->setInt("gAlbedoRoughness", 2);
    for (unsigned int i = 0; i < 4; ++i)
        deferredLightingShader_->setInt("shadowMap[" + std::to_string(i) + "]", 3 + i);
    deferredLightingShader_->setFloat("farPlane", shadowFarPlane_);
    // 既存の割り当て（0〜2=G-Buffer, 3〜6=shadowMap）を壊さないよう 7 を使う
    deferredLightingShader_->setInt("ssao", 7);
    for (unsigned int i = 0; i < 4; ++i)
        deferredLightingShader_->setInt("shadowColor[" + std::to_string(i) + "]", 8 + i);
    deferredLightingShader_->setInt("irradianceMap", 12);
    deferredLightingShader_->setInt("prefilterMap", 13);
    deferredLightingShader_->setInt("brdfLUT", 14);
    // ambientStrength は Render() 側、SSAO 系の uniform は initSSAO() 側で送る
}

void Scene::initFramebuffer() {
    /* framebuffer configuration */
    framebuffer_.create();
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    textureColorbuffer_.create(); // location=0 -> FragColor
    glBindTexture(GL_TEXTURE_2D, textureColorbuffer_);
    // RGB16F はカラーレンダリング可能が保証されない（initGBuffer のコメント参照）
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, scrWidth_, scrHeight_, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer_, 0);
    brightColorBuffer_.create(); // location=1 -> BrightColor
    glBindTexture(GL_TEXTURE_2D, brightColorBuffer_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, scrWidth_, scrHeight_, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, brightColorBuffer_, 0);
    unsigned int attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, attachments);

    /* 深度とステンシル。サンプリングしないのでレンダーバッファ */
    rbo_.create();
    glBindRenderbuffer(GL_RENDERBUFFER, rbo_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, scrWidth_, scrHeight_);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo_);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    /* ポイントシャドウ用キューブマップFBO。カラーを持たず深度だけを書く */
    for (unsigned int j = 0; j < 4; ++j)

    {
        depthMapFBO_[j].create();
        // 各テクセルに入るのは色ではなく、光源からの正規化距離 [0,1]
        depthCubemap_[j].create();
        shadowColorCubemap_[j].create();

        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap_[j]);
        for (unsigned int i = 0; i < 6; ++i)
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0,
                         GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        glBindTexture(GL_TEXTURE_CUBE_MAP, shadowColorCubemap_[j]);
        for (unsigned int i = 0; i < 6; ++i)
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA8, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_RGBA,
                         GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO_[j]);
        // glFramebufferTexture なら6面が1アタッチメントになり、gl_Layer で面を選べる
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap_[j], 0);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, shadowColorCubemap_[j], 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    /* ぼかし処理を書き込むFBO */
    // 縦方向と横方向にガウシアンブラーをかけるので二回のループ
    for (unsigned int i = 0; i < 2; i++) {
        pingpongFBO_[i].create();
        pingpongColorbuffers_[i].create();
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO_[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers_[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, scrWidth_, scrHeight_, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                        GL_CLAMP_TO_EDGE); // REPEAT だとブラーが反対側の値を拾う
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers_[i], 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Framebuffer not complete!" << std::endl;
    }
}

void Scene::initUBO() {
    matricesUBO_.create();
    glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO_);
    glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), nullptr, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, matricesUBO_);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void Scene::initGBuffer() {
    gBuffer_.create();
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer_);

    // GL_RGB16F は禁止 こうしないと GPU によっては書き込めない
    gPosition_.create();
    glBindTexture(GL_TEXTURE_2D, gPosition_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, scrWidth_, scrHeight_, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // REPEAT だと画面の反対側の値を拾い、画面端に不自然な遮蔽が出る
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition_, 0);
    // 法線は [-1,1] の負値を持つので浮動小数点フォーマット
    gNormal_.create();
    glBindTexture(GL_TEXTURE_2D, gNormal_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, scrWidth_, scrHeight_, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal_, 0);
    // アルベドも roughness も [0,1] なので 8bit で足りる
    gAlbedoRoughness_.create();
    glBindTexture(GL_TEXTURE_2D, gAlbedoRoughness_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, scrWidth_, scrHeight_, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedoRoughness_, 0);

    unsigned int attachments[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
    glDrawBuffers(3, attachments);

    gDepthRBO_.create();
    glBindRenderbuffer(GL_RENDERBUFFER, gDepthRBO_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, scrWidth_, scrHeight_);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, gDepthRBO_);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::G-BUFFER:: Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Scene::initSSAO() {
    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
    std::default_random_engine generator;

    /* --- サンプルカーネル --- */
    // 接空間（+Z が法線方向）における、半球内のサンプル点のテンプレート。
    ssaoKernel_.reserve(SSAO_KERNEL_SIZE);
    for (unsigned int i = 0; i < SSAO_KERNEL_SIZE; ++i) {
        glm::vec3 sample(randomFloats(generator) * 2.0f - 1.0f, randomFloats(generator) * 2.0f - 1.0f,
                         randomFloats(generator));
        sample = glm::normalize(sample);
        sample *= randomFloats(generator); // 半球の表面ではなく内部に散らす

        float scale = static_cast<float>(i) / static_cast<float>(SSAO_KERNEL_SIZE);
        scale = 0.1f + 0.9f * scale * scale; // 二次関数で原点寄りに偏らせる
        sample *= scale;

        ssaoKernel_.push_back(sample);
    }

    /* --- ノイズテクスチャ --- */
    std::vector<glm::vec3> ssaoNoise;
    ssaoNoise.reserve(16);
    for (unsigned int i = 0; i < 16; ++i) {
        // z = 0 にするのは「Z軸まわりの回転」にしたいから
        ssaoNoise.emplace_back(randomFloats(generator) * 2.0f - 1.0f, randomFloats(generator) * 2.0f - 1.0f, 0.0f);
    }
    noiseTexture_.create();
    glBindTexture(GL_TEXTURE_2D, noiseTexture_);
    // 負の値を保持する必要があるため浮動小数点フォーマット。
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 4, 4, 0, GL_RGB, GL_FLOAT, ssaoNoise.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // タイル状に敷き詰めるので GL_REPEAT が必須
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    /* --- SSAO パスの出力先 --- */
    // 遮蔽率はスカラー [0,1] なので 1チャンネル 8bit
    ssaoFBO_.create();
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO_);
    ssaoColorBuffer_.create();
    glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, scrWidth_, scrHeight_, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBuffer_, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::SSAO:: Framebuffer is not complete!" << std::endl;

    /* --- ブラーパスの出力先 --- */
    ssaoBlurFBO_.create();
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO_);
    ssaoColorBufferBlur_.create();
    glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, scrWidth_, scrHeight_, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBufferBlur_, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::SSAO_BLUR:: Framebuffer is not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    /* --- シェーダーの uniform 設定 --- */
    // initTextures() は initSSAO() より先に呼ばれるので、カーネル送信はここに置く
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
    heightScale_ = heightScale;

    // 透過窓の並び順は前方描画でも使うので、ここで受け取って持ち回る
    std::vector<gl::TransparentDraw> sorted;
    updateTransparentInstances(sorted);

    renderShadowPasses();         // [1] 光源視点の深度とガラスの透過色（4灯ぶん）
    updateMatricesUBO();          // [2] view / projection を UBO へ。以降の全パスが参照する
    renderGeometryPass();         // [3] 不透明物の幾何情報を G-Buffer へ
    renderSSAOPass();             // [4] G-Buffer から遮蔽率を求めてブラーまで
    blitGeometryDepth();          // [5] G-Buffer の深度を framebuffer_ へ複製（前方描画の深度テスト用）
    renderDeferredLightingPass(); // [6] G-Buffer + 影 + AO を合成
    renderForwardPass(sorted);    // [7] G-Buffer に入れられないもの（ライトキューブ・空・ガラス）
    renderBloomBlur();            // [8] 明るい部分をぼかして Bloom の素材を作る
    renderToScreen();             // [9] トーンマッピングとガンマ補正をしてデフォルトFBOへ
}

// 現在のガラスは乗算／加算ブレンドなので、この並べ替えは正しさには影響しない
void Scene::updateTransparentInstances(std::vector<gl::TransparentDraw> &sorted) {
    for (unsigned int i = 0; i < windows_pos_.size(); ++i)
        sorted.push_back({glm::length(camera_->GetViewPosition() - windows_pos_[i]), i});
    std::sort(sorted.begin(), sorted.end(),
              [](const gl::TransparentDraw &a, const gl::TransparentDraw &b) { return a.distance > b.distance; });
    transparent_positions_.clear();
    for (const auto &draw : sorted)
        transparent_positions_.push_back(windows_pos_[draw.index]);

    glBindBuffer(GL_ARRAY_BUFFER, transparentInstanceVBO_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3) * transparent_positions_.size(),
                    transparent_positions_.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
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

// 深度とガラスの透過色を同じ FBO へ、glDrawBuffer で書き込み先を切り替えて作る
void Scene::renderShadowPasses() {
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glEnable(GL_DEPTH_TEST);
    for (unsigned int j = 0; j < 4; ++j) {
        glm::vec3 lightPos = pointLights_[j].position;
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT,
                                                shadowNearPlane_, shadowFarPlane_);
        std::vector<glm::mat4> shadowTransforms;
        shadowTransforms.push_back(shadowProj *
                                   glm::lookAt(lightPos, lightPos + glm::vec3(1, 0, 0), glm::vec3(0, -1, 0)));
        shadowTransforms.push_back(shadowProj *
                                   glm::lookAt(lightPos, lightPos + glm::vec3(-1, 0, 0), glm::vec3(0, -1, 0)));
        shadowTransforms.push_back(shadowProj *
                                   glm::lookAt(lightPos, lightPos + glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)));
        shadowTransforms.push_back(shadowProj *
                                   glm::lookAt(lightPos, lightPos + glm::vec3(0, -1, 0), glm::vec3(0, 0, -1)));
        shadowTransforms.push_back(shadowProj *
                                   glm::lookAt(lightPos, lightPos + glm::vec3(0, 0, 1), glm::vec3(0, -1, 0)));
        shadowTransforms.push_back(shadowProj *
                                   glm::lookAt(lightPos, lightPos + glm::vec3(0, 0, -1), glm::vec3(0, -1, 0)));

        /* ── Pass 1: Point Shadow Depth Pass ── */
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO_[j]);
        // カラーサブパスと交互に使うので、ループ内で毎回 use() しないと uniform の送り先がずれる
        pointDepthShader_->use();
        glDrawBuffer(GL_NONE);
        glClear(GL_DEPTH_BUFFER_BIT);
        for (int i = 0; i < 6; ++i)
            pointDepthShader_->setMat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
        pointDepthShader_->setFloat("farPlane", shadowFarPlane_);
        pointDepthShader_->setVec3("lightPos", lightPos);
        pointDepthShader_->setBool("useAlphaTest", false);
        renderFloor(*pointDepthShader_);
        renderCubes(*pointDepthShader_);
        renderWalls(*pointDepthShader_);
        renderModels(*pointDepthShader_);
        pointDepthShader_->setBool("useAlphaTest", true);
        renderWindow(*pointDepthShader_);

        /* ── Pass 1.5: Point Shadow Color Pass ── */
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // ガラスを通らない方向 = 減衰なし
        glClear(GL_COLOR_BUFFER_BIT);
        // 深度テストは残したまま書き込みだけ止め、不透明物より奥のガラスを弾く
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ZERO, GL_SRC_COLOR);
        pointColorShader_->use();
        for (int i = 0; i < 6; ++i)
            pointColorShader_->setMat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
        pointColorShader_->setFloat("farPlane", shadowFarPlane_);
        pointColorShader_->setVec3("lightPos", lightPos);
        renderWindow(*pointColorShader_);
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
        glDrawBuffer(GL_NONE);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Scene::renderGeometryPass() {
    glViewport(0, 0, scrWidth_, scrHeight_); // シャドウ用に変えた viewport を元に戻す
    glEnable(GL_DEPTH_TEST);
    // ブレンドが有効なままだと、アルファ未定義の出力は書き込みが丸ごと消える
    glDisable(GL_BLEND);

    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer_);
    // 非ゼロだと ssao.frag の「法線がゼロなら背景」判定をすり抜ける
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* cube */
    // POM の穴から裏面が透けるのを防ぐ。片面ポリゴンは消えるので cube の間だけ
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    gbufferCubeShader_->use();
    gbufferCubeShader_->setVec3("viewPos", camera_->GetViewPosition());
    gbufferCubeShader_->setMat3("normalMatrix", glm::mat3(1.0f));
    gbufferCubeShader_->setFloat("heightScale", heightScale_);
    cubeMaterial_.applyToShader(*gbufferCubeShader_);
    renderCubes(*gbufferCubeShader_);
    glDisable(GL_CULL_FACE);

    /* floor */
    gbufferFloorShader_->use();
    gbufferFloorShader_->setMat3("normalMatrix", glm::mat3(1.0f));
    floorMaterial_.applyToShader(*gbufferFloorShader_);
    renderFloor(*gbufferFloorShader_);

    /* wall */
    gbufferWallShader_->use();
    wallMaterial_.applyToShader(*gbufferWallShader_);
    renderWalls(*gbufferWallShader_);

    /* model */
    gbufferModelShader_->use();
    renderModels(*gbufferModelShader_);

    /* Transparent window's fisical frame */
    gbufferWindowShader_->use();
    windowMaterial_.applyToShader(*gbufferWindowShader_);
    renderWindow(*gbufferWindowShader_);
}

void Scene::renderSSAOPass() {
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
    // 4x4 のノイズをタイル状に敷いた代償の格子模様を、同じ 4x4 の平均で打ち消す
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO_);
    glClear(GL_COLOR_BUFFER_BIT);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer_);
    ssaoBlurShader_->use();
    glBindVertexArray(quadVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// これがないと後続の前方描画が不透明物と前後判定できない
void Scene::blitGeometryDepth() {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer_);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer_);
    glBlitFramebuffer(0, 0, scrWidth_, scrHeight_, 0, 0, scrWidth_, scrHeight_, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Scene::renderDeferredLightingPass() {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    glClear(GL_COLOR_BUFFER_BIT); // 深度は blitGeometryDepth() でコピー済み
    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gPosition_);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gNormal_);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gAlbedoRoughness_);
    for (unsigned int j = 0; j < 4; ++j) {
        glActiveTexture(GL_TEXTURE3 + j);
        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap_[j]);
    }
    // ブラー後のAOをユニット7へ（3〜6は shadowMap が使っている）
    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur_);
    for (unsigned int j = 0; j < 4; ++j) {
        glActiveTexture(GL_TEXTURE8 + j);
        glBindTexture(GL_TEXTURE_CUBE_MAP, shadowColorCubemap_[j]);
    }
    glActiveTexture(GL_TEXTURE12);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap_);
    glActiveTexture(GL_TEXTURE13);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap_);
    glActiveTexture(GL_TEXTURE14);
    glBindTexture(GL_TEXTURE_2D, brdfLUT_);
    deferredLightingShader_->use();
    deferredLightingShader_->setVec3("viewPos", camera_->GetViewPosition());
    // UI から変わる値なので毎フレーム送る
    deferredLightingShader_->setInt("debugMode", debugMode_);
    deferredLightingShader_->setFloat("ssaoStrength", ssaoStrength_);
    deferredLightingShader_->setFloat("ambientStrength", ambientStrength_);
    applyPointLights(*deferredLightingShader_);
    glBindVertexArray(quadVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glEnable(GL_DEPTH_TEST);
}

// G-Buffer に載せられないものだけを描く
void Scene::renderForwardPass(const std::vector<gl::TransparentDraw> &sorted) {
    glViewport(0, 0, scrWidth_, scrHeight_);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    glEnable(GL_DEPTH_TEST);

    renderLightCubes();
    renderSkybox();

    /* 透過窓（ブレンドが必要なのはここだけ。Geometryパスの冒頭で無効化しているので、描画中だけ有効にする）*/
    glEnable(GL_BLEND);
    renderTransparentWindows(sorted);
    glDisable(GL_BLEND);
}

void Scene::renderBloomBlur() {
    glDisable(GL_DEPTH_TEST);
    bool first_iteration = true;
    unsigned int amount = 10;
    blurShader_->use();
    for (unsigned int i = 0; i < amount; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO_[horizontal_]);
        blurShader_->setInt("horizontal", horizontal_);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D,
                      first_iteration ? brightColorBuffer_.get() : pingpongColorbuffers_[!horizontal_].get());
        glBindVertexArray(quadVAO_);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        horizontal_ = !horizontal_;
        if (first_iteration)
            first_iteration = false;
    }
}

void Scene::renderToScreen() {
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
    screenshader_->setFloat("bloomStrength", bloomStrength_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureColorbuffer_);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D,
                  pingpongColorbuffers_[!horizontal_]);

    glBindVertexArray(quadVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Scene::applyPointLights(gl::Shader &shader) {
    for (const auto &pointLight : pointLights_)
        pointLight.applyToShader(shader, "pointLights[" + std::to_string(&pointLight - pointLights_.data()) + "]");
}

void Scene::renderCubes(gl::Shader &shader) {
    cubeTexture_->bind(0);
    cubeNormalMap_->bind(1);
    cubeHeightMap_->bind(2);
    glBindVertexArray(cubeVAO_);
    shader.setMat4("model", glm::mat4(1.0f));
    glDrawElementsInstanced(GL_TRIANGLES, gl::cubeIndices.size(), GL_UNSIGNED_INT, 0, cube_pos_.size());
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
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap_);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS);
}

void Scene::renderTransparentWindows(const std::vector<gl::TransparentDraw> &sorted) {
    transparentwindowShader_->use();
    transparentwindowShader_->setVec3("viewPos", camera_->GetViewPosition());
    transparentwindowShader_->setMat3("normalMatrix", glm::mat3(1.0f));
    glassMaterial_.applyToShader(*transparentwindowShader_);
    for (unsigned int j = 0; j < 4; ++j) {
        glActiveTexture(GL_TEXTURE8 + j);
        glBindTexture(GL_TEXTURE_CUBE_MAP, shadowColorCubemap_[j]);
    }
    glActiveTexture(GL_TEXTURE13);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap_);
    glActiveTexture(GL_TEXTURE14);
    glBindTexture(GL_TEXTURE_2D, brdfLUT_);
    applyPointLights(*transparentwindowShader_);

    glDepthMask(GL_FALSE);
    glBlendFunc(GL_ZERO, GL_SRC_COLOR);
    transparentwindowShader_->setBool("reflectionPass", false);
    renderWindow(*transparentwindowShader_);

    glBlendFunc(GL_ONE, GL_ONE);
    transparentwindowShader_->setBool("reflectionPass", true);
    renderWindow(*transparentwindowShader_);
    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Scene::renderWindow(gl::Shader &shader) {
    shader.setMat4("model", glm::mat4(1.0f));
    shader.setMat3("normalMatrix", glm::mat3(1.0f));

    glBindVertexArray(transparentVAO_);
    transparentTexture_->bind(0);
    glDrawElementsInstanced(GL_TRIANGLES, gl::transparentIndices.size(), GL_UNSIGNED_INT, 0,
                            transparent_positions_.size());
}

void Scene::renderModels(gl::Shader &shader) {
    for (size_t i = 0; i < models_.size(); ++i)
        models_[i]->Draw(shader, modelMatrices_[i]);
}

void Scene::renderWalls(gl::Shader &shader) {
    brickwallTexture_->bind(0);
    brickwallNormalTexture_->bind(1);

    glBindVertexArray(wallVAO_);
    shader.setMat4("model", glm::mat4(1.0f));
    glDrawElements(GL_TRIANGLES, gl::wallIndices.size(), GL_UNSIGNED_INT, 0);
}
