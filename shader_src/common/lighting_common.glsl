// #include される側なので #version は書かない
// PointLight と BRDF 関数の両方に依存するので、必要なものを自分で include する
#ifndef LIGHTING_COMMON_GLSL
#define LIGHTING_COMMON_GLSL

#include "shadow_common.glsl"
#include "pbr_common.glsl"

// この光源からの直接光だけを返す。環境光は呼び出し側で一括して足す。
// ガラスのように拡散反射を持たせたくない場合は albedo に 0 を渡す
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir,
    vec3 albedo, float roughness, float metallic, vec3 F0, float shadow) {
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

#endif
