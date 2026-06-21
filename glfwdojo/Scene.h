#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include <memory>
#include <vector>
#include "Shader.h"
#include "Camera.h"

class Scene
{
public:
	Scene(std::shared_ptr<Camera> camera, int srcWindow, int scrHeight);
	~Scene();
	void Draw(float deltaTime);

private:
	void initMesh();
	void initTextures();
	unsigned int loadTexture(const char* path, bool hasAlpha);
	unsigned int VAO_, VBO_;
	unsigned int texture1_, texture2_;
	std::unique_ptr<Shader> shader_;
	std::shared_ptr<Camera> camera_;
	int scrWidth_, scrHeight_;
	float elapsedTime_ = 0.0f;

	static constexpr float animationTime_ = 5.0f;
	std::vector<glm::vec3> cubePositions_;
	
};

