#include "Scene.h"
#include <map>
#include <cstddef>

Scene::Scene(std::shared_ptr<Camera> camera, int scrWidth, int scrHeight) 
	: 
	camera_(camera), 
	scrWidth_(scrWidth), 
	scrHeight_(scrHeight)
{
	shader_ = std::make_unique<gl::Shader>("shader.vert", "shader.frag");
	//lightShader_ = std::make_unique<gl::Shader>("light_cube.vert", "light_cube.frag");
	//shaderSingleColor_ = std::make_unique<gl::Shader>("shader.vert", "stencil_single_color.frag");
	//glasscubeShader_ = std::make_unique<gl::Shader>("glasscube.vert", "glasscube.frag");
	//model_ = std::make_unique<Model>("resources\\objects\\backpack\\backpack.obj", cache_);
	screenshader_ = std::make_unique<gl::Shader>("fragment_quad.vert", "fragment_quad.frag");
	skyboxShader_ = std::make_unique<gl::Shader>("skybox.vert", "skybox.frag");

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
	glBindVertexArray(cubeVAO_);
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO_);
	glBufferData(GL_ARRAY_BUFFER, sizeof(gl::cubeVertices), gl::cubeVertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(gl::Vertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(gl::Vertex, normal));
	glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(gl::Vertex, uv));
    glBindVertexArray(0);

	glGenVertexArrays(1, &planeVAO_); // 床用のVAO
	glGenBuffers(1, &planeVBO_);
	glBindVertexArray(planeVAO_);
	glBindBuffer(GL_ARRAY_BUFFER, planeVBO_);
	glBufferData(GL_ARRAY_BUFFER, sizeof(gl::planeVertices), gl::planeVertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(gl::Vertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(gl::Vertex, normal));
	glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(gl::Vertex, uv));
    glBindVertexArray(0);

	glGenVertexArrays(1, &transparentVAO_); // 透過窓のVAO
	glGenBuffers(1, &transparentVBO_);
	glBindVertexArray(transparentVAO_);
	glBindBuffer(GL_ARRAY_BUFFER, transparentVBO_);
	glBufferData(GL_ARRAY_BUFFER, sizeof(gl::transparentVertices), gl::transparentVertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(gl::Vertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(gl::Vertex, normal));
	glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(gl::Vertex, uv));
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

	//glBindVertexArray(lightVAO_);
	//glBindBuffer(GL_ARRAY_BUFFER, VBO_);
	//glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(gl::Vertex, position));
	//glEnableVertexAttribArray(0);

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
	cubeTexture_ = cache_.get("resources\\textures\\marble.jpg", true);
	floorTexture_ = cache_.get("resources\\textures\\metal.png", true);
	transparentTexture_ = cache_.get("resources\\textures\\window.png", true);
	std::vector<std::string> faces
	{
		"resources\\textures\\skybox\\right.jpg",
		"resources\\textures\\skybox\\left.jpg",
		"resources\\textures\\skybox\\top.jpg",
		"resources\\textures\\skybox\\bottom.jpg",
		"resources\\textures\\skybox\\front.jpg",
		"resources\\textures\\skybox\\back.jpg"
	};
	cubemapTexture_ = cache_.loadCubemap(faces, false);

	shader_->use();
	shader_->setInt("texture1", 0);
	screenshader_->use();
	screenshader_->setInt("screenTexture", 0); 
	skyboxShader_->use();
	skyboxShader_->setInt("skybox", 0);
	//material_.setUniforms(*shader_);
}

void Scene::initFramebuffer() {
	// framebuffer configuration
    glGenFramebuffers(1, &framebuffer_); // フレームバッファ
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    // create a color attachment texture
    glGenTextures(1, &textureColorbuffer_); // 最終的に画面に貼り付けるカラーバッファ
    glBindTexture(GL_TEXTURE_2D, textureColorbuffer_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, scrWidth_, scrHeight_, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer_, 0);
    // create a renderbuffer object for depth and stencil attachment (we won't be sampling these)
    glGenRenderbuffers(1, &rbo_); // デプスやステンシルの処理を行う
    glBindRenderbuffer(GL_RENDERBUFFER, rbo_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, scrWidth_, scrHeight_); // use a single renderbuffer object for both a depth AND stencil buffer.
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo_); // now actually attach it
    // now that we actually created the framebuffer and added all attachments we want to check if it is actually complete now
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);// フレームバッファの初期化処理
}

void Scene::initUBO() {
    glGenBuffers(1, &ubo_);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo_);
    glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), nullptr, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo_);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

// whileループでの描画処理
void Scene::Render(float deltaTime)
{
	elapsedTime_ += deltaTime;
	const float t = static_cast<float>(fmod(elapsedTime_, animationTime_));

	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
	glEnable(GL_DEPTH_TEST); // enable depth testing (is disabled for rendering screen-space quad)
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	std::vector<gl::TransparentDraw> sorted;
	for (unsigned int i = 0; i < gl::windows_pos.size(); ++i)
	{
		float distance = glm::length(camera_->GetViewPosition() - gl::windows_pos[i]);
		sorted.push_back({ distance, i });
	}

	std::sort(sorted.begin(), sorted.end(),
		[](const gl::TransparentDraw& a, const gl::TransparentDraw& b)
		{
			return a.distance > b.distance;
		});

	// オブジェクト描画
	//shaderSingleColor_->use();
	glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 view = camera_->GetViewMatrix();
	glm::mat4 projection = glm::perspective(glm::radians(camera_->GetZoomValue()), (float)scrWidth_ / (float)scrHeight_, 0.1f, 100.0f);
	//shaderSingleColor_->setVec3("viewPos", camera_->GetViewPosition());
	//shaderSingleColor_->setMat4("view", view);
	//shaderSingleColor_->setMat4("projection", projection);
	glBindBuffer(GL_UNIFORM_BUFFER, ubo_);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(view));
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(projection));
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	shader_->use();
	// UBOによりviewとprojectionを送信するので、以下のコードはコメントアウト
	//shader_->setMat4("view", view);
	//shader_->setMat4("projection", projection);

	// cube
	glBindVertexArray(cubeVAO_);
	cubeTexture_->bind(0);
	model = glm::translate(model, glm::vec3(-1.0f, 0.0f, -1.0f));
	shader_->setMat4("model", model);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(2.0f, 0.0f, 0.0f));
	shader_->setMat4("model", model);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	// floor
	glBindVertexArray(planeVAO_);
	glBindTexture(GL_TEXTURE_2D, floorTexture_->getID());
	shader_->setMat4("model", glm::mat4(1.0f));
	glDrawArrays(GL_TRIANGLES, 0, 6);

	// draw skybox
	glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
	skyboxShader_->use();
	// skybox cube
	glBindVertexArray(skyboxVAO_);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture_);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
	glDepthFunc(GL_LESS); // set depth function back to default

	// transparent windows
	shader_->use();
	glBindVertexArray(transparentVAO_);
	glBindTexture(GL_TEXTURE_2D, transparentTexture_->getID());
	for (const auto& draw : sorted)
	{
		glm::vec3 position = gl::windows_pos[draw.index];
		model = glm::mat4(1.0f);
		model = glm::translate(model, position);
		shader_->setMat4("model", model);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	// now bind back to default framebuffer and draw a quad plane with the attached framebuffer color texture
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
	// clear all relevant buffers
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // set clear color to white (not really necessary actually, since we won't be able to see behind the quad anyways)
	glClear(GL_COLOR_BUFFER_BIT);

	screenshader_->use();
	glBindVertexArray(quadVAO_);
	glBindTexture(GL_TEXTURE_2D, textureColorbuffer_);	// use the color attachment texture as the texture of the quad plane
	glDrawArrays(GL_TRIANGLES, 0, 6);

	//directionalLight_.applyToShader(*shader_, "dirLight");

	//for (const auto& pointLight : pointLights)
	//{
	//	pointLight.applyToShader(*shader_, "pointLights[" + std::to_string(&pointLight - pointLights.data()) + "]");
	//}
	//spotlight_.applyToShader(*shader_, "spotLight", *camera_);

	//　リュックサックのモデルを描画
	//glm::mat4 backpackModel = glm::mat4(1.0f);
	//shader_->setMat4("model", backpackModel);
	//glm::mat3 backpackNormalMatrix = glm::transpose(glm::inverse(glm::mat3(backpackModel)));
	//shader_->setMat3("normalMatrix", backpackNormalMatrix);
	//model_->Draw(*shader_);
	//
	//material_.bind();
	//glBindVertexArray(VAO_);
	//for(const auto& position : gl::cubePositions)
	//{
	//	glm::mat4 model = glm::mat4(1.0f);
	//	model = glm::translate(model, position);
	//	float angle = 20.0f * ( & position - gl::cubePositions.data());
	//	model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
	//	shader_->setMat4("model", model);

	//	// 法線の行列はCPUで計算したほうが高速
	//	glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
	//	shader_->setMat3("normalMatrix", normalMatrix);

	//	glDrawArrays(GL_TRIANGLES, 0, 36);
	//}

	// 光源
	//lightShader_->use();
	//lightShader_->setMat4("view", view);
	//lightShader_->setMat4("projection", projection);

	//glBindVertexArray(lightVAO_);
	//lightShader_->setVec3("lightColor", pointlight_.diffuse);
	//for (const auto& pointLight : pointLights)
	//{
	//	glm::mat4 lightModel = glm::translate(glm::mat4(1.0f), pointLight.position);
	//	lightModel = glm::scale(lightModel, glm::vec3(0.2f)); // 小さく
	//	lightShader_->setMat4("model", lightModel);
	//	glDrawArrays(GL_TRIANGLES, 0, 36);
	//}
	//glDrawArrays(GL_TRIANGLES, 0, 36);
}

Scene::~Scene() {
	glDeleteVertexArrays(1, &cubeVAO_);
    glDeleteVertexArrays(1, &planeVAO_);
    glDeleteVertexArrays(1, &transparentVAO_);
    glDeleteVertexArrays(1, &quadVAO_);
    glDeleteVertexArrays(1, &skyboxVAO_);
    glDeleteBuffers(1, &cubeVBO_);
    glDeleteBuffers(1, &planeVBO_);
    glDeleteBuffers(1, &transparentVBO_);
    glDeleteBuffers(1, &quadVBO_);
    glDeleteBuffers(1, &skyboxVBO_);
    glDeleteFramebuffers(1, &framebuffer_);
    glDeleteTextures(1, &textureColorbuffer_);
    glDeleteTextures(1, &cubemapTexture_);
    glDeleteRenderbuffers(1, &rbo_);
	glDeleteBuffers(1, &ubo_);
}
