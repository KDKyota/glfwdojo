#include "Scene.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Scene::Scene(std::shared_ptr<Camera> camera, int scrWidth, int scrHeight) 
	: 
	camera_(camera), 
	scrWidth_(scrWidth), 
	scrHeight_(scrHeight)
{
	shader_ = std::make_unique<Shader>("shader.vert", "shader.frag");

	std::vector<glm::vec3> cubePositions = {
		glm::vec3( 0.0f,  0.0f,  0.0f), 
		glm::vec3( 2.0f,  5.0f, -15.0f), 
		glm::vec3(-1.5f, -2.2f, -2.5f),  
		glm::vec3(-3.8f, -2.0f, -12.3f),  
		glm::vec3( 2.4f, -0.4f, -3.5f),  
		glm::vec3(-1.7f,  3.0f, -7.5f),  
		glm::vec3( 1.3f, -2.0f, -2.5f),  
		glm::vec3( 1.5f,  2.0f, -2.5f), 
		glm::vec3( 1.5f,  0.2f, -1.5f), 
		glm::vec3(-1.3f,  1.0f, -1.5f)  
	};

	cubePositions_ = std::move(cubePositions);

	initMesh();
	initTextures();
}

void Scene::initMesh() {
	float vertices[] = {
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
		 0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f

	};

	glGenVertexArrays(1, &VAO_);
	glGenBuffers(1, &VBO_);
	glBindVertexArray(VAO_);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	int stride = 5 * sizeof(float);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
}


void Scene::initTextures() {
	stbi_set_flip_vertically_on_load(true);
	texture1_ = loadTexture("resources\\textures\\container.jpg", false);
	texture2_ = loadTexture("resources\\textures\\awesomeface.png", true);

	shader_->use();
	shader_->setInt("texture1", 0);
	shader_->setInt("texture2", 1);
}


unsigned int Scene::loadTexture(const char* path, bool hasAlpha) {

	unsigned int textureID; // return value

	// texture2
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
		GLenum format = hasAlpha ? GL_RGBA : GL_RGB; // 透過色あり(GL_RGBA)/なし(GL_RGB)
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

void Scene::Draw(float deltaTime)
{
	elapsedTime_ += deltaTime;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glActiveTexture(GL_TEXTURE0); // active a texture unit before binding a texture
	glBindTexture(GL_TEXTURE_2D, texture1_);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture2_);
	glBindVertexArray(VAO_);

	shader_->use();

	//glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 view = camera_->GetViewMatrix();
	glm::mat4 projection = glm::perspective(glm::radians(camera_->GetZoomValue()), (float)scrWidth_ / (float)scrHeight_, 0.1f, 100.0f);
	//model = glm::rotate(model, (float)glfwGetTime(), glm::vec3(0.5f, 1.0f, 1.0f));
	//view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));
	//projection = glm::perspective(glm::radians(45.0f), (float)800/(float)600, 0.1f, 100.0f);
	// pass them to the shader
	//unsigned int modelLoc = glGetUniformLocation(ourShader.ID, "model");
	unsigned int viewLoc = glGetUniformLocation(shader_->ID, "view");
	//glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	shader_->setMat4("projection", projection);
	

	const float t = static_cast<float>(fmod(elapsedTime_, animationTime_));

	for (unsigned int i = 0; i < 10; i++)
	{
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, cubePositions_[i]);
		float first_angle = 20.0f * i;
		model = glm::rotate(model, (float)((glm::radians)(first_angle)+(glm::radians)(t / animationTime_) * 360.0f), glm::vec3(1.0f, 0.3f, 0.5f));
		shader_->setMat4("model", model);
		glDrawArrays(GL_TRIANGLES, 0, 36);
	}



}

Scene::~Scene() {
	glDeleteVertexArrays(1, &VAO_);
	glDeleteBuffers(1, &VBO_);
}
