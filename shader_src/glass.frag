// 透過窓のガラス部分を前方描画するシェーダー。
// 窓枠は gbuffer_window.frag が Deferred 側で描くので、ここでは alpha >= 0.5 を discard する。
//
// サンプラー配列をループ変数で添字するため 4.60 が必要。330 以前は定数式のみ許され、
// Mesa などではコンパイルエラーになる（NVIDIA/AMD は黙って通してしまう）
#version 460 core

out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

// 元は sampler2D diffuse / sampler2D specular をメンバに持っていたが、
// WSL の Mesa d3d12 は struct のメンバに sampler があるだけで DXIL 変換時に
// segfault する（使っていなくても落ちる）ので、struct から sampler を外している
struct Material
{
    vec3 ambient;    // 環境光の影響
    float shininess; // C++側から material.shininess として設定される
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

#define NR_POINT_LIGHTS 4

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D texture1;
// 各テクセルに光源からの正規化距離 [0,1] が入る
uniform samplerCube shadowMap[NR_POINT_LIGHTS];
// ガラスを透過した光の色。deferred_lighting.frag と同じマップを共有する
uniform samplerCube shadowColor[NR_POINT_LIGHTS];
uniform sampler2D ssao;
// shadowMap に書き込まれた正規化距離を実距離スケールに戻すための基準値
uniform float farPlane;

uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform Material material;

uniform vec3 viewPos; // カメラの位置

// deferred_lighting.frag と同じ値を受け取り、不透明面と透過窓で扱いを揃える
uniform float ambientStrength;
uniform bool reflectionPass; // 透過と反射を切り替えられるようにする

// function
// ambient は扱わない。この関数が返すのはこの光源からの直接光だけ
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, float shadow);
float ShadowCalculation(vec3 fragPos, vec3 normal, vec3 lightDir, vec3 lightPos, samplerCube shadowMap);

void main()
{
    // Diffuse
    vec3 normal = normalize(Normal);

    if (!gl_FrontFacing)
        normal = -normal;

    vec2 screenUV = gl_FragCoord.xy / vec2(textureSize(ssao, 0));
    float ao = texture(ssao, screenUV).r;

    vec3 viewDir = normalize(viewPos - FragPos);

    vec4 texColor = texture(texture1, TexCoords);
    // transparent windowのガラス部分だけをレンダリングする
    if (texColor.a >= 0.5 || texColor.a < 0.01)
        discard;

    const vec3 F0 = vec3(0.04);
    float cosTheta = max(dot(normal, viewDir), 0.0);
    vec3 fresnel = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
    // 環境光はループの外で1回だけ。距離減衰を掛けないので光源から遠くても効く
    // vec3 result = ambientStrength * texColor.rgb * ao;

    if (reflectionPass) // 反射(足し算)
    {
        vec3 reflected = vec3(0.0);
        // 2. point lighting
        for (int i = 0; i < NR_POINT_LIGHTS; i++)
        {
            vec3 lightDir = normalize(pointLights[i].position - FragPos);
            float shadow = ShadowCalculation(FragPos, normal, lightDir, pointLights[i].position, shadowMap[i]);
            // 自分自身の透過色も乗るのでスペキュラがガラスの色に少し染まる
            vec3 transmit = texture(shadowColor[i], FragPos - pointLights[i].position).rgb;
            reflected += transmit * CalcPointLight(pointLights[i], normal, FragPos, viewDir, shadow);
        }

        vec3 result = fresnel * reflected;

        float brightness = dot(result, vec3(0.2126, 0.7152, 0.0722));
        if (brightness > 1.0)
            BrightColor = vec4(result, 1.0);
        else
            BrightColor = vec4(0.0);

        FragColor = vec4(result, 0.0);
    }
    else // 透過(掛け算)
    {
        BrightColor = vec4(1.0);

        vec3 transmittance = mix(vec3(1.0), texColor.rgb, texColor.a);

        transmittance *= (1.0 - fresnel);
        FragColor = vec4(transmittance, 1.0);
    }
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
    // vec3 specular = light.specular * spec * vec3(texture(texture1, TexCoords));
    vec3 specular = light.specular * spec;

    // deferred_lighting.frag と同じ式にすること。片方だけ変えると不透明物と食い違う
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (distance * distance);

    diffuse *= attenuation * 0.1;
    specular *= attenuation;
    // 直接光の遮蔽はシャドウマップが担当する
    // return (1.0f - shadow) * (diffuse + specular);
    return (1.0f - shadow) * specular;
}

float ShadowCalculation(vec3 fragPos, vec3 normal, vec3 lightDir, vec3 lightPos, samplerCube shadowMap)
{
    // samplerCube は方向でテクセルを解決するので正規化しない
    vec3 fragToLight = fragPos - lightPos;
    float currentDepth = length(fragToLight);
    // 角度に応じて可変にするスロープバイアス（shadow acne 対策）
    float bias = max(0.15 * (1.0 - dot(normal, lightDir)), 0.05);

    // PCF: 26方向サンプリング
    float shadow = 0.0;
    // UV空間ではなく方向ベクトル空間でのオフセット量なので固定値
    float offset = 0.05;
    // {-1,0,1}^3 から中心を除いた26方向
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
