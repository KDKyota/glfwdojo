#include "Scene.h"
#include "stb_image.h"
#include <cstddef>

Scene::Scene(std::shared_ptr<Camera> camera, int scrWidth, int scrHeight) 
	: 
	camera_(camera), 
	scrWidth_(scrWidth), 
	scrHeight_(scrHeight)
{
	shader_ = std::make_unique<Shader>("shader.vert", "shader.frag");
	lightShader_ = std::make_unique<Shader>("light_cube.vert", "light_cube.frag");
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
	stbi_set_flip_vertically_on_load(true);
	texture1_ = loadTexture("resources\\textures\\container2.png", false);
	//texture2_ = loadTexture("resources\\textures\\awesomeface.png", true);

	shader_->use();
	shader_->setInt("material.diffuse", 0);
	//shader_->setInt("texture2", 1);
}


unsigned int Scene::loadTexture(const char* path, bool hasAlpha) {

	unsigned int textureID; // return value

	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	// set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// set texture filtering parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // when texture area is smaller than the image, use mipmap linear filter
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // when texture area is larger than the image, use linear filter

	int width, height, nrChannels;
	unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
	if (data)
	{
		GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB; // PNG実チャンネル数で判定
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);

	return textureID;
}

// whileループでの描画処理
void Scene::Draw(float deltaTime)
{
	elapsedTime_ += deltaTime;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glActiveTexture(GL_TEXTURE0); // active a texture unit before binding a texture
	glBindTexture(GL_TEXTURE_2D, texture1_);
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

	shader_->setVec3("light.position", light_->position);
	shader_->setVec3("light.ambient", light_->ambient);
	shader_->setVec3("light.diffuse", light_->diffuse);
	shader_->setVec3("light.specular", light_->specular);

	shader_->setVec3("material.ambient", 1.0f, 0.5f, 0.31f);
	shader_->setVec3("material.specular", 0.5f, 0.5f, 0.5f);
	shader_->setFloat("material.shininess", 32.0f);
	
	static glm::mat3 NormalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
	shader_->setMat3("normalMatrix", NormalMatrix);

	glBindVertexArray(VAO_);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	// 光源
	lightShader_->use();
	lightShader_->setVec3("lightColor", light_->color);
	lightShader_->setMat4("view", view);
	lightShader_->setMat4("projection", projection);
	glm::mat4 lightModel = glm::translate(glm::mat4(1.0f), light_->position);
	lightModel = glm::scale(lightModel, glm::vec3(0.2f)); // 小さく
	lightShader_->setMat4("model", lightModel);

	glBindVertexArray(lightVAO_);
	glDrawArrays(GL_TRIANGLES, 0, 36);
}

Scene::~Scene() {
	glDeleteVertexArrays(1, &VAO_);
	glDeleteVertexArrays(1, &lightVAO_);
	glDeleteBuffers(1, &VBO_);
	glDeleteTextures(1, &texture1_);
	//glDeleteTextures(1, &texture2_);
}
