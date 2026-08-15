// #include される側なので #version は書かない
// ライト関連の uniform はここが唯一の宣言場所。include する側では宣言しないこと
#ifndef SHADOW_COMMON_GLSL
#define SHADOW_COMMON_GLSL

const int NR_LIGHTS = 4;

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

uniform PointLight pointLights[NR_LIGHTS];
uniform samplerCube shadowMap[NR_LIGHTS];
// ガラスを透過した光の色。ガラスを通らない方向は白
uniform samplerCube shadowColor[NR_LIGHTS];
uniform float farPlane;

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

#endif
