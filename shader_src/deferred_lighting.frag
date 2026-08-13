#version 460 core
out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
// 1.0=遮蔽なし, 0.0=完全遮蔽
uniform sampler2D ssao;

struct PointLight {
    vec3 position;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    // PointLight::calcRadius() が減衰式から逆算して送ってくる
    float radius;
};

const int NR_LIGHTS = 4;
uniform PointLight pointLights[NR_LIGHTS];
uniform vec3 viewPos;
uniform samplerCube shadowMap[NR_LIGHTS];
// ガラスを透過した光の色。ガラスを通らない方向は白
uniform samplerCube shadowColor[NR_LIGHTS];
uniform float farPlane;

// ライトごとに持たせると光源から離れるほど AO を掛ける対象が消えるため定数に一本化
uniform float ambientStrength;

// 対応表は main.cpp の kDebugModes。hdr.frag の debugRawOutput も要有効
uniform int debugMode;

uniform float ssaoStrength;

const float PI = 3.14159265359;

// ambient は扱わない。この関数が返すのはこの光源からの直接光だけ
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir,
    vec3 albedo, float roughness, float metallic, vec3 F0,
    float shadow);
float ShadowCalculation(vec3 fragPos, vec3 normal, vec3 lightDir, vec3 lightPos,
    samplerCube shadowMap);

void main() {
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 Normal = texture(gNormal, TexCoords).rgb;
    vec3 Albedo = texture(gAlbedoSpec, TexCoords).rgb;
    float Roughness = texture(gAlbedoSpec, TexCoords).a;
    float Metallic = texture(gNormal, TexCoords).a;

    if (debugMode == 0) {
        // ---- 通常のライティング ----
        float AmbientOcclusion =
            mix(1.0, texture(ssao, TexCoords).r, ssaoStrength);

        vec3 viewDir = normalize(viewPos - FragPos);

        // 非金属は一律 0.04、金属は鏡面反射がアルベドの色を持つ
        vec3 F0 = mix(vec3(0.04), Albedo, Metallic);

        // 環境光はループの外で1回だけ。距離減衰を掛けないので遠くでも AO が効く
        vec3 result = ambientStrength * Albedo * AmbientOcclusion;

        for (int i = 0; i < NR_LIGHTS; ++i) {
            // 26回サンプリングする ShadowCalculation() の手前で弾く
            float dist = length(pointLights[i].position - FragPos);
            if (dist >= pointLights[i].radius)
                continue;

            vec3 lightDir = normalize(pointLights[i].position - FragPos);
            float shadow = ShadowCalculation(FragPos, Normal, lightDir,
                    pointLights[i].position, shadowMap[i]);
            // 窓枠は shadow≈1 で黒い影、ガラスは shadow=0 のままここで色付きに減衰する
            vec3 transmit =
                texture(shadowColor[i], FragPos - pointLights[i].position).rgb;
            result += transmit * CalcPointLight(pointLights[i], Normal, FragPos, viewDir,
                        Albedo, Roughness, Metallic, F0, shadow);
        }

        FragColor = vec4(result, 1.0);

        float brightness = dot(result, vec3(0.2126, 0.7152, 0.0722));
        if (brightness > 1.0)
            BrightColor = vec4(result, 1.0);
        else
            BrightColor = vec4(0.0, 0.0, 0.0, 1.0);

        return;
    }

    // ---- デバッグ表示 ----
    // Bloom が乗ると判定できなくなるので、デバッグ中は BrightColor を常に黒にする
    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);

    if (debugMode == 1) {
        // 必ず1灯だけで見ること。4灯を max() でまとめるとほぼ全面が白くなり判定できない
        vec3 lightDir0 = normalize(pointLights[0].position - FragPos);
        float shadow0 = ShadowCalculation(FragPos, Normal, lightDir0,
                pointLights[0].position, shadowMap[0]);
        FragColor = vec4(vec3(shadow0), 1.0);
    } else if (debugMode == 2) {
        vec3 fragToLight0 = FragPos - pointLights[0].position;
        float closest = texture(shadowMap[0], fragToLight0).r;
        FragColor = vec4(vec3(closest), 1.0);
    } else if (debugMode == 3) {
        FragColor = vec4(Albedo, 1.0);
    } else if (debugMode == 4) {
        FragColor = vec4(Normal * 0.5 + 0.5, 1.0);
    } else if (debugMode == 5) {
        FragColor = vec4(abs(FragPos) / farPlane, 1.0);
    } else if (debugMode == 6) {
        // 4分割して G-Buffer とシャドウマップを同時に見る
        vec2 uv = TexCoords * 2.0;
        vec2 quad = floor(uv); // (0,0)=左下 (1,0)=右下 (0,1)=左上 (1,1)=右上
        vec2 localUV = fract(uv);

        vec3 qPos = texture(gPosition, localUV).rgb;
        vec3 qNormal = texture(gNormal, localUV).rgb;
        vec4 qAlbedo = texture(gAlbedoSpec, localUV);

        vec3 debugColor;
        if (quad.y > 0.5 && quad.x < 0.5)
            debugColor = qAlbedo.rgb; // 左上: Albedo
        else if (quad.y > 0.5)
            debugColor = qNormal * 0.5 + 0.5; // 右上: Normal
        else if (quad.x < 0.5)
            debugColor = abs(qPos) / 25.0; // 左下: Position
        else {
            // 右下: shadowMap[0] の生の深度値
            vec3 dir = qPos - pointLights[0].position;
            debugColor = vec3(texture(shadowMap[0], dir).r);
        }

        if (abs(TexCoords.x - 0.5) < 0.001 || abs(TexCoords.y - 0.5) < 0.001)
            debugColor = vec3(1.0, 0.0, 0.0);

        FragColor = vec4(debugColor, 1.0);
    } else if (debugMode == 7) {
        // 真っ黒なら FBO・サンプラー・パスの配線が繋がっていない
        float ao = texture(ssao, TexCoords).r;
        FragColor = vec4(vec3(ao), 1.0);
    } else if (debugMode == 8) {
        // 白地にガラスのシルエットが色付きで写れば配線は正しい
        vec3 dir0 = FragPos - pointLights[0].position;
        FragColor = vec4(texture(shadowColor[0], dir0).rgb, 1.0);
    } else if (debugMode == 9) {
        // オブジェクトごとに一様な明るさで見えれば gNormal.a への書き込みは正しい
        FragColor = vec4(vec3(Metallic), 1.0);
    } else if (debugMode == 10) {
        FragColor = vec4(vec3(Roughness), 1.0);
    } else {
        FragColor = vec4(1.0, 0.0, 1.0, 1.0); // 未定義の debugMode（マゼンタ）
    }
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    // 式中の α は roughness そのものではなく roughness の二乗
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = NdotH2 * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

// 光が届く確率と、反射光が見える確率の積
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdotL, roughness) *
        GeometrySchlickGGX(NdotV, roughness);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir,
    vec3 albedo, float roughness, float metallic, vec3 F0,
    float shadow) {
    vec3 lightDir = normalize(light.position - fragPos);
    vec3 halfwayDir = normalize(viewDir + lightDir);

    // 逆二乗減衰。変更したら PointLight::calcRadius() も必ず合わせる
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance = light.diffuse * attenuation;

    float NDF = DistributionGGX(normal, halfwayDir, roughness);
    float G = GeometrySmith(normal, viewDir, lightDir, roughness);
    vec3 F = fresnelSchlick(max(dot(halfwayDir, viewDir), 0.0), F0);

    // 真横から見たときのゼロ除算を避ける
    float denominator = 4.0 * max(dot(normal, viewDir), 0.0) *
            max(dot(normal, lightDir), 0.0) +
            0.0001;
    vec3 specular = (NDF * G * F) / denominator;

    // 反射に回らなかったぶんが拡散へ。金属は拡散反射を持たない
    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);

    float NdotL = max(dot(normal, lightDir), 0.0);

    // 直接光の遮蔽はシャドウマップが担当する（SSAO ではない）
    return (1.0 - shadow) * (kD * albedo / PI + specular) * radiance * NdotL;
}

float ShadowCalculation(vec3 fragPos, vec3 normal, vec3 lightDir, vec3 lightPos,
    samplerCube shadowMap) {
    // samplerCube は方向でテクセルを解決するので正規化しない
    vec3 fragToLight = fragPos - lightPos;
    float currentDepth = length(fragToLight);
    // 角度に応じて可変にするスロープバイアス（shadow acne 対策）
    float bias = max(0.15 * (1.0 - dot(normal, lightDir)), 0.05);

    // PCF: 26方向サンプリング
    float shadow = 0.0;
    // UV空間ではなく方向ベクトル空間でのオフセット量なので固定値
    float offset = 0.05;

    vec3 sampleOffsetDirections[26] =
        vec3[](vec3(1, 1, 1), vec3(1, -1, 1), vec3(-1, -1, 1), vec3(-1, 1, 1),
            vec3(1, 1, -1), vec3(1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
            vec3(1, 1, 0), vec3(1, -1, 0), vec3(-1, -1, 0), vec3(-1, 1, 0),
            vec3(1, 0, 1), vec3(-1, 0, 1), vec3(1, 0, -1), vec3(-1, 0, -1),
            vec3(0, 1, 1), vec3(0, -1, 1), vec3(0, -1, -1), vec3(0, 1, -1),
            vec3(1, 0, 0), vec3(-1, 0, 0), vec3(0, 1, 0), vec3(0, -1, 0),
            vec3(0, 0, 1), vec3(0, 0, -1));
    for (int i = 0; i < 26; ++i) {
        float closestDepth =
            texture(shadowMap, fragToLight + sampleOffsetDirections[i] * offset).r;
        closestDepth *= farPlane;
        if (currentDepth - bias > closestDepth)
            shadow += 1.0;
    }
    return shadow / 26.0;
}
