#include "Scene.h"
#include "Camera.h"
#include "GeometryData.h"
#include <cstddef>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <map>
#include <random>

Scene::Scene(std::shared_ptr<Camera> camera, int scrWidth, int scrHeight)
    : camera_(camera), scrWidth_(scrWidth), scrHeight_(scrHeight)
{
    shader_ = std::make_unique<gl::Shader>("shader.vert", "shader.frag");
    // cubeShader_ = std::make_unique<gl::Shader>("cube.vert", "cube.frag");
    lightcubeShader_ = std::make_unique<gl::Shader>("light_cube.vert", "light_cube.frag");
    // shaderSingleColor_ = std::make_unique<gl::Shader>("shader.vert",
    // "stencil_single_color.frag"); glasscubeShader_ =
    // std::make_unique<gl::Shader>("glasscube.vert", "glasscube.frag"); model_ =
    // std::make_unique<Model>("resources\\objects\\backpack\\backpack.obj",
    // cache_);
    screenshader_ = std::make_unique<gl::Shader>("fragment_quad.vert", "hdr.frag");
    skyboxShader_ = std::make_unique<gl::Shader>("skybox.vert", "skybox.frag");
    transparentwindowShader_ = std::make_unique<gl::Shader>("window.vert", "glass.frag");
    pointDepthShader_ =
        std::make_unique<gl::Shader>("point_shadow_depth.vert", "point_shadow_depth.geom", "point_shadow_depth.frag");
    // vert / geom は深度パスと共用し、frag だけ差し替える
    pointColorShader_ =
        std::make_unique<gl::Shader>("point_shadow_depth.vert", "point_shadow_depth.geom", "point_shadow_color.frag");
    debugDepthShader_ = std::make_unique<gl::Shader>("fragment_quad.vert", "debug_depth.frag");
    // wallShader_ = std::make_unique<gl::Shader>("wall.vert", "wall.frag");
    blurShader_ = std::make_unique<gl::Shader>("fragment_quad.vert", "blur.frag");

    /* G-Buffer */
    gbufferFloorShader_ = std::make_unique<gl::Shader>("shader.vert", "gbuffer_floor.frag");
    gbufferCubeShader_ = std::make_unique<gl::Shader>("cube.vert", "gbuffer_cube.frag");
    gbufferWallShader_ = std::make_unique<gl::Shader>("wall.vert", "gbuffer_wall.frag");
    gbufferWindowShader_ = std::make_unique<gl::Shader>("window.vert", "gbuffer_window.frag");
    deferredLightingShader_ = std::make_unique<gl::Shader>("fragment_quad.vert", "deferred_lighting.frag");

    /* SSAO */
    ssaoShader_ = std::make_unique<gl::Shader>("fragment_quad.vert", "ssao.frag");
    ssaoBlurShader_ = std::make_unique<gl::Shader>("fragment_quad.vert", "ssao_blur.frag");

    // cubePositions_ = std::move(cubePositions);

    initMesh();
    initTextures();
    initFramebuffer();
    initUBO();
    initGBuffer();
    initSSAO();
}

void Scene::initMesh()
{
    int stride = sizeof(gl::Vertex);

    cubeVAO_.create(); // cube用のVAO
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
    glVertexAttribDivisor(5, 1); // インスタンスごとに変化する属性
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    planeVAO_.create(); // 床用のVAO
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

    transparentVAO_.create(); // 透過窓のVAO
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
    glVertexAttribDivisor(5, 1); // インスタンスごとに変化する属性
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // skybox
    skyboxVAO_.create(); // スカイボックスのVAO
    skyboxVBO_.create();
    glBindVertexArray(skyboxVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO_);
    glBufferData(GL_ARRAY_BUFFER, gl::skyboxVertices.size() * sizeof(glm::vec3), gl::skyboxVertices.data(),
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(gl::skyboxVertices[0]), (void *)0);
    glBindVertexArray(0);

    // 壁 (Normal Mapping 用: location 0-4 を使用)
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

    // screen quad
    quadVAO_.create(); // スクリーンテクスチャのVAO
    quadVBO_.create();
    glBindVertexArray(quadVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(gl::quadVertices), gl::quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
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

void Scene::initTextures()
{
    // material_.diffuse = cache_.get("resources\\textures\\container2.png",
    // false); material_.specular =
    // cache_.get("resources\\textures\\container2_specular.png", true);
    cubeTexture_ = cache_.get("resources/textures/bricks2.jpg", true, ColorSpace::SRGB);
    cubeNormalMap_ = cache_.get("resources/textures/bricks2_normal.jpg", true, ColorSpace::Linear);
    cubeHeightMap_ = cache_.get("resources/textures/bricks2_disp.jpg", true, ColorSpace::Linear);

    floorTexture_ = cache_.get("resources/textures/wood.png", true, ColorSpace::SRGB);
    transparentTexture_ = cache_.get("resources/textures/window.png", true, ColorSpace::SRGB);
    std::vector<std::string> faces{"resources/textures/skybox/right.jpg", "resources/textures/skybox/left.jpg",
                                   "resources/textures/skybox/top.jpg",   "resources/textures/skybox/bottom.jpg",
                                   "resources/textures/skybox/front.jpg", "resources/textures/skybox/back.jpg"};
    cubemapTexture_.reset(cache_.loadCubemap(faces, false, ColorSpace::SRGB));

    brickwallTexture_ = cache_.get("resources/textures/brickwall.jpg", true, ColorSpace::SRGB);
    brickwallNormalTexture_ = cache_.get("resources/textures/brickwall_normal.jpg", true, ColorSpace::Linear);

    /* cube */
    // use G-Buffer in cube, floor, wall, transparent window
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
    transparentwindowShader_->setInt("ssao", 7);
    // 0=texture1, 3〜6=shadowMap, 7=ssao が埋まっているので 8〜11
    for (unsigned int i = 0; i < 4; ++i)
        transparentwindowShader_->setInt("shadowColor[" + std::to_string(i) + "]", 8 + i);
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
    deferredLightingShader_->setInt("gAlbedoSpec", 2);
    for (unsigned int i = 0; i < 4; ++i)
        deferredLightingShader_->setInt("shadowMap[" + std::to_string(i) + "]", 3 + i);
    deferredLightingShader_->setFloat("farPlane", shadowFarPlane_);
    // 既存の割り当て（0〜2=G-Buffer, 3〜6=shadowMap）を壊さないよう 7 を使う
    deferredLightingShader_->setInt("ssao", 7);
    for (unsigned int i = 0; i < 4; ++i)
        deferredLightingShader_->setInt("shadowColor[" + std::to_string(i) + "]", 8 + i);
    // ambientStrength は UI から変更するので Render() 側で毎フレーム送る
    // ssaoShader_ / ssaoBlurShader_ の uniform は initSSAO() 側で設定する。
}

void Scene::initFramebuffer()
{
    /* framebuffer configuration */
    framebuffer_.create(); // フレームバッファ（通常は *FBO とかに命名するけど…）
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    // create a color attachment texture（通常のカラーバッファをアタッチメント
    // location=0 -> FragColor）
    textureColorbuffer_.create(); // 最終的に画面に貼り付けるカラーバッファ
    glBindTexture(GL_TEXTURE_2D, textureColorbuffer_);
    // RGB16F はカラーレンダリング可能が保証されていないフォーマットなので RGBA16F
    // を使う（initGBuffer のコメント参照）
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, scrWidth_, scrHeight_, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer_, 0);
    // 明るい部分のカラーバッファをアタッチメント（location = 1 -> BrightColor）
    brightColorBuffer_.create();
    glBindTexture(GL_TEXTURE_2D, brightColorBuffer_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, scrWidth_, scrHeight_, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, brightColorBuffer_, 0);
    unsigned int attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, attachments);

    /* create a renderbuffer object for depth and stencil attachment(we won't be
     * sampling these) */
    rbo_.create(); // デプスやステンシルの処理を行う
    glBindRenderbuffer(GL_RENDERBUFFER, rbo_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, scrWidth_,
                          scrHeight_); // use a single renderbuffer object for
                                       // both a depth AND stencil buffer.
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                              rbo_); // now actually attach it
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // フレームバッファの初期化処理

    /* ポイントシャドウ用キューブマップFBO */
    // depthMapFBO_: カラーバッファを持たず、深度だけを depthCubemap_
    // に書き込む専用FBO
    for (unsigned int j = 0; j < 4; ++j)

    {
        depthMapFBO_[j].create();
        // depthCubemap_: 6面ぶんの深度テクスチャ。各テクセルには
        // point_shadow_depth.frag が書き込む 「光源からの正規化距離
        // [0,1]」が入る（通常のcubemapのような色情報ではない点に注意）
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
        // depthCubemap_ をFBOの深度アタッチメントに設定。glFramebufferTexture
        // を使うことで
        // 6面すべてが1つのアタッチメントとして扱われ、geometry
        // shaderのgl_Layerで面を選択できるようになる
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap_[j], 0);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, shadowColorCubemap_[j], 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    /* ぼかし処理を書き込むFBO */
    // 縦方向と横方向にガウシアンブラーをかけるのでそのために二回のループ
    for (unsigned int i = 0; i < 2; i++)
    {
        pingpongFBO_[i].create();
        pingpongColorbuffers_[i].create();
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO_[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers_[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, scrWidth_, scrHeight_, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                        GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would
                                           // otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers_[i], 0);
        // also check if framebuffers are complete (no need for depth buffer)
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Framebuffer not complete!" << std::endl;
    }
}

void Scene::initUBO()
{
    matricesUBO_.create();
    glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO_);
    glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), nullptr, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, matricesUBO_);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void Scene::initGBuffer()
{
    // gBuffer_にバインド
    gBuffer_.create();
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer_);

    // 内部フォーマットに GL_RGB16F を使ってはいけない。
    // OpenGL の必須フォーマット表では RGB16F
    // は「テクスチャとしては必須／カラーレンダリング可能は非必須」に
    // 分類されており、環境によってはテクスチャ作成も glCheckFramebufferStatus
    // も通るのに 書き込みだけが正しく行われない。3成分しか使わなくても RGBA16F
    // を使うこと。
    // gPosition（ワールド座標。負値や1を超える値を持つため浮動小数点フォーマットが必要）
    gPosition_.create();
    glBindTexture(GL_TEXTURE_2D, gPosition_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, scrWidth_, scrHeight_, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // デフォルトの GL_REPEAT
    // のままだと画面の反対側の値を拾い、画面端に不自然な遮蔽が出る。
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition_, 0);
    // gNormal（法線。[-1,1]
    // の負値を保持する必要があるため浮動小数点フォーマット）
    gNormal_.create();
    glBindTexture(GL_TEXTURE_2D, gNormal_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, scrWidth_, scrHeight_, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal_, 0);
    // gAlbedoSpec（rgb=アルベド, a=スペキュラ強度。いずれも
    // [0,1]なので8bitで十分）
    gAlbedoSpec_.create();
    glBindTexture(GL_TEXTURE_2D, gAlbedoSpec_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, scrWidth_, scrHeight_, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedoSpec_, 0);

    unsigned int attachments[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
    glDrawBuffers(3, attachments);

    // --- Depth Buffer ---
    gDepthRBO_.create();
    glBindRenderbuffer(GL_RENDERBUFFER, gDepthRBO_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, scrWidth_, scrHeight_);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, gDepthRBO_);
    // エラーチェック
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::G-BUFFER:: Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // フレームバッファをデフォルトに戻す
}

void Scene::initSSAO()
{
    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
    std::default_random_engine generator;

    /* --- サンプルカーネル --- */
    // 接空間（+Z が法線方向）における、半球内のサンプル点のテンプレート。
    ssaoKernel_.reserve(SSAO_KERNEL_SIZE);
    for (unsigned int i = 0; i < SSAO_KERNEL_SIZE; ++i)
    {
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
    for (unsigned int i = 0; i < 16; ++i)
    {
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
    // initTextures() ではなくここで設定するのは、カーネルを送るのに
    // 上で生成した ssaoKernel_ が必要なため（initTextures() は initSSAO()
    // より先に呼ばれる）。
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

void Scene::Render(float deltaTime, float heightScale)
{
    elapsedTime_ += deltaTime;
    heightScale_ = heightScale;

    // 透過窓の並び順は前方描画でも使うので、ここで受け取って持ち回る
    std::vector<gl::TransparentDraw> sorted;
    updateTransparentInstances(sorted);

    renderShadowPasses();          // [1] 光源視点の深度とガラスの透過色（4灯ぶん）
    updateMatricesUBO();           // [2] view / projection を UBO へ。以降の全パスが参照する
    renderGeometryPass();          // [3] 不透明物の幾何情報を G-Buffer へ
    renderSSAOPass();              // [4] G-Buffer から遮蔽率を求めてブラーまで
    blitGeometryDepth();           // [5] G-Buffer の深度を framebuffer_ へ複製（前方描画の深度テスト用）
    renderDeferredLightingPass();  // [6] G-Buffer + 影 + AO を合成
    renderForwardPass(sorted);     // [7] G-Buffer に入れられないもの（ライトキューブ・空・ガラス）
    renderBloomBlur();             // [8] 明るい部分をぼかして Bloom の素材を作る
    renderToScreen();              // [9] トーンマッピングとガンマ補正をしてデフォルトFBOへ
}

// 現在のガラスは乗算／加算ブレンドなので、この並べ替えは正しさには影響しない
void Scene::updateTransparentInstances(std::vector<gl::TransparentDraw> &sorted)
{
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

void Scene::updateMatricesUBO()
{
    glm::mat4 view = camera_->GetViewMatrix();
    glm::mat4 projection =
        glm::perspective(glm::radians(camera_->GetZoomValue()), (float)scrWidth_ / (float)scrHeight_, 0.1f, 100.0f);
    glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO_);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(view));
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(projection));
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

// 深度とガラスの透過色を同じ FBO へ、glDrawBuffer で書き込み先を切り替えて作る
void Scene::renderShadowPasses()
{
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glEnable(GL_DEPTH_TEST);
    for (unsigned int j = 0; j < 4; ++j)
    {
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
        // shadowTransforms[6] を geometry shader の uniform 配列 shadowMatrices[6]
        // に渡す
        for (int i = 0; i < 6; ++i)
            pointDepthShader_->setMat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
        pointDepthShader_->setFloat("farPlane", shadowFarPlane_);
        pointDepthShader_->setVec3("lightPos", lightPos);
        pointDepthShader_->setBool("useAlphaTest", false);
        renderFloor(*pointDepthShader_);
        renderCubes(*pointDepthShader_);
        renderWalls(*pointDepthShader_);
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

void Scene::renderGeometryPass()
{
    glViewport(0, 0, scrWidth_, scrHeight_); // シャドウ用に変えた viewport を元に戻す
    glEnable(GL_DEPTH_TEST);
    // G-Buffer
    // に入るのは「色」ではなく座標・法線という幾何情報なので、絶対にブレンドしてはいけない。
    // Window.cpp で透過窓のために glEnable(GL_BLEND) されたままだと、
    // gPosition / gNormal は out vec3（アルファ成分が未定義＝実質0）なので
    // src.rgb * 0 + dst.rgb * 1 となって書き込みが丸ごと消え、G-Buffer
    // がクリア値のままになる。
    // 著者はここをミスっちゃった（バグ解消に位置に近かった）
    glDisable(GL_BLEND);

    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer_);
    // G-Buffer は色ではなくデータなので必ずゼロクリアする。
    // 非ゼロだと ssao.frag の「法線がゼロなら背景」判定をすり抜ける
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
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

    /* Transparent window's fisical frame */
    gbufferWindowShader_->use();
    renderWindow(*gbufferWindowShader_);
}

void Scene::renderSSAOPass()
{
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
    // 4x4 のノイズをタイル状に敷いた代償として出る格子模様を、4x4
    // の平均で打ち消す
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
void Scene::blitGeometryDepth()
{
    glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer_);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer_);
    glBlitFramebuffer(0, 0, scrWidth_, scrHeight_, 0, 0, scrWidth_, scrHeight_, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Scene::renderDeferredLightingPass()
{
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    glClear(GL_COLOR_BUFFER_BIT); // 深度は blitGeometryDepth() でコピー済みなのでクリアしない
    glDisable(GL_DEPTH_TEST);     // フルスクリーンクワッドなので深度テスト不要
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gPosition_);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gNormal_);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gAlbedoSpec_);
    for (unsigned int j = 0; j < 4; ++j)
    {
        glActiveTexture(GL_TEXTURE3 + j);
        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap_[j]);
    }
    // ブラー後のAOをユニット7へ（3〜6は shadowMap が使っている）
    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur_);
    for (unsigned int j = 0; j < 4; ++j)
    {
        glActiveTexture(GL_TEXTURE8 + j);
        glBindTexture(GL_TEXTURE_CUBE_MAP, shadowColorCubemap_[j]);
    }
    deferredLightingShader_->use();
    deferredLightingShader_->setVec3("viewPos", camera_->GetViewPosition());
    // UI から変更される設定は毎フレーム送る。
    // initTextures() で一度だけ送ると、値を変えても反映されない。
    deferredLightingShader_->setInt("debugMode", debugMode_);
    deferredLightingShader_->setFloat("ssaoStrength", ssaoStrength_);
    deferredLightingShader_->setFloat("ambientStrength", ambientStrength_);
    applyPointLights(*deferredLightingShader_);
    glBindVertexArray(quadVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glEnable(GL_DEPTH_TEST);
}

// G-Buffer に載せられないものだけを描く
void Scene::renderForwardPass(const std::vector<gl::TransparentDraw> &sorted)
{
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

void Scene::renderBloomBlur()
{
    glDisable(GL_DEPTH_TEST);
    bool first_iteration = true;
    unsigned int amount = 10;
    blurShader_->use();
    for (unsigned int i = 0; i < amount; ++i)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO_[horizontal_]);
        blurShader_->setInt("horizontal", horizontal_);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, first_iteration ? brightColorBuffer_.get() : pingpongColorbuffers_[!horizontal_].get());
        glBindVertexArray(quadVAO_);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        horizontal_ = !horizontal_;
        if (first_iteration)
            first_iteration = false;
    }
}

void Scene::renderToScreen()
{
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
                  pingpongColorbuffers_[!horizontal_]); // ブラー結果の最終バッファ

    glBindVertexArray(quadVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Scene::applyPointLights(gl::Shader &shader)
{
    for (const auto &pointLight : pointLights_)
        pointLight.applyToShader(shader, "pointLights[" + std::to_string(&pointLight - pointLights_.data()) + "]");
}

void Scene::renderCubes(gl::Shader &shader)
{
    cubeTexture_->bind(0);
    cubeNormalMap_->bind(1);
    cubeHeightMap_->bind(2);
    glBindVertexArray(cubeVAO_);
    shader.setMat4("model", glm::mat4(1.0f));
    glDrawElementsInstanced(GL_TRIANGLES, gl::cubeIndices.size(), GL_UNSIGNED_INT, 0, cube_pos_.size());
}

void Scene::renderFloor(gl::Shader &shader)
{
    floorTexture_->bind(0);
    glBindVertexArray(planeVAO_);
    shader.setMat4("model", glm::mat4(1.0f));
    glDrawElements(GL_TRIANGLES, gl::planeIndices.size(), GL_UNSIGNED_INT, 0);
}

void Scene::renderLightCubes()
{
    lightcubeShader_->use();
    glBindVertexArray(cubeVAO_);
    for (const auto &pointLight : pointLights_)
    {
        lightcubeShader_->setVec3("lightColor", pointLight.diffuse);
        glm::mat4 lightModel = glm::translate(glm::mat4(1.0f), pointLight.position);
        lightModel = glm::scale(lightModel, glm::vec3(0.2f));
        lightcubeShader_->setMat4("model", lightModel);
        glDrawElements(GL_TRIANGLES, gl::cubeIndices.size(), GL_UNSIGNED_INT, 0);
    }
}

void Scene::renderSkybox()
{
    glDepthFunc(GL_LEQUAL);
    skyboxShader_->use();
    glBindVertexArray(skyboxVAO_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture_);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS);
}

void Scene::renderTransparentWindows(const std::vector<gl::TransparentDraw> &sorted)
{
    transparentwindowShader_->use();
    transparentwindowShader_->setVec3("viewPos", camera_->GetViewPosition());
    transparentwindowShader_->setMat3("normalMatrix", glm::mat3(1.0f));
    transparentwindowShader_->setFloat("material.shininess", 128.0f);
    // 不透明面（Deferred）と環境光の扱いを揃える。UI
    // から変更するので毎フレーム送る
    transparentwindowShader_->setFloat("ambientStrength", ambientStrength_);
    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur_);
    for (unsigned int j = 0; j < 4; ++j)
    {
        glActiveTexture(GL_TEXTURE8 + j);
        glBindTexture(GL_TEXTURE_CUBE_MAP, shadowColorCubemap_[j]);
    }
    applyPointLights(*transparentwindowShader_);

    glDepthMask(GL_FALSE);
    glBlendFunc(GL_ZERO, GL_SRC_COLOR);
    transparentwindowShader_->setBool("reflectionPass", false);
    renderWindow(*transparentwindowShader_);

    glBlendFunc(GL_ONE, GL_ONE);
    transparentwindowShader_->setBool("reflectionPass", true);
    renderWindow(*transparentwindowShader_);
    // 実害はないかもしれないが，ブレンドを元に戻す
    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Scene::renderWindow(gl::Shader &shader)
{
    shader.setMat4("model", glm::mat4(1.0f));
    shader.setMat3("normalMatrix", glm::mat3(1.0f));

    glBindVertexArray(transparentVAO_);
    transparentTexture_->bind(0);
    glDrawElementsInstanced(GL_TRIANGLES, gl::transparentIndices.size(), GL_UNSIGNED_INT, 0,
                            transparent_positions_.size());
}

void Scene::renderWalls(gl::Shader &shader)
{
    brickwallTexture_->bind(0);
    brickwallNormalTexture_->bind(1);
    // shadowMap(白ライト用)はPass
    // 2の冒頭でユニット3にバインド済みなのでここでは何もしない

    glBindVertexArray(wallVAO_);
    shader.setMat4("model", glm::mat4(1.0f));
    glDrawElements(GL_TRIANGLES, gl::wallIndices.size(), GL_UNSIGNED_INT, 0);
}

