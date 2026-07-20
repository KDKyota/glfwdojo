#version 330 core

out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

struct Material {
	vec3 ambient; // 環境光の影響
	sampler2D diffuse;
	// ふつうの物体はambientとdiffuseは同じ色
	sampler2D specular;
	float shininess;
};

struct DirLight {
	vec3 direction;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

struct PointLight {
	vec3 position;

	float constant;
	float linear;
	float quadratic;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

struct SpotLight {
	vec3 position;
	vec3 direction;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;

	// Attenuation(減衰)の値
	float constant;
	float linear;
	float quadratic;
	// スポットライトの角度
	float cutOff;
	float outerCutOff;
};

#define NR_POINT_LIGHTS 1 

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec3 TangentLightPos;
in vec3 TangentViewPos;
in vec3 TangentFragPos;

uniform sampler2D diffuseMap; // 画像テクスチャ
// point shadow 用のデプスキューブマップ（各テクセルには光源からの正規化距離 [0,1] が入っている）
uniform samplerCube shadowMap;
uniform sampler2D normalMap;
uniform sampler2D heightMap;
// shadowMap に書き込まれた正規化距離を実距離スケールに戻すための基準値
uniform float farPlane;

uniform DirLight dirLight;
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform SpotLight spotLight;
uniform Material material;

// lightPos と viewPos は.vertで計算
//uniform vec3 lightPos; // 光源の位置
//uniform vec3 viewPos; // カメラの位置

uniform float heightScale;

//function
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, float shadow);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewdir);
float ShadowCalculation(vec3 fragPos, vec3 normal, vec3 lightDir);
vec2 SteepParallaxMapping(vec2 texCoords, vec3 viewDir);
vec2 ParallaxOcclusionMapping(vec2 texCoords, vec3 viewDir);

void main()
{
	vec3 viewDir = normalize(TangentViewPos - TangentFragPos);
	vec4 texColor = texture(diffuseMap, TexCoords);
    vec2 texCoords = ParallaxOcclusionMapping(TexCoords, viewDir);
	if(texCoords.x > 1.0 || texCoords.y > 1.0 || texCoords.x < 0.0 || texCoords.y < 0.0)
        discard;
    vec3 TangentNormal = texture(normalMap, texCoords).rgb;
    TangentNormal = normalize(TangentNormal * 2.0 - 1.0);

    vec3 color = texture(diffuseMap, texCoords).rgb;
	// 1. directional lighting
	//vec3 result = CalcDirLight(dirLight, norm, viewDir);

	// 2. point lighting
    vec3 lightDir = normalize(TangentLightPos - TangentFragPos);
    float shadow = ShadowCalculation(FragPos, TangentNormal, lightDir);
	vec3 result = vec3(0.0f);
    vec3 normal = TangentNormal;
	for(int i = 0; i < NR_POINT_LIGHTS; i++)
	{
	  // CalcPointLight は light.position を内部で fragPos と比較するので、
	  // normal(タンジェント空間)と揃えるために、この呼び出し専用に position をタンジェント空間へ置き換えたコピーを渡す
	  // （TBNは直交行列なので、同じTBNで変換した2点間の距離・内積はワールド空間と同じ値になる）
	  PointLight tangentLight = pointLights[i];
	  tangentLight.position = TangentLightPos;
	  result += CalcPointLight(tangentLight, normal, TangentFragPos, viewDir, shadow);
	}
	// 3. spot lighting
	// result += CalcSpotLight(spotLight, norm, FragPos, viewDir);

	FragColor = vec4(result, 1.0);

	float brightness = dot(result, vec3(0.2126, 0.7152, 0.0722));
	if(brightness > 1.0)
		BrightColor = vec4(result, 1.0);
	else
		BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}

// DirLightの計算関数
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
      vec3 lightDir = normalize(-light.direction);
      // Diffuse
      float diff = max(dot(normal, lightDir), 0.0);
      vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoords));
      // Specular
      vec3 reflectDir = reflect(-lightDir, normal);
      float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
      vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
      // Combine results
      vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));
      return (ambient + diffuse + specular);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, float shadow)
{
      vec3 lightDir = normalize(light.position - fragPos);
      // Diffuse
      float diff = max(dot(normal, lightDir), 0.0);
      //vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoords));
      vec3 diffuse = light.diffuse * diff * vec3(texture(diffuseMap, TexCoords));
      // Specular
      vec3 reflectDir = reflect(-lightDir, normal);
      float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
      //vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
      vec3 specular = light.specular * spec * vec3(texture(diffuseMap, TexCoords));

      // Combine results
      //vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));
      vec3 ambient = light.ambient * vec3(texture(diffuseMap, TexCoords));

      // attenuation
      float distance = length(light.position - fragPos);
      float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

      ambient *= attenuation;
      diffuse *= attenuation;
      specular *= attenuation;
      // 返し値にshadowを考慮
      return (ambient + (1.0f - shadow) * (diffuse + specular));
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir   )
{
      vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    // spotlight intensity
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    // combine results
    vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    return (ambient + diffuse + specular);
}

float ShadowCalculation(vec3 fragPos, vec3 normal, vec3 lightDir)
{
    // 光源からフラグメントへの方向ベクトル。samplerCube はUV座標ではなく
    // この「方向」でどの面のどのテクセルかを解決するため、正規化せずそのまま使う
    vec3 fragToLight = fragPos - pointLights[0].position;
    // 光源からフラグメントまでの実距離（比較の基準値。shadowMap側の値と同じスケールにする）
    float currentDepth = length(fragToLight);
    // 法線とライト方向の角度に応じて可変にするスロープバイアス（shadow acne 対策）
    float bias = max(0.15 * (1.0 - dot(normal, lightDir)), 0.05);

    // PCF: 26方向サンプリング
    float shadow = 0.0;
    // sampleOffsetDirections を掛ける係数。directional版のtexelSizeに相当するが、
    // UV空間ではなく方向ベクトル空間でのオフセット量なので固定値になっている
    float offset = 0.05;
    // {-1,0,1}^3 から中心(0,0,0)を除いた26方向すべて
    // （立方体の頂点8+辺の中点12+面の中心6。方向ベクトルを少しずつ散らして周辺テクセルをサンプリングし、
    //   影の縁をぼかす。サンプル数を増やすほど shadow/26.0 が取り得る段階数が増え、グラデーションが細かくなる）
    vec3 sampleOffsetDirections[26] = vec3[]
    (
        vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1),
        vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
        vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
        vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
        vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1),
        vec3( 1,  0,  0), vec3(-1,  0,  0), vec3( 0,  1,  0), vec3( 0, -1,  0),
        vec3( 0,  0,  1), vec3( 0,  0, -1)
    );
    for(int i = 0; i < 26; ++i)
    {
        // 方向ベクトルを少し揺らしてサンプリングした、光源から見た「最も近い遮蔽物」までの距離
        float closestDepth = texture(shadowMap, fragToLight + sampleOffsetDirections[i] * offset).r;
        // [0,1] 正規化されていた値を farPlane 倍して実距離スケールに戻す
        closestDepth *= farPlane;
        if(currentDepth - bias > closestDepth)
            shadow += 1.0;
    }
    return shadow / 26.0;
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

