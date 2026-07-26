#version 460 core

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;


in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec3 TangentLightPos;
in vec3 TangentViewPos;
in vec3 TangentFragPos;
in mat3 TBNtoWorld;

uniform sampler2D diffuseMap; // 画像テクスチャ
// point shadow 用のデプスキューブマップ（各テクセルには光源からの正規化距離 [0,1] が入っている）
uniform sampler2D normalMap;
uniform sampler2D heightMap;
// shadowMap に書き込まれた正規化距離を実距離スケールに戻すための基準値

uniform float heightScale;

//function
vec2 ParallaxOcclusionMapping(vec2 texCoords, vec3 viewDir);

void main()
{
	vec3 tangentViewDir = normalize(TangentViewPos - TangentFragPos);
	//vec4 texColor = texture(diffuseMap, TexCoords);
	vec2 texCoords = ParallaxOcclusionMapping(TexCoords, tangentViewDir);
	if(texCoords.x > 1.0 || texCoords.y > 1.0 || texCoords.x < 0.0 || texCoords.y < 0.0)
		discard;
	vec3 TangentNormal = texture(normalMap, texCoords).rgb;
	TangentNormal = normalize(TangentNormal * 2.0 - 1.0);
	vec3 normal = normalize(TBNtoWorld * TangentNormal); // ワールド空間の法線に戻す

	vec4 diffuseColor = texture(diffuseMap, texCoords);

	gPosition = FragPos;
	gNormal = normal;

	gAlbedoSpec.rgb = texture(diffuseMap, texCoords).rgb;
	// 専用のspecularテクスチャが無いので、diffuseの輝度をスペキュラ強度の代用値にする
	gAlbedoSpec.a = dot(diffuseColor.rgb, vec3(0.2126, 0.7152, 0.0722));
}


// Steep Parallax Mapping
// 視線レイを numLayers 個の深度レイヤーに分割し、手前から奥へ1層ずつ進めながら
// 「レイヤーの深さ」と「ハイトマップが示す深さ」を比較し、レイが表面にぶつかった層のテクスチャ座標を返す
vec2 SteepParallaxMapping(vec2 texCoords, vec3 viewDir)
{
	// 視線が面に対して斜めになるほどレイヤー数を増やす
	// （真上から見るときは少ないレイヤーでも破綻しにくいが、斜めから見るほど階段状の見た目が目立ちやすいため）
	const float minLayers = 8.0;
	const float maxLayers = 32.0;
	float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));

	// 1レイヤー分の深さと、現在調べているレイヤーの累積深度（0.0〜1.0）
	float layerDepth = 1.0 / numLayers;
	float currentLayerDepth = 0.0;

	// 視線を1レイヤー分進めるごとに、テクスチャ座標をどれだけずらすか
	// （viewDir.xy / viewDir.z で「奥に1進んだときの横方向の移動量」を求め、heightScaleで強さを調整する）
	vec2 P = viewDir.xy / viewDir.z * heightScale;
	vec2 deltaTexCoords = P / numLayers;

	// 一番手前のレイヤーから探索を開始する
	vec2 currentTexCoords = texCoords;
	float currentDepthMapValue = texture(heightMap, currentTexCoords).r;

	// 「レイヤーの深さ」が「ハイトマップの深さ」を追い越すまで、視線を奥へ進めていく
	while (currentLayerDepth < currentDepthMapValue)
	{
		currentTexCoords -= deltaTexCoords;
		currentDepthMapValue = texture(heightMap, currentTexCoords).r;
		currentLayerDepth += layerDepth;
	}

	return currentTexCoords;
}

// Parallax Occlusion Mapping (POM)
// Steep Parallax Mapping で見つけた「衝突した層」と、その1つ手前の層の間を線形補間することで、
// レイヤーの粗さによる階段状のアーティファクトをさらに滑らかにする
vec2 ParallaxOcclusionMapping(vec2 texCoords, vec3 viewDir)
{
	const float minLayers = 8.0;
	const float maxLayers = 32.0;
	float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));

	float layerDepth = 1.0 / numLayers;
	float currentLayerDepth = 0.0;

	vec2 P = viewDir.xy / viewDir.z * heightScale;
	vec2 deltaTexCoords = P / numLayers;

	vec2 currentTexCoords = texCoords;
	float currentDepthMapValue = texture(heightMap, currentTexCoords).r;

	// ここまでは Steep Parallax Mapping と同じ探索処理
	while (currentLayerDepth < currentDepthMapValue)
	{
		currentTexCoords -= deltaTexCoords;
		currentDepthMapValue = texture(heightMap, currentTexCoords).r;
		currentLayerDepth += layerDepth;
	}

	// 衝突が検出される直前（1つ手前）のテクスチャ座標を復元する
	vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

	// 衝突後・衝突前それぞれについて「ハイトマップの深さ」と「レイヤーの深さ」の差を求める
	// （負なら表面より奥、正なら表面より手前を表す）
	float afterDepth = currentDepthMapValue - currentLayerDepth;
	float beforeDepth = texture(heightMap, prevTexCoords).r - currentLayerDepth + layerDepth;

	// 2つの深度差の比率から、実際の交点に近いテクスチャ座標を線形補間で求める
	float weight = afterDepth / (afterDepth - beforeDepth);
	vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

	return finalTexCoords;
}
