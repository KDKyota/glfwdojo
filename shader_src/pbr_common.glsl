// #include される側なので #version は書かない
// Cook-Torrance BRDF の D/G/F 項と IBL 用のサンプリングユーティリティ。
#ifndef PBR_COMMON_GLSL
#define PBR_COMMON_GLSL

const float PI = 3.14159265359;

// 微小鏡のうちハーフベクトル H を向いているものの割合。積分すると 1 になる
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    // 式中の α は roughness そのものではなく roughness の二乗
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = NdotH2 * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

// LearnOpenGL の実装に合わせ、k は α ではなく roughness から作る
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

// Fresnel-Schlick 近似。浅い角度ほど反射率が F0 から 1.0 に近づく。
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// IBL 用。環境光はあらゆる方向から来るので、粗い面ではフレネルの立ち上がりが鈍る
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) *
        pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

/* ==== IBL の事前計算専用（毎フレームは使わない） ==== */

// ビット反転で作る低食い違い量列
float RadicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

vec2 Hammersley(uint i, uint N) {
    return vec2(float(i) / float(N), RadicalInverse_VdC(i));
}

// GGX の分布に沿ってハーフベクトルを撒く。一様に撒くより早く収束する
vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness) {
    float a = roughness * roughness;

    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    vec3 H = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);

    return normalize(tangent * H.x + bitangent * H.y + N * H.z);
}

// 直接光版とは k の定義が違う（α²/2 と (α+1)²/8）。取り違えると LUT がずれる
float GeometrySchlickGGX_IBL(float NdotV, float roughness) {
    float k = (roughness * roughness) / 2.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmithIBL(vec3 N, vec3 V, vec3 L, float roughness) {
    return GeometrySchlickGGX_IBL(max(dot(N, L), 0.0), roughness) *
        GeometrySchlickGGX_IBL(max(dot(N, V), 0.0), roughness);
}

#endif
