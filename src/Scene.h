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
	void Render(float deltaTime, float heightScale);

private:
	void initMesh();
	void initTextures();
	void initFramebuffer();
	void initGBuffer();
	unsigned int loadTexture(const char* path, bool hasAlpha);
	void initUBO();
	void applyPointLights(gl::Shader& shader);
	void renderCubes(gl::Shader& shader);
	void renderFloor(gl::Shader& shader);
	void renderLightCubes();
	void renderSkybox();
	void renderTransparentWindows(const std::vector<gl::TransparentDraw>& sorted);
	void renderWalls(gl::Shader& shader);

	static constexpr unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024; // depthCubemap_ 各面の解像度
	int scrWidth_, scrHeight_;
	static constexpr float shadowNearPlane_ = 1.0f; // 光源視点の投影のnear plane（shadowProj計算に使用）
	static constexpr float shadowFarPlane_  = 50.0f; // 光源視点の投影のfar plane。シェーダー側の farPlane uniform と同じ値で、深度の正規化・逆正規化の基準になる

	/* メッシュのVAO / VBO / EBO */
	unsigned int cubeVAO_, planeVAO_, cubeVBO_, planeVBO_, transparentVAO_, transparentVBO_, quadVAO_, quadVBO_, skyboxVAO_, skyboxVBO_;
	unsigned int lightVAO_, lightVBO_;
	unsigned int cubeInstanceVBO_;
	unsigned int transparentInstanceVBO_;
	unsigned int cubeEBO_, planeEBO_, transparentEBO_;
	unsigned int wallVAO_, wallVBO_, wallEBO_;
	/* フレームバッファ */
	unsigned int framebuffer_, textureColorbuffer_, rbo_;  // メンバ変数として持つ
	unsigned int depthMapFBO_[4]; // point shadow のデプスパス専用フレームバッファ（depthCubemap_ をアタッチ）
	unsigned int pingpongFBO_[2]; // HDRレンダリング後のガウシアンブラー用フレームバッファ（pingpongColorbuffers_ をアタッチ）
	unsigned int pingpongColorbuffers_[2]; // HDRレンダリング後のガウシアンブラー用テクスチャ（pingpongFBO_ にアタッチ）
	unsigned int brightColorBuffer_; // HDRレンダリング後の明るい部分だけを抽出するためのテクスチャ（pingpongFBO_ にアタッチ）
	/* UBO */
	unsigned int matricesUBO_;
	/* G-Buffer */
	unsigned int gBuffer_; 
	unsigned int gPosition_, gNormal_, gAlbedoSpec_; // gBuffer_にアタッチする3枚のテクスチャ
	unsigned int gDepthRBO_; // gBuffer_ 用の深度バッファ

	Material material_;
	TextureCache cache_;

	/* Shaders */
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
	std::unique_ptr<gl::Shader> pointDepthShader_;
	std::unique_ptr<gl::Shader> debugDepthShader_;
	std::unique_ptr<gl::Shader> wallShader_;
	std::unique_ptr<gl::Shader> blurShader_; // Boolの際にぼかしを入れるシェーダ
	/* G-Bufferのシェーダ */
	std::unique_ptr<gl::Shader> gbufferFloorShader_;
	std::unique_ptr<gl::Shader> gbufferWallShader_;
	std::unique_ptr<gl::Shader> gbufferCubeShader_;
	std::unique_ptr<gl::Shader> deferredLightingShader_;

	//gl::DirectionalLight directionalLight_;
	//gl::SpotLight spotlight_;

	/* Textures */
	std::shared_ptr<Texture> cubeTexture_;
	std::shared_ptr<Texture> cubeNormalMap_;
	std::shared_ptr<Texture> cubeHeightMap_;
	std::shared_ptr<Texture> floorTexture_;
	std::shared_ptr<Texture> transparentTexture_;
	std::shared_ptr<Texture> brickwallTexture_;
	std::shared_ptr<Texture> brickwallNormalTexture_;
	unsigned int cubemapTexture_;
	unsigned int depthCubemap_[4]; // ポイントシャドウ用キューブマップ（各テクセルは光源からの正規化距離 [0,1]。shader.frag/wall.frag の shadowMap にバインドされる）

	float elapsedTime_ = 0.0f;
	float heightScale_ = 0.0f;
	float exposure_ = 0.3f; // HDR tone mappingの明るさ調整
	int viewLoc_ = -1; // viewのローケーション番号(初期値-1)
	bool blurEnable_ = true;
	bool horizontal_ = true;
	std::vector<glm::vec3> transparent_positions_; // 透過オブジェクトのモデル行列を格納する配列

	static constexpr float animationTime_ = 5.0f;
	std::vector<glm::vec3> cubePositions_;

	/* シーン固有の配置データ */
	const std::vector<glm::vec3> cube_pos_ = {
		glm::vec3(-1.0f, 1.0f, -1.0f),
		glm::vec3(2.0f,  1.5f,  0.0f),
		glm::vec3(0.0f,  1.0f, -20.0f), // z=-25 壁の前
		glm::vec3(0.0f,  1.0f,  20.0f), // z=+25 壁の前
	};
	
	// point light の位置と色（LearnOpenGL Bloomチュートリアルの配色をベース）
	//
	// 減衰係数は LearnOpenGL の減衰テーブルの「到達距離 32」の行を使用している。
	// ここを緩く（到達距離を長く）しすぎると、4灯すべてがシーン全域に届いてしまい、
	// 1灯が遮られても残り3灯が影を埋めてしまうため、point shadow がほとんど見えなくなる。
	// 逆に constant=0（= 1/d^2）にすると光源の至近距離しか照らされず全体が真っ暗になるので、
	// constant=1.0 を保ったまま linear / quadratic で到達距離を絞るのが扱いやすい。
	//
	// diffuse / specular を上げすぎると hdr.frag のトーンマッピング
	// （mapped = 1 - exp(-hdrColor * exposure)）が飽和して明暗差が潰れ、さらに輝度1.0超えの
	// 面が増えて Bloom が影の上に滲む。全体の明るさは exposure_ 側で調整すること。
	//
	// ambient は影の内側が完全な黒に潰れないための下駄。ここも上げすぎると影が薄くなる。
	const std::array<gl::PointLight, 4> pointLights_ = {{
		// position,                    ambient,                        diffuse,                     specular,                    constant, linear, quadratic
		{glm::vec3( 0.0f, 0.8f,  2.2f), glm::vec3(0.04f, 0.04f, 0.04f), glm::vec3(1.8f, 1.8f, 1.8f), glm::vec3(0.9f, 0.9f, 0.9f), 1.0f, 0.14f, 0.07f},
		{glm::vec3(-5.0f, 0.8f, -4.0f), glm::vec3(0.04f, 0.01f, 0.01f), glm::vec3(2.2f, 0.4f, 0.25f), glm::vec3(1.1f, 0.2f, 0.12f), 1.0f, 0.14f, 0.07f},
		{glm::vec3( 4.2f, 3.0f,  1.8f), glm::vec3(0.01f, 0.02f, 0.04f), glm::vec3(0.4f, 0.6f, 2.2f), glm::vec3(0.2f, 0.3f, 1.1f), 1.0f, 0.14f, 0.07f},
		{glm::vec3(-1.5f, 3.0f, -2.2f), glm::vec3(0.01f, 0.04f, 0.02f), glm::vec3(0.35f, 1.8f, 0.5f), glm::vec3(0.18f, 0.9f, 0.25f), 1.0f, 0.14f, 0.07f}
	}};

	// 透過オブジェクトの位置
	const std::vector<glm::vec3> windows_pos_ = {
		glm::vec3(-1.5f, 0.0f, -0.48f),
		glm::vec3( 1.5f, 0.0f, 0.51f),
		glm::vec3( 0.0f, 0.0f, 0.7f),
		glm::vec3(-0.3f, 0.0f, -2.3f),
		glm::vec3( 0.5f, 0.0f, -0.6f)
	};
};

