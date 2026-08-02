// サンプラー配列をループ変数で添字する（shadowMap[i]）ため 4.60 が必要。
// GLSL 3.30 以前の仕様ではサンプラー配列は定数式でしか添字できず、Mesa
// など仕様に厳密な ドライバではコンパイルエラーになる（NVIDIA/AMD は 330
// でも黙って通してしまう）。
#version 460 core

out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

// 元は sampler2D diffuse / sampler2D specular をメンバに持っていたが、
// WSL の Mesa d3d12 ドライバは「struct のメンバに sampler がある」だけで
// シェーダーのDXIL変換時に segfault する（実際に使っていなくても落ちる）。
// texture1 を直接サンプルする形にして、struct からは sampler を外している。
struct Material
{
    vec3 ambient;    // 環境光の影響
    float shininess; // C++側から material.shininess として設定される
};

struct DirLight
{
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight
{
    vec3 position;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight
{
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

#define NR_POINT_LIGHTS 4

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D texture1;
// point shadow 用のデプスキューブマップ（各テクセルには光源からの正規化距離
// [0,1] が入っている）。4灯ぶん
uniform samplerCube shadowMap[NR_POINT_LIGHTS];
// shadowMap に書き込まれた正規化距離を実距離スケールに戻すための基準値
uniform float farPlane;

uniform DirLight dirLight;
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform SpotLight spotLight;
uniform Material material;

uniform vec3 lightPos; // 光源の位置
uniform vec3 viewPos;  // カメラの位置

// シーン全体にかかる環境光。deferred_lighting.frag と同じ値を受け取る。
// ライトごとの ambient を使うと attenuation
// が掛かって光源から離れるほど消えるため、
// 不透明面（Deferred）と透過窓（前方描画）で扱いを揃えている
uniform float ambientStrength;

// function
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
// ambient は扱わない。この関数が返すのはこの光源からの直接光だけ
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, float shadow);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewdir);
float ShadowCalculation(vec3 fragPos, vec3 normal, vec3 lightDir, vec3 lightPos, samplerCube shadowMap);

void main()
{
    // Diffuse
    vec3 normal = normalize(Normal);

    if (!gl_FrontFacing)
        normal = -normal;
    vec3 viewDir = normalize(viewPos - FragPos);

    // 1. directional lighting
    // directional lightは今は使わないのでコメントアウト
    // vec3 result = CalcDirLight(dirLight, norm, viewDir);
    vec4 texColor = texture(texture1, TexCoords);
    // transparent windowのガラス部分だけをレンダリングする
    if (texColor.a >= 0.5)
        discard;

    // 環境光はループの外で1回だけ。距離減衰を掛けないので光源から遠くても効く
    vec3 result = ambientStrength * texColor.rgb;

    // 2. point lighting
    for (int i = 0; i < NR_POINT_LIGHTS; i++)
    {
        vec3 lightDir = normalize(pointLights[i].position - FragPos);
        float shadow = ShadowCalculation(FragPos, normal, lightDir, pointLights[i].position, shadowMap[i]);
        result += CalcPointLight(pointLights[i], normal, FragPos, viewDir, shadow);
    }
    // 3. spot lighting
    // result += CalcSpotLight(spotLight, norm, FragPos, viewDir);

    FragColor = vec4(result, texColor.a);

    float brightness = dot(result, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 1.0)
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
    vec3 diffuse = light.diffuse * diff * vec3(texture(texture1, TexCoords));
    // Specular
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * vec3(texture(texture1, TexCoords));
    // Combine results
    // 今回はambientStrengthを用いるのでライトによるambientは必要ない
    // vec3 ambient = light.ambient * vec3(texture(texture1, TexCoords));
    // return (ambient + diffuse + specular);
    return (diffuse + specular);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, float shadow)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * vec3(texture(texture1, TexCoords));
    // Specular
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * vec3(texture(texture1, TexCoords));

    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    diffuse *= attenuation;
    specular *= attenuation;
    // 直接光の遮蔽はシャドウマップが担当する
    return (1.0f - shadow) * (diffuse + specular);
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
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
    // vec3 ambient = light.ambient * vec3(texture(texture1, TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(texture1, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(texture1, TexCoords));
    // ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    // return (ambient + diffuse + specular);
    return (diffuse + specular);
}

float ShadowCalculation(vec3 fragPos, vec3 normal, vec3 lightDir, vec3 lightPos, samplerCube shadowMap)
{
    // 光源からフラグメントへの方向ベクトル。samplerCube はUV座標ではなく
    // この「方向」でどの面のどのテクセルかを解決するため、正規化せずそのまま使う
    vec3 fragToLight = fragPos - lightPos;
    // 光源からフラグメントまでの実距離（比較の基準値。shadowMap側の値と同じスケールにする）
    float currentDepth = length(fragToLight);
    // 法線とライト方向の角度に応じて可変にするスロープバイアス（shadow acne
    // 対策）
    float bias = max(0.15 * (1.0 - dot(normal, lightDir)), 0.05);

    // PCF: 26方向サンプリング
    float shadow = 0.0;
    // sampleOffsetDirections を掛ける係数。directional版のtexelSizeに相当するが、
    // UV空間ではなく方向ベクトル空間でのオフセット量なので固定値になっている
    float offset = 0.05;
    // {-1,0,1}^3 から中心(0,0,0)を除いた26方向すべて
    // （立方体の頂点8+辺の中点12+面の中心6。方向ベクトルを少しずつ散らして周辺テクセルをサンプリングし、
    //   影の縁をぼかす。サンプル数を増やすほど shadow/26.0
    //   が取り得る段階数が増え、グラデーションが細かくなる）
    vec3 sampleOffsetDirections[26] = vec3[](
        vec3(1, 1, 1), vec3(1, -1, 1), vec3(-1, -1, 1), vec3(-1, 1, 1), vec3(1, 1, -1), vec3(1, -1, -1),
        vec3(-1, -1, -1), vec3(-1, 1, -1), vec3(1, 1, 0), vec3(1, -1, 0), vec3(-1, -1, 0), vec3(-1, 1, 0),
        vec3(1, 0, 1), vec3(-1, 0, 1), vec3(1, 0, -1), vec3(-1, 0, -1), vec3(0, 1, 1), vec3(0, -1, 1), vec3(0, -1, -1),
        vec3(0, 1, -1), vec3(1, 0, 0), vec3(-1, 0, 0), vec3(0, 1, 0), vec3(0, -1, 0), vec3(0, 0, 1), vec3(0, 0, -1));
    for (int i = 0; i < 26; ++i)
    {
        // 方向ベクトルを少し揺らしてサンプリングした、光源から見た「最も近い遮蔽物」までの距離
        float closestDepth = texture(shadowMap, fragToLight + sampleOffsetDirections[i] * offset).r;
        // [0,1] 正規化されていた値を farPlane 倍して実距離スケールに戻す
        closestDepth *= farPlane;
        if (currentDepth - bias > closestDepth)
            shadow += 1.0;
    }
    return shadow / 26.0;
}
