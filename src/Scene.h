#pragma once
#include <memory>
#include <vector>
#include "Shader.h"
#include "Camera.h"
#include "Lighting.h"
#include "GeometryData.h"
#include "TextureCache.h"
#include "Material.h"

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
	Material material_;
	TextureCache cache_;
	std::unique_ptr<gl::Shader> shader_;
	std::unique_ptr<gl::Shader> lightShader_;
	std::shared_ptr<Camera> camera_;

	gl::PointLight pointlight_;
	gl::DirectionalLight directionalLight_;
	gl::SpotLight spotlight_;
	//std::shared_ptr<Texture> texture1_;
	//std::shared_ptr<Texture> texture2_;
	int scrWidth_, scrHeight_;
	float elapsedTime_ = 0.0f;
	int viewLoc_ = -1; // viewのローケーション番号(初期値-1)

	static constexpr float animationTime_ = 5.0f;
	std::vector<glm::vec3> cubePositions_;
	std::array<gl::PointLight, 4> pointLights = {{
		{ glm::vec3( 0.7f,  0.2f,  2.0f) },
		{ glm::vec3( 2.3f, -3.3f, -4.0f) },
		{ glm::vec3(-4.0f,  2.0f,-12.0f) },
		{ glm::vec3( 0.0f,  0.0f, -3.0f) },
	}};
	
};

