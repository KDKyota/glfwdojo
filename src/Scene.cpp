#include "Scene.h"
#include <cstddef>

Scene::Scene(std::shared_ptr<Camera> camera, int scrWidth, int scrHeight) 
	: 
	camera_(camera), 
	scrWidth_(scrWidth), 
	scrHeight_(scrHeight)
{
	shader_ = std::make_unique<gl::Shader>("shader.vert", "shader.frag");
	lightShader_ = std::make_unique<gl::Shader>("light_cube.vert", "light_cube.frag");
	model_ = std::make_unique<Model>("resources\\objects\\backpack\\backpack.obj", cache_);

	//cubePositions_ = std::move(cubePositions);

	initMesh();
	//initTextures();
}

void Scene::initMesh() {

	glGenVertexArrays(1, &lightVAO_); // LightingのVAO
	glGenVertexArrays(1, &VAO_); // object用のVAO
	glGenBuffers(1, &VBO_);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_);
	glBufferData(GL_ARRAY_BUFFER, sizeof(gl::cubeVertices), gl::cubeVertices.data(), GL_STATIC_DRAW);

	glBindVertexArray(lightVAO_);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_);
	int stride = sizeof(gl::Vertex);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(gl::Vertex, position));
	glEnableVertexAttribArray(0);

	glBindVertexArray(VAO_);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(gl::Vertex, position));
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(gl::Vertex, normal));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(gl::Vertex, uv));
	glEnableVertexAttribArray(2);
}


void Scene::initTextures() {
	//material_.diffuse = cache_.get("resources\\textures\\container2.png", false);
	//material_.specular = cache_.get("resources\\textures\\container2_specular.png", true);

	//shader_->use();
	//material_.setUniforms(*shader_);
}

// whileループでの描画処理
void Scene::Render(float deltaTime)
{
	elapsedTime_ += deltaTime;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	const float t = static_cast<float>(fmod(elapsedTime_, animationTime_));

	// オブジェクト描画
	shader_->use();
	glm::mat4 view = camera_->GetViewMatrix();
	glm::mat4 projection = glm::perspective(glm::radians(camera_->GetZoomValue()), (float)scrWidth_ / (float)scrHeight_, 0.1f, 100.0f);
	shader_->setVec3("viewPos", camera_->GetViewPosition());
	shader_->setMat4("view", view);
	shader_->setMat4("projection", projection);

	directionalLight_.applyToShader(*shader_, "dirLight");
	for (const auto& pointLight : pointLights)
	{
		pointLight.applyToShader(*shader_, "pointLights[" + std::to_string(&pointLight - pointLights.data()) + "]");
	}
	spotlight_.applyToShader(*shader_, "spotLight", *camera_);

	//　リュックサックのモデルを描画
	glm::mat4 backpackModel = glm::mat4(1.0f);
	shader_->setMat4("model", backpackModel);
	glm::mat3 backpackNormalMatrix = glm::transpose(glm::inverse(glm::mat3(backpackModel)));
	shader_->setMat3("normalMatrix", backpackNormalMatrix);
	model_->Draw(*shader_);
	
	material_.bind();
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
	lightShader_->use();
	lightShader_->setMat4("view", view);
	lightShader_->setMat4("projection", projection);

	glBindVertexArray(lightVAO_);
	lightShader_->setVec3("lightColor", pointlight_.diffuse);
	for (const auto& pointLight : pointLights)
	{
		glm::mat4 lightModel = glm::translate(glm::mat4(1.0f), pointLight.position);
		lightModel = glm::scale(lightModel, glm::vec3(0.2f)); // 小さく
		lightShader_->setMat4("model", lightModel);
		glDrawArrays(GL_TRIANGLES, 0, 36);
	}
	glDrawArrays(GL_TRIANGLES, 0, 36);
}

Scene::~Scene() {
	glDeleteVertexArrays(1, &VAO_);
	glDeleteVertexArrays(1, &lightVAO_);
	glDeleteBuffers(1, &VBO_);
}
