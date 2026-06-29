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
	light_ = std::make_unique<gl::Light>();

	//cubePositions_ = std::move(cubePositions);

	initMesh();
	initTextures();
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
	material_.diffuse = cache_.get("resources\\textures\\container2.png", false);
	material_.specular = cache_.get("resources\\textures\\container2_specular.png", true);

	shader_->use();
	material_.setUniforms(*shader_);
}

// whileループでの描画処理
void Scene::Draw(float deltaTime)
{
	elapsedTime_ += deltaTime;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	material_.bind();
	//texture1_->bind(0);
	//glActiveTexture(GL_TEXTURE1);
	//glBindTexture(GL_TEXTURE_2D, texture2_);
	//glBindVertexArray(VAO_);

	//shader_->use();

	//model = glm::rotate(model, (float)glfwGetTime(), glm::vec3(0.5f, 1.0f, 1.0f));
	//view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));
	//projection = glm::perspective(glm::radians(45.0f), (float)800/(float)600, 0.1f, 100.0f);
	

	const float t = static_cast<float>(fmod(elapsedTime_, animationTime_));

	//light_->color.x = sin(glfwGetTime() * 2.0f);
	//light_->color.y = sin(glfwGetTime() * 0.7f);
	//light_->color.z = sin(glfwGetTime() * 1.3f);

	//light_->diffuse = light_->color   * glm::vec3(0.5f);
	//glm::vec3 ambientColor = light_->diffuse * glm::vec3(0.2f);

	// オブジェクト描画
	shader_->use();
	glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 view = camera_->GetViewMatrix();
	glm::mat4 projection = glm::perspective(glm::radians(camera_->GetZoomValue()), (float)scrWidth_ / (float)scrHeight_, 0.1f, 100.0f);
	shader_->setMat4("model", model);
	shader_->setVec3("viewPos", camera_->GetViewPosition());
	shader_->setMat4("view", view);
	shader_->setMat4("projection", projection);

	shader_->setVec3("light.direction", camera_->GetViewFront());
	shader_->setVec3("light.position", camera_->GetViewPosition());
	shader_->setVec3("light.ambient", light_->ambient);
	shader_->setVec3("light.diffuse", light_->diffuse);
	shader_->setVec3("light.specular", light_->specular);
	shader_->setFloat("light.constant", light_->constant);
	shader_->setFloat("light.linear", light_->linear);
	shader_->setFloat("light.quadratic", light_->quadratic);
	shader_->setFloat("light.cutOff", light_->cutOff);
	shader_->setFloat("light.outerCutOff", light_->outerCutOff);

	//shader_->setVec3("material.ambient", 1.0f, 0.5f, 0.31f);
	//shader_->setVec3("material.specular", 0.5f, 0.5f, 0.5f);
	//shader_->setFloat("material.shininess", 32.0f);
	material_.bind();
	
	// 法線の行列はCPUで計算したほうが高速
	static glm::mat3 NormalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
	shader_->setMat3("normalMatrix", NormalMatrix);

	glBindVertexArray(VAO_);
	//glDrawArrays(GL_TRIANGLES, 0, 36);

	for(const auto& position : gl::cubePositions)
	{
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, position);
		float angle = 20.0f * ( & position - gl::cubePositions.data());
		model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
		shader_->setMat4("model", model);

		glDrawArrays(GL_TRIANGLES, 0, 36);
	}

	// 光源
	//lightShader_->use();
	//lightShader_->setVec3("lightColor", light_->color);
	//lightShader_->setMat4("view", view);
	//lightShader_->setMat4("projection", projection);
	//glm::mat4 lightModel = glm::translate(glm::mat4(1.0f), light_->position);
	//lightModel = glm::scale(lightModel, glm::vec3(0.2f)); // 小さく
	//lightShader_->setMat4("model", lightModel);

	//glBindVertexArray(lightVAO_);
	//glDrawArrays(GL_TRIANGLES, 0, 36);
}

Scene::~Scene() {
	glDeleteVertexArrays(1, &VAO_);
	glDeleteVertexArrays(1, &lightVAO_);
	glDeleteBuffers(1, &VBO_);
}
