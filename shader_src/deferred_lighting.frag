#version 460 core
out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;
  
in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

struct PointLight {
	vec3 position;

	float constant;
	float linear;
	float quadratic;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

const int NR_LIGHTS = 4;
uniform PointLight pointLights[NR_LIGHTS];
uniform vec3 viewPos;
uniform samplerCube shadowMap[NR_LIGHTS];
uniform float farPlane;

// ==== デバッグ表示の切り替え ====
// 0 : 通常のライティング（本来の描画）
// 1 : ライト0のシャドウ判定だけを表示（黒=影でない / 白=影）
// 2 : shadowMap[0] の生の深度値を表示
//     全面黒   → samplerCube がキューブマップを読めていない（バインドかユニット割り当ての問題）
//     全面白   → デプスパスで何も描かれていない
//     階調あり → シャドウマップは正常。原因は ShadowCalculation の bias や farPlane 側
// 3 : G-Buffer の Albedo をそのまま表示（ジオメトリパスの確認用）
// 4 : G-Buffer の Normal を表示（[-1,1] を [0,1] に変換）
// 5 : G-Buffer の Position を表示（farPlane でスケールして [0,1] 付近に収める）
// 6 : 画面4分割で一度に確認（左上=Albedo 右上=Normal 左下=Position 右下=shadowMap[0]の生値）
#define DEBUG_MODE 0

// functions
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 albedo, float specularStrength, float shadow);
float ShadowCalculation(vec3 fragPos, vec3 normal, vec3 lightDir, vec3 lightPos, samplerCube shadowMap);

void main()
{             
    // retrieve data from G-buffer
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 Normal = texture(gNormal, TexCoords).rgb;
    vec3 Albedo = texture(gAlbedoSpec, TexCoords).rgb;
    float Specular = texture(gAlbedoSpec, TexCoords).a;
    
#if DEBUG_MODE == 0
    // ---- 通常のライティング ----
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 result = vec3(0.0);
    for(int i = 0; i < NR_LIGHTS; ++i)
    {
        vec3 lightDir = normalize(pointLights[i].position - FragPos);
		float shadow = ShadowCalculation(FragPos, Normal, lightDir, pointLights[i].position, shadowMap[i]);
        result += CalcPointLight(pointLights[i], Normal, FragPos, viewDir, Albedo, Specular, shadow);
    }

    FragColor = vec4(result, 1.0);

    // reflect bright color
	float brightness = dot(result, vec3(0.2126, 0.7152, 0.0722));
	if(brightness > 1.0)
		BrightColor = vec4(result, 1.0);
	else
		BrightColor = vec4(0.0, 0.0, 0.0, 1.0);

#else
    // ---- デバッグ表示 ----
    // Bloom が乗ると判定できなくなるので、デバッグ中は BrightColor を常に黒にする
    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);

  #if DEBUG_MODE == 1
    // ライト0のシャドウ判定だけを表示する。
    // 4灯まとめて max() を取ると、ライトに背を向けた面は必ず shadow=1 になるため
    // ほぼ全面が白くなってしまい判定に使えない。必ず1灯だけで見ること。
    vec3 lightDir0 = normalize(pointLights[0].position - FragPos);
    float shadow0 = ShadowCalculation(FragPos, Normal, lightDir0, pointLights[0].position, shadowMap[0]);
    FragColor = vec4(vec3(shadow0), 1.0);

  #elif DEBUG_MODE == 2
    // shadowMap[0] の生の深度値。ShadowCalculation を通さずに直接サンプルする。
    // 添字を定数 [0] にしているので、サンプラー配列の動的添字の問題も同時に切り分けられる。
    vec3 fragToLight0 = FragPos - pointLights[0].position;
    float closest = texture(shadowMap[0], fragToLight0).r;
    FragColor = vec4(vec3(closest), 1.0);

  #elif DEBUG_MODE == 3
    FragColor = vec4(Albedo, 1.0);

  #elif DEBUG_MODE == 4
    FragColor = vec4(Normal * 0.5 + 0.5, 1.0);

  #elif DEBUG_MODE == 5
    FragColor = vec4(abs(FragPos) / farPlane, 1.0);

  #elif DEBUG_MODE == 6
    // 画面を4分割し、G-Buffer の各要素とシャドウマップを同時に表示する。
    // 各象限では TexCoords を [0,1] に引き伸ばし直してサンプルするので、
    // どの象限にもシーン全体が縮小表示される。
    vec2 uv    = TexCoords * 2.0;
    vec2 quad  = floor(uv);   // (0,0)=左下 (1,0)=右下 (0,1)=左上 (1,1)=右上
    vec2 localUV = fract(uv);

    vec3  qPos    = texture(gPosition,   localUV).rgb;
    vec3  qNormal = texture(gNormal,     localUV).rgb;
    vec4  qAlbedo = texture(gAlbedoSpec, localUV);

    vec3 debugColor;
    if (quad.y > 0.5 && quad.x < 0.5)
        debugColor = qAlbedo.rgb;                    // 左上: Albedo（テクスチャ色が出れば正常）
    else if (quad.y > 0.5)
        debugColor = qNormal * 0.5 + 0.5;            // 右上: Normal（面ごとに色が変われば正常）
    else if (quad.x < 0.5)
        debugColor = abs(qPos) / 25.0;               // 左下: Position（位置に応じたグラデーションが出れば正常）
    else
    {
        // 右下: shadowMap[0] の生の深度値
        vec3 dir = qPos - pointLights[0].position;
        debugColor = vec3(texture(shadowMap[0], dir).r);
    }

    // 象限の境界に赤い線を引いて区切りを分かりやすくする
    if (abs(TexCoords.x - 0.5) < 0.001 || abs(TexCoords.y - 0.5) < 0.001)
        debugColor = vec3(1.0, 0.0, 0.0);

    FragColor = vec4(debugColor, 1.0);

  #else
    FragColor = vec4(1.0, 0.0, 1.0, 1.0); // 未定義のDEBUG_MODE（マゼンタ）
  #endif
#endif
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 albedo, float specularStrength, float shadow)
{
	vec3 lightDir = normalize(light.position - fragPos);
	// Diffuse
	float diff = max(dot(normal, lightDir), 0.0);
	//vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoords));
	vec3 diffuse = light.diffuse * diff * albedo;
	// Specular
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
	//vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
	vec3 specular = light.specular * spec * specularStrength;

	// Combine results
	//vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));
	vec3 ambient = light.ambient * albedo;

	// attenuation
	float distance = length(light.position - fragPos);
	float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

	ambient *= attenuation;
	diffuse *= attenuation; // 今のレンガcubeにはdiffuseとspecularは小さいほうが自然
	specular *= attenuation;
	// 返し値にshadowを考慮
	return (ambient + (1.0f - shadow) * (diffuse + specular));
}

float ShadowCalculation(vec3 fragPos, vec3 normal, vec3 lightDir, vec3 lightPos, samplerCube shadowMap)
{
    // 光源からフラグメントへの方向ベクトル。samplerCube はUV座標ではなく
    // この「方向」でどの面のどのテクセルかを解決するため、正規化せずそのまま使う
    vec3 fragToLight = fragPos - lightPos;
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
