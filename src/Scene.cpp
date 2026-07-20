#include "Scene.h"
#include <map>
#include <cstddef>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

Scene::Scene(std::shared_ptr<Camera> camera, int scrWidth, int scrHeight)
	:
	camera_(camera),
	scrWidth_(scrWidth),
	scrHeight_(scrHeight)
{
	shader_ = std::make_unique<gl::Shader>("shader.vert", "shader.frag");
	cubeShader_ = std::make_unique<gl::Shader>("cube.vert", "cube.frag");
	lightcubeShader_ = std::make_unique<gl::Shader>("light_cube.vert", "light_cube.frag");
	//shaderSingleColor_ = std::make_unique<gl::Shader>("shader.vert", "stencil_single_color.frag");
	//glasscubeShader_ = std::make_unique<gl::Shader>("glasscube.vert", "glasscube.frag");
	//model_ = std::make_unique<Model>("resources\\objects\\backpack\\backpack.obj", cache_);
	screenshader_ = std::make_unique<gl::Shader>("fragment_quad.vert", "hdr.frag");  
	skyboxShader_ = std::make_unique<gl::Shader>("skybox.vert", "skybox.frag");
	transparentwindowShader_ = std::make_unique<gl::Shader>("window.vert", "shader.frag");
	pointDepthShader_ = std::make_unique<gl::Shader>("point_shadow_depth.vert", "point_shadow_depth.geom", "point_shadow_depth.frag");
	debugDepthShader_ = std::make_unique<gl::Shader>("fragment_quad.vert", "debug_depth.frag");
	wallShader_ = std::make_unique<gl::Shader>("wall.vert", "wall.frag");
	blurShader_ = std::make_unique<gl::Shader>("fragment_quad.vert", "blur.frag");

	//cubePositions_ = std::move(cubePositions);

	initMesh();
	initTextures();
	initFramebuffer();
	initUBO();
}

void Scene::initMesh() {
	int stride = sizeof(gl::Vertex);

	glGenVertexArrays(1, &cubeVAO_); // cube用のVAO
	glGenBuffers(1, &cubeVBO_);
	glGenBuffers(1, &cubeInstanceVBO_);
	glGenBuffers(1, &cubeEBO_);
	glBindVertexArray(cubeVAO_);
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO_);
	glBufferData(GL_ARRAY_BUFFER, sizeof(gl::cubeVertices), gl::cubeVertices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEBO_);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(gl::cubeIndices), gl::cubeIndices.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(gl::Vertex, position));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(gl::Vertex, normal));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(gl::Vertex, uv));
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(gl::Vertex, tangent));
	glEnableVertexAttribArray(5);
	glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(gl::Vertex, bitangent));
	glBindBuffer(GL_ARRAY_BUFFER, cubeInstanceVBO_);
	glBufferData(GL_ARRAY_BUFFER, cube_pos_.size() * sizeof(glm::vec3), cube_pos_.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
	glVertexAttribDivisor(3, 1); // インスタンスごとに変化する属性
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	glGenVertexArrays(1, &planeVAO_); // 床用のVAO
	glGenBuffers(1, &planeVBO_);
	glGenBuffers(1, &planeEBO_);
	glBindVertexArray(planeVAO_);
	glBindBuffer(GL_ARRAY_BUFFER, planeVBO_);
	glBufferData(GL_ARRAY_BUFFER, sizeof(gl::planeVertices), gl::planeVertices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, planeEBO_);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(gl::planeIndices), gl::planeIndices.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(gl::Vertex, position));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(gl::Vertex, normal));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(gl::Vertex, uv));
	glBindVertexArray(0);

	glGenVertexArrays(1, &transparentVAO_); // 透過窓のVAO
	glGenBuffers(1, &transparentVBO_);
	glGenBuffers(1, &transparentInstanceVBO_);
	glGenBuffers(1, &transparentEBO_);
	glBindVertexArray(transparentVAO_);
	glBindBuffer(GL_ARRAY_BUFFER, transparentVBO_);
	glBufferData(GL_ARRAY_BUFFER, sizeof(gl::transparentVertices), gl::transparentVertices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, transparentEBO_);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(gl::transparentIndices), gl::transparentIndices.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(gl::Vertex, position));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(gl::Vertex, normal));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(gl::Vertex, uv));
	glBindBuffer(GL_ARRAY_BUFFER, transparentInstanceVBO_);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * windows_pos_.size(), nullptr, GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
	glVertexAttribDivisor(3, 1); // インスタンスごとに変化する属性
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// skybox
	glGenVertexArrays(1, &skyboxVAO_); // スカイボックスのVAO
	glGenBuffers(1, &skyboxVBO_);
	glBindVertexArray(skyboxVAO_);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO_);
	glBufferData(GL_ARRAY_BUFFER, gl::skyboxVertices.size() * sizeof(glm::vec3), gl::skyboxVertices.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(gl::skyboxVertices[0]), (void*)0);
	glBindVertexArray(0);

	// 壁 (Normal Mapping 用: location 0-4 を使用)
	glGenVertexArrays(1, &wallVAO_);
	glGenBuffers(1, &wallVBO_);
	glGenBuffers(1, &wallEBO_);
	glBindVertexArray(wallVAO_);
	glBindBuffer(GL_ARRAY_BUFFER, wallVBO_);
	glBufferData(GL_ARRAY_BUFFER, sizeof(gl::wallVertices), gl::wallVertices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wallEBO_);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(gl::wallIndices), gl::wallIndices.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(gl::Vertex, position));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(gl::Vertex, normal));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(gl::Vertex, uv));
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(gl::Vertex, tangent));
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(gl::Vertex, bitangent));
	glBindVertexArray(0);

	// screen quad
	glGenVertexArrays(1, &quadVAO_); // スクリーンテクスチャのVAO
	glGenBuffers(1, &quadVBO_);
	glBindVertexArray(quadVAO_);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO_);
	glBufferData(GL_ARRAY_BUFFER, sizeof(gl::quadVertices), gl::quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glBindVertexArray(0);

	//glBindVertexArray(VAO_);
	//glBindBuffer(GL_ARRAY_BUFFER, VBO_);
	//glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(gl::Vertex, position));
	//glEnableVertexAttribArray(0);

	//glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(gl::Vertex, normal));
	//glEnableVertexAttribArray(1);

	//glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(gl::Vertex, uv));
	//glEnableVertexAttribArray(2);
}

void Scene::initTextures() {
	//material_.diffuse = cache_.get("resources\\textures\\container2.png", false);
	//material_.specular = cache_.get("resources\\textures\\container2_specular.png", true);
	cubeTexture_ = cache_.get("resources/textures/bricks2.jpg", true);
	cubeNormalMap_ = cache_.get("resources/textures/bricks2_normal.jpg", true);
	cubeHeightMap_ = cache_.get("resources/textures/bricks2_disp.jpg", true);

	floorTexture_ = cache_.get("resources/textures/wood.png", true);
	transparentTexture_ = cache_.get("resources/textures/window.png", true);
	std::vector<std::string> faces
	{
		"resources/textures/skybox/right.jpg",
		"resources/textures/skybox/left.jpg",
		"resources/textures/skybox/top.jpg",
		"resources/textures/skybox/bottom.jpg",
		"resources/textures/skybox/front.jpg",
		"resources/textures/skybox/back.jpg"
	};
	cubemapTexture_ = cache_.loadCubemap(faces, false);

	brickwallTexture_ = cache_.get("resources/textures/brickwall.jpg", true);
	brickwallNormalTexture_ = cache_.get("resources/textures/brickwall_normal.jpg", true);

	shader_->use();
	shader_->setInt("texture1", 0);
	shader_->setInt("shadowMap", 1);
	shader_->setFloat("farPlane", shadowFarPlane_);
	cubeShader_->use();
	cubeShader_->setInt("diffuseMap", 0);
	cubeShader_->setInt("shadowMap", 1);
	cubeShader_->setInt("normalMap", 2);
	cubeShader_->setInt("heightMap", 3);
	cubeShader_->setFloat("farPlane", shadowFarPlane_);
	transparentwindowShader_->use();
	transparentwindowShader_->setInt("texture1", 0);
	transparentwindowShader_->setInt("shadowMap", 1);
	transparentwindowShader_->setFloat("farPlane", shadowFarPlane_);
	screenshader_->use();
	screenshader_->setInt("screenTexture", 0);
	skyboxShader_->use();
	skyboxShader_->setInt("skybox", 0);
	debugDepthShader_->use();
	debugDepthShader_->setInt("depthMap", 0);
	debugDepthShader_->setFloat("near_plane", shadowNearPlane_);
	debugDepthShader_->setFloat("far_plane", shadowFarPlane_);
	wallShader_->use();
	wallShader_->setInt("texture1", 0);
	wallShader_->setInt("normalMap", 1);
	wallShader_->setInt("shadowMap", 2);
	wallShader_->setFloat("farPlane", shadowFarPlane_);
	//material_.setUniforms(*shader_);
	blurShader_->setInt("image", 0);
}

void Scene::initFramebuffer() {
	/* framebuffer configuration */
	glGenFramebuffers(1, &framebuffer_); // フレームバッファ（通常は *FBO とかに命名するけど…）
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
	// create a color attachment texture（通常のカラーバッファをアタッチメント location=0 -> FragColor）
	glGenTextures(1, &textureColorbuffer_); // 最終的に画面に貼り付けるカラーバッファ
	glBindTexture(GL_TEXTURE_2D, textureColorbuffer_);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, scrWidth_, scrHeight_, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer_, 0);
	// 明るい部分のカラーバッファをアタッチメント（location = 1 -> BrightColor）
	glGenTextures(1, &brightColorBuffer_);
	glBindTexture(GL_TEXTURE_2D, brightColorBuffer_);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, scrWidth_, scrHeight_, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, brightColorBuffer_, 0);
	unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);

	/* create a renderbuffer object for depth and stencil attachment(we won't be sampling these) */
	glGenRenderbuffers(1, &rbo_); // デプスやステンシルの処理を行う
	glBindRenderbuffer(GL_RENDERBUFFER, rbo_);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, scrWidth_, scrHeight_); // use a single renderbuffer object for both a depth AND stencil buffer.
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo_); // now actually attach it
	// now that we actually created the framebuffer and added all attachments we want to check if it is actually complete now
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);// フレームバッファの初期化処理

	/* ポイントシャドウ用キューブマップFBO */
	// depthMapFBO_: カラーバッファを持たず、深度だけを depthCubemap_ に書き込む専用FBO
	glGenFramebuffers(1, &depthMapFBO_);
	// depthCubemap_: 6面ぶんの深度テクスチャ。各テクセルには point_shadow_depth.frag が書き込む
	// 「光源からの正規化距離 [0,1]」が入る（通常のcubemapのような色情報ではない点に注意）
	glGenTextures(1, &depthCubemap_);
	glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap_);
	for (unsigned int i = 0; i < 6; ++i)
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT,
			SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO_);
	// depthCubemap_ をFBOの深度アタッチメントに設定。glFramebufferTexture (Texture2Dではない) を使うことで
	// 6面すべてが1つのアタッチメントとして扱われ、geometry shaderのgl_Layerで面を選択できるようになる
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap_, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenFramebuffers(2, pingpongFBO_);
    glGenTextures(2, pingpongColorbuffers_);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO_[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers_[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, scrWidth_, scrHeight_, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers_[i], 0);
        // also check if framebuffers are complete (no need for depth buffer)
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Framebuffer not complete!" << std::endl;
    }
}

void Scene::initUBO() {
	glGenBuffers(1, &matricesUBO_);
	glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO_);
	glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), nullptr, GL_STATIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, matricesUBO_);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void Scene::Render(float deltaTime, float heightScale)
{
	elapsedTime_ += deltaTime;
	heightScale_ = heightScale; // Parallax Mapping の強さ（Callbacks.cpp の processInput で矢印キーにより更新される）

	// 透過窓をカメラからの距離でソート
	std::vector<gl::TransparentDraw> sorted;
	for (unsigned int i = 0; i < windows_pos_.size(); ++i)
		sorted.push_back({ glm::length(camera_->GetViewPosition() - windows_pos_[i]), i });
	std::sort(sorted.begin(), sorted.end(),
		[](const gl::TransparentDraw& a, const gl::TransparentDraw& b) { return a.distance > b.distance; });

	// ポイントシャドウ用: 光源から6方向へのライト空間行列を計算
	// lightPos: シャドウを落とす点光源の位置。6方向すべての視点(lookAt)の原点になる
	glm::vec3 lightPos = pointLights_[0].position;
	// shadowProj: 立方体の1面をちょうど覆う画角(90度)の透視投影行列。6面共通で使い回す
	// near/far は shadowNearPlane_ / shadowFarPlane_ を使用（far は深度正規化の基準にもなる）
	glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f),
		(float)SHADOW_WIDTH / (float)SHADOW_HEIGHT, shadowNearPlane_, shadowFarPlane_);
	// shadowTransforms: cubemapの+X,-X,+Y,-Y,+Z,-Zの6面それぞれに対応する view*projection 行列
	// この配列を point_shadow_depth.geom の shadowMatrices[6] にそのまま渡す
	std::vector<glm::mat4> shadowTransforms;
	shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1, 0, 0), glm::vec3(0, -1, 0)));
	shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1, 0, 0), glm::vec3(0, -1, 0)));
	shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)));
	shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0, -1, 0), glm::vec3(0, 0, -1)));
	shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0, 0, 1), glm::vec3(0, -1, 0)));
	shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0, 0, -1), glm::vec3(0, -1, 0)));

	// ここから、シャドウデプスの作成
	// ── Pass 1: ポイントシャドウ デプスパス ──
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO_);
	glEnable(GL_DEPTH_TEST);
	glClear(GL_DEPTH_BUFFER_BIT);
	pointDepthShader_->use();
	// shadowTransforms[6] を geometry shader の uniform 配列 shadowMatrices[6] に渡す
	for (int i = 0; i < 6; ++i)
		pointDepthShader_->setMat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
	// フラグメントシェーダー側で距離を正規化する際の基準値（shader.frag側の farPlane と揃える）
	pointDepthShader_->setFloat("farPlane", shadowFarPlane_);
	// フラグメントシェーダー側で「光源からの距離」を計算するための光源位置
	pointDepthShader_->setVec3("lightPos", lightPos);
	renderFloor(*pointDepthShader_);
	renderCubes(*pointDepthShader_);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// ── Pass 2: メインパス ──
	glViewport(0, 0, scrWidth_, scrHeight_); // viewport を元に戻す
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// UBO に view / projection を書き込む
	glm::mat4 view = camera_->GetViewMatrix();
	glm::mat4 projection = glm::perspective(glm::radians(camera_->GetZoomValue()),
		(float)scrWidth_ / (float)scrHeight_, 0.1f, 100.0f);
	glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO_);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(view));
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(projection));
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	// キューブマップをシャドウ用テクスチャとして事前バインド
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap_);

	// キューブ
	cubeShader_->use();
	cubeShader_->setVec3("viewPos", camera_->GetViewPosition());
	// cube.vert で TangentLightPos(= TBN * lightPos) を計算するのに必要
	cubeShader_->setVec3("lightPos", pointLights_[0].position);
	cubeShader_->setMat3("normalMatrix", glm::mat3(1.0f));
	cubeShader_->setFloat("material.shininess", 32.0f);
	cubeShader_->setFloat("heightScale", heightScale_);
	applyPointLights(*cubeShader_);
	renderCubes(*cubeShader_);

	// 床
	shader_->use();
	shader_->setVec3("viewPos", camera_->GetViewPosition());
	shader_->setMat3("normalMatrix", glm::mat3(1.0f));
	shader_->setFloat("material.shininess", 32.0f);
	applyPointLights(*shader_);
	renderFloor(*shader_);

	// 壁 (Normal Mapping)
	wallShader_->use();
	wallShader_->setVec3("viewPos", camera_->GetViewPosition());
	wallShader_->setFloat("material.shininess", 32.0f);
	applyPointLights(*wallShader_);
	renderWalls();

	// ライトキューブ（FBO バインド中に描画）
	renderLightCubes();

	// スカイボックス
	renderSkybox();

	// 透過窓
	renderTransparentWindows(sorted);

	// ── スクリーンクワッド ──
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT);

	// [DEBUG] デプスマップを画面全体に表示して確認したい場合はここをアンコメント
	 //debugDepthShader_->use();
	 //glBindVertexArray(quadVAO_);
	 //glActiveTexture(GL_TEXTURE0);
	 //glBindTexture(GL_TEXTURE_2D, depthmapTexture_);
	 //glDrawArrays(GL_TRIANGLES, 0, 6);

	screenshader_->use();
	screenshader_->setFloat("exposure", exposure_);
	glBindVertexArray(quadVAO_);
	glBindTexture(GL_TEXTURE_2D, textureColorbuffer_);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Scene::applyPointLights(gl::Shader& shader)
{
	for (const auto& pointLight : pointLights_)
		pointLight.applyToShader(shader, "pointLights[" + std::to_string(&pointLight - pointLights_.data()) + "]");
}

void Scene::renderCubes(gl::Shader& shader)
{
	cubeTexture_->bind(0);
	cubeNormalMap_->bind(2);
	cubeHeightMap_->bind(3);
	glBindVertexArray(cubeVAO_);
	shader.setMat4("model", glm::mat4(1.0f));
	glDrawElementsInstanced(GL_TRIANGLES, gl::cubeIndices.size(), GL_UNSIGNED_INT, 0, cube_pos_.size());
}

void Scene::renderFloor(gl::Shader& shader)
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
	lightcubeShader_->setVec3("lightColor", pointlight_.diffuse);
	for (const auto& pointLight : pointLights_)
	{
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

void Scene::renderTransparentWindows(const std::vector<gl::TransparentDraw>& sorted)
{
	transparent_positions_.clear();
	for (const auto& draw : sorted)
		transparent_positions_.push_back(windows_pos_[draw.index]);

	glBindBuffer(GL_ARRAY_BUFFER, transparentInstanceVBO_);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3) * transparent_positions_.size(), transparent_positions_.data());
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	transparentwindowShader_->use();
	transparentwindowShader_->setVec3("viewPos", camera_->GetViewPosition());
	transparentwindowShader_->setMat3("normalMatrix", glm::mat3(1.0f));
	transparentwindowShader_->setFloat("material.shininess", 32.0f);
	applyPointLights(*transparentwindowShader_);
	glBindVertexArray(transparentVAO_);
	transparentTexture_->bind(0);
	glDrawElementsInstanced(GL_TRIANGLES, gl::transparentIndices.size(), GL_UNSIGNED_INT, 0, transparent_positions_.size());
}

void Scene::renderWalls()
{
	brickwallTexture_->bind(0);
	brickwallNormalTexture_->bind(1);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap_);

	glBindVertexArray(wallVAO_);
	wallShader_->setMat4("model", glm::mat4(1.0f));
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
	glDeleteFramebuffers(1, &depthMapFBO_);
	glDeleteTextures(1, &textureColorbuffer_);
	glDeleteTextures(1, &cubemapTexture_);
	glDeleteTextures(1, &depthCubemap_);
	glDeleteRenderbuffers(1, &rbo_);
	glDeleteBuffers(1, &matricesUBO_);
	// TODO: brightColorBuffer_, pingpongFBO_[2], pingpongColorbuffers_[2] の glDelete* を追加。
	glDeleteTextures(1, &brightColorBuffer_);
	glDeleteFramebuffers(2, pingpongFBO_);
	glDeleteTextures(2, pingpongColorbuffers_);
}