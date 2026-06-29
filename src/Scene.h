#pragma once
#include <memory>
#include <vector>
#include "Shader.h"
#include "Camera.h"
#include "Lighting.h"
#include "GeometryData.h"

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
	unsigned int VAO_, lightVAO_, VBO_;
	unsigned int texture1_;
	std::unique_ptr<Shader> shader_;
	std::unique_ptr<Shader> lightShader_;
	std::shared_ptr<Camera> camera_;
	std::unique_ptr<gl::Light> light_;
	int scrWidth_, scrHeight_;
	float elapsedTime_ = 0.0f;
	int viewLoc_ = -1; // viewのローケーション番号(初期値-1)

	static constexpr float animationTime_ = 5.0f;
	std::vector<glm::vec3> cubePositions_;
	
};

