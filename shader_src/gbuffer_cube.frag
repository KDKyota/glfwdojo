#version 460 core

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec4 gNormal; // a = metallic
layout (location = 2) out vec4 gAlbedoRoughness;


in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec3 TangentLightPos;
in vec3 TangentViewPos;
in vec3 TangentFragPos;
in mat3 TBNtoWorld;

uniform sampler2D diffuseMap;
uniform sampler2D normalMap;
uniform sampler2D heightMap;

uniform float heightScale;
uniform float metallic;
uniform float roughness;

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
	gNormal = vec4(normal, metallic);

	gAlbedoRoughness.rgb = diffuseColor.rgb;
	gAlbedoRoughness.a = roughness;
}


// 視線レイを層に分割し、ハイトマップの深さを追い越した層の座標を返す
vec2 SteepParallaxMapping(vec2 texCoords, vec3 viewDir)
{
	// 斜めから見るほど階段状のアーティファクトが目立つのでレイヤー数を増やす
	const float minLayers = 8.0;
	const float maxLayers = 32.0;
	float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));

	// 1レイヤー分の深さと、現在調べているレイヤーの累積深度（0.0〜1.0）
	float layerDepth = 1.0 / numLayers;
	float currentLayerDepth = 0.0;

	// viewDir.xy / viewDir.z が「奥に1進んだときの横方向の移動量」
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

// 衝突した層と1つ手前の層を線形補間し、階段状のアーティファクトを滑らかにする
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

	// ハイトマップの深さとレイヤーの深さの差。負なら表面より奥
	float afterDepth = currentDepthMapValue - currentLayerDepth;
	float beforeDepth = texture(heightMap, prevTexCoords).r - currentLayerDepth + layerDepth;

	// 2つの深度差の比率から、実際の交点に近いテクスチャ座標を線形補間で求める
	float weight = afterDepth / (afterDepth - beforeDepth);
	vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

	return finalTexCoords;
}
