#version 330 core

out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

struct PointLight {
    vec3 position;
    float constant;
    float linear;
    float quadratic;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct Material {
    float shininess;
};

#define NR_POINT_LIGHTS 4

in vec3 FragPos;
in vec2 TexCoords;
in mat3 TBN;

uniform sampler2D texture1;
uniform sampler2D normalMap;
// point shadow 用のデプスキューブマップ（各テクセルには光源からの正規化距離 [0,1] が入っている）。4灯ぶん
uniform samplerCube shadowMap[NR_POINT_LIGHTS];
// shadowMap に書き込まれた正規化距離を実距離スケールに戻すための基準値
uniform float farPlane;

uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform Material material;
uniform vec3 viewPos;

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, float shadow);
float ShadowCalculation(vec3 fragPos, vec3 normal, vec3 lightDir, vec3 lightPos, samplerCube shadowMap);

void main()
{
    // ノーマルマップをサンプルしてタンジェント空間 [-1,1] に変換
    vec3 normal = texture(normalMap, TexCoords).rgb;
    normal = normalize(normal * 2.0 - 1.0);
    // TBN でタンジェント空間からワールド空間へ変換
    normal = normalize(TBN * normal);

    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 result = vec3(0.0);
    for (int i = 0; i < NR_POINT_LIGHTS; i++)
    {
        vec3 lightDir = normalize(pointLights[i].position - FragPos);
        float shadow = ShadowCalculation(FragPos, normal, lightDir, pointLights[i].position, shadowMap[i]);
        result += CalcPointLight(pointLights[i], normal, FragPos, viewDir, shadow);
    }

    vec4 texColor = texture(texture1, TexCoords);
    FragColor = vec4(result, texColor.a);

    float brightness = dot(result, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 1.0)
        BrightColor = vec4(result, 1.0);
    else
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, float shadow)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * vec3(texture(texture1, TexCoords));

    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * vec3(texture(texture1, TexCoords));

    vec3 ambient = light.ambient * vec3(texture(texture1, TexCoords));

    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;

    return ambient + (1.0 - shadow) * (diffuse + specular);
}

float ShadowCalculation(vec3 fragPos, vec3 normal, vec3 lightDir, vec3 lightPos, samplerCube shadowMap)
{
    // 光源からフラグメントへの方向ベクトル。samplerCube はUV座標ではなく
    // この「方向」でどの面のどのテクセルかを解決するため、正規化せずそのまま使う
    // ※ normal はノーマルマップ適用後（TBN変換済み）のワールド空間法線が渡ってくる
    vec3 fragToLight = fragPos - lightPos;
    // 光源からフラグメントまでの実距離（比較の基準値。shadowMap側の値と同じスケールにする）
    float currentDepth = length(fragToLight);
    // 法線とライト方向の角度に応じて可変にするスロープバイアス（shadow acne 対策）
    float bias = max(0.15 * (1.0 - dot(normal, lightDir)), 0.05);

    float shadow = 0.0;
    // sampleOffsetDirections を掛ける係数。UV空間ではなく方向ベクトル空間でのオフセット量
    float offset = 0.05;
    // {-1,0,1}^3 から中心(0,0,0)を除いた26方向すべて（頂点8+辺の中点12+面の中心6、PCF用）
    // サンプル数を増やすほど shadow/26.0 が取り得る段階数が増え、グラデーションが細かくなる
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
