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
	shaderSingleColor_ = std::make_unique<gl::Shader>("shader.vert", "stencil_single_color.frag");
	//model_ = std::make_unique<Model>("resources\\objects\\backpack\\backpack.obj", cache_);

	//cubePositions_ = std::move(cubePositions);

	initMesh();
	initTextures();
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

	shader_->use();
	shader_->setInt("texture1", 0);
	//material_.setUniforms(*shader_);
}

// whileループでの描画処理
void Scene::Render(float deltaTime)
{
	elapsedTime_ += deltaTime;

	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	const float t = static_cast<float>(fmod(elapsedTime_, animationTime_));

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
	shaderSingleColor_->use();
	glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 view = camera_->GetViewMatrix();
	glm::mat4 projection = glm::perspective(glm::radians(camera_->GetZoomValue()), (float)scrWidth_ / (float)scrHeight_, 0.1f, 100.0f);
	shaderSingleColor_->setVec3("viewPos", camera_->GetViewPosition());
	shaderSingleColor_->setMat4("view", view);
	shaderSingleColor_->setMat4("projection", projection);

	// floor
	glBindVertexArray(planeVAO_);
	glBindTexture(GL_TEXTURE_2D, floorTexture_->getID());
	shader_->use();
	shader_->setMat4("model", glm::mat4(1.0f));
	shader_->setMat4("view", view);
	shader_->setMat4("projection", projection);
	glDrawArrays(GL_TRIANGLES, 0, 6);

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

	// transparent windows
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
	glDeleteVertexArrays(1, &planeVBO_);
	glDeleteBuffers(1, &cubeVBO_);
	glDeleteBuffers(1, &planeVBO_);
}
