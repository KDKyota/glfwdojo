#pragma once
#include <memory>
#include <vector>
#include "Shader.h"
#include "Camera.h"
#include "Lighting.h"
#include "GeometryData.h"
#include "TextureCache.h"
#include "Model.h"
#include "Material.h"

class Scene
{
public:
	Scene(std::shared_ptr<Camera> camera, int srcWindow, int scrHeight);
	~Scene();
	void Render(float deltaTime);
	unsigned int framebuffer_, textureColorbuffer_, rbo_;  // メンバ変数として持つ

private:
	void initMesh();
	void initTextures();
	void initFramebuffer();
	static constexpr unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
	static constexpr float shadowNearPlane_ = 1.0f;
	static constexpr float shadowFarPlane_  = 7.5f;
	unsigned int loadTexture(const char* path, bool hasAlpha);
	unsigned int cubeVAO_, planeVAO_, cubeVBO_, planeVBO_, transparentVAO_, transparentVBO_, quadVAO_, quadVBO_, skyboxVAO_, skyboxVBO_;
	unsigned int lightVAO_, lightVBO_;
	unsigned int cubeInstanceVBO_;
	unsigned int transparentInstanceVBO_;
	unsigned int cubeEBO_, planeEBO_, transparentEBO_;
	unsigned int depthMapFBO_;

	Material material_;
	TextureCache cache_;
	std::unique_ptr<gl::Shader> shader_;
	std::unique_ptr<gl::Shader> cubeShader_;
	std::unique_ptr<gl::Shader> transparentwindowShader_;
	std::unique_ptr<gl::Shader> lightcubeShader_;
	//std::unique_ptr<gl::Shader> shaderSingleColor_;
    std::unique_ptr<gl::Shader> screenshader_;
	//std::unique_ptr<gl::Shader> glasscubeShader_;
	std::unique_ptr<gl::Shader> skyboxShader_;
	//std::unique_ptr<Model> model_; // 3Dモデル
	std::shared_ptr<Camera> camera_;
	std::unique_ptr<gl::Shader> depthShader_;
	std::unique_ptr<gl::Shader> debugDepthShader_;

	unsigned int matricesUBO_;
	
	void initUBO();

	void applyPointLights(gl::Shader& shader);
	void renderCubes(gl::Shader& shader);
	void renderFloor(gl::Shader& shader);
	void renderLightCubes();
	void renderSkybox();
	void renderTransparentWindows(const std::vector<gl::TransparentDraw>& sorted);

	gl::PointLight pointlight_;
	//gl::DirectionalLight directionalLight_;
	//gl::SpotLight spotlight_;
	std::shared_ptr<Texture> cubeTexture_;
	std::shared_ptr<Texture> floorTexture_;
	std::shared_ptr<Texture> transparentTexture_;
	unsigned int cubemapTexture_;
	unsigned int depthmapTexture_; // 陰影レンダリング用のテクスチャ
	int scrWidth_, scrHeight_;
	float elapsedTime_ = 0.0f;
	int viewLoc_ = -1; // viewのローケーション番号(初期値-1)
	std::vector<glm::vec3> transparent_positions_; // 透過オブジェクトのモデル行列を格納する配列

	static constexpr float animationTime_ = 5.0f;
	std::vector<glm::vec3> cubePositions_;
};

