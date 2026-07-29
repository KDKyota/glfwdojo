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

	// ImGui のパネルから設定値を直接編集するためのアクセサ。
	// private メンバを個別に公開すると getter/setter が際限なく増えるので、
	// 編集対象への参照だけをまとめて渡している。
	int&   DebugMode()       { return debugMode_; }
	bool&  DebugRawOutput()  { return debugRawOutput_; }
	float& SsaoStrength()    { return ssaoStrength_; }
	float& Exposure()        { return exposure_; }

private:
	void initMesh();
	void initTextures();
	void initFramebuffer();
	void initGBuffer();
	void initSSAO();
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

	/* ==== UI から実行時に変更する設定 ====
	 * 以前はシェーダーの #define だったが、値を変えるたびに編集して再ビルドする
	 * 必要があったため uniform 化し、ここから毎フレーム送るようにした。 */
	// deferred_lighting.frag の表示切り替え
	//   0=通常 / 1=ライト0のシャドウ / 2=shadowMap[0]の生値
	//   3=Albedo / 4=Normal / 5=Position / 6=4分割 / 7=SSAO
	int debugMode_ = 0;
	// hdr.frag のトーンマッピング・Bloom・ガンマ補正をすべて飛ばすか。
	// G-Buffer や AO を可視化するときは true にしないと階調が潰れて判定できない。
	bool debugRawOutput_ = false;
	// SSAO の効き具合（0.0 = 無効, 1.0 = そのまま適用）
	float ssaoStrength_ = 1.0f;

	/* SSAO */
	unsigned int ssaoFBO_, ssaoBlurFBO_;
	unsigned int ssaoColorBuffer_, ssaoColorBufferBlur_;  // GL_R8
	unsigned int noiseTexture_;                            // 4x4 GL_RGBA16F
	std::vector<glm::vec3> ssaoKernel_;                    // 接空間のサンプル点

	std::unique_ptr<gl::Shader> ssaoShader_;
	std::unique_ptr<gl::Shader> ssaoBlurShader_;

	static constexpr unsigned int SSAO_KERNEL_SIZE = 64;
	// 遮蔽物とみなす近傍の半径（ワールド空間の長さ）。「暗くなる帯の幅」を決める。
	// 基準は暗くしたい隙間や角の大きさで、目安は物体サイズの 0.2〜1.0 倍。
	// このシーンは cube が1辺 1.0 なので 0.6（＝cube の半分強）。
	static constexpr float SSAO_RADIUS = 0.6f;
	// 自己遮蔽によるアクネ対策。radius に対する相対値が効くので、
	// radius を変えたらこちらも比例させること。
	static constexpr float SSAO_BIAS = 0.03f;
	// AO のコントラスト。ブラー後の遮蔽率を pow(ao, SSAO_POWER) する。
	// 1.0 で素の値。上げるほど中間調が暗く押し下げられ、AO がはっきりする。
	//   例) ao=0.7 のとき  power=1.0 → 0.70 / power=2.0 → 0.49 / power=3.0 → 0.34
	// 「効果がうっすらしか見えない」ときに最初に触るのがここ。
	// 上げすぎると接地部が不自然に黒く落ち込むので 1.5〜3.0 程度が実用範囲。
	static constexpr float SSAO_POWER = 2.0f;

	// シーン全体の環境光の強さ。LearnOpenGL の SSAO の章の
	//   vec3 ambient = vec3(0.3 * Diffuse * AmbientOcclusion);
	// の 0.3 に相当する。
	// SSAO が掛かるのはこの項だけなので、ここを 0 にすると AO は完全に見えなくなる。
	static constexpr float AMBIENT_STRENGTH = 0.3f;

	float elapsedTime_ = 0.0f;
	float heightScale_ = 0.0f;
	// HDR tone mapping の明るさ調整。
	// LearnOpenGL の SSAO の章はトーンマッピングを通さず値を直接出力しているので、
	// それに近い明るさになるよう高めに設定している。
	// exposure は HDR値そのものを変えないので、Bloom の閾値やトーンマッピングの飽和に
	// 影響せず、AO や影のコントラストを保ったまま全体を明るくできる。
	float exposure_ = 2.0f;
	int viewLoc_ = -1; // viewのローケーション番号(初期値-1)
	bool blurEnable_ = true;
	bool horizontal_ = true;
	std::vector<glm::vec3> transparent_positions_; // 透過オブジェクトのモデル行列を格納する配列

	static constexpr float animationTime_ = 5.0f;
	std::vector<glm::vec3> cubePositions_;

	/* シーン固有の配置データ */
	// cube は局所座標 -0.5〜+0.5（1辺 1.0）、床は y = -0.5 にある。
	// そのため y = 0.0 に置くと底面がちょうど床に接する。
	// SSAO は「面と面が近づいている場所」でしか効かないので、接地させておかないと
	// 実装が正しくても画面上で何も起きず、成否を判定できない。
	const std::vector<glm::vec3> cube_pos_ = {
		glm::vec3(-1.0f, 0.0f, -1.0f), // 接地。根元に AO が出る
		glm::vec3( 2.0f, 0.0f,  0.0f), // 接地
		// AO 確認用に近接配置した2つ。中心間 1.2 なので隙間は 0.2 ユニット
		glm::vec3( 0.0f, 0.0f,  2.5f),
		glm::vec3( 1.2f, 0.0f,  2.5f),
		// 積み重ね。上下の cube の境目にも AO が出る
		glm::vec3(-1.0f, 1.0f, -1.0f),
		glm::vec3( 0.0f, 0.0f, -20.0f), // z=-25 壁の前
		glm::vec3( 0.0f, 0.0f,  20.0f), // z=+25 壁の前
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
	// === LearnOpenGL の SSAO の章に合わせた設定 ===
	//
	// あちらは lightColor = (0.2, 0.2, 0.7)、linear = 0.09、quadratic = 0.032 の
	// 点光源1灯だけという、かなり控えめな構成になっている。SSAO を見せるのが目的なので
	// 直接光を弱くし、環境光の割合を大きく取っているため。
	//
	// ここでは配色を残したいので4灯のままだが、強度と減衰を同じ水準に合わせている。
	// diffuse が 1 を大きく超えないため Bloom もほとんど発火せず、AO が滲みで消えない。
	//
	// ambient がすべてゼロなのは、環境光を「ライトごと」ではなく
	// 「シーン全体で1つの定数（ambientStrength_）」に一本化したため。
	// 詳細は deferred_lighting.frag の ambientStrength のコメントを参照。
	const std::array<gl::PointLight, 4> pointLights_ = {{
		// position,                    ambient,          diffuse,                       specular,                      constant, linear, quadratic
		{glm::vec3( 0.0f, 2.0f,  2.2f), glm::vec3(0.0f), glm::vec3(0.7f,  0.7f,  0.7f), glm::vec3(0.7f,  0.7f,  0.7f), 1.0f, 0.09f, 0.032f},
		{glm::vec3(-5.0f, 0.8f, -4.0f), glm::vec3(0.0f), glm::vec3(0.7f,  0.2f,  0.2f), glm::vec3(0.7f,  0.2f,  0.2f), 1.0f, 0.09f, 0.032f},
		{glm::vec3( 4.2f, 3.0f,  1.8f), glm::vec3(0.0f), glm::vec3(0.2f,  0.2f,  0.7f), glm::vec3(0.2f,  0.2f,  0.7f), 1.0f, 0.09f, 0.032f},
		{glm::vec3(-1.5f, 3.0f, -2.2f), glm::vec3(0.0f), glm::vec3(0.2f,  0.6f,  0.3f), glm::vec3(0.2f,  0.6f,  0.3f), 1.0f, 0.09f, 0.032f}
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

