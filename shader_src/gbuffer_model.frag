#version 460 core

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec4 gNormal; // a = metallic
layout (location = 2) out vec4 gAlbedoRoughness;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in mat3 TBN;

uniform sampler2D baseColorMap;
// glTF は metallic と roughness を1枚にパックしている（G=roughness, B=metallic）
uniform sampler2D metallicRoughnessMap;
uniform sampler2D normalMap;
uniform sampler2D occlusionMap;
uniform sampler2D emissiveMap;

// テクスチャを持たないマテリアルでは factor だけが効く
uniform bool hasBaseColorMap;
uniform bool hasMetallicRoughnessMap;
uniform bool hasNormalMap;
uniform bool hasOcclusionMap;
uniform bool hasEmissiveMap;

uniform vec3 baseColorFactor;
uniform vec3 emissiveFactor;
uniform float metallic;
uniform float roughness;

void main()
{
    vec3 albedo = baseColorFactor;
    if (hasBaseColorMap)
        albedo *= texture(baseColorMap, TexCoords).rgb;

    float metallicValue = metallic;
    float roughnessValue = roughness;
    if (hasMetallicRoughnessMap) {
        vec3 packed = texture(metallicRoughnessMap, TexCoords).rgb;
        roughnessValue *= packed.g;
        metallicValue *= packed.b;
    }

    vec3 normal = normalize(Normal);
    if (hasNormalMap) {
        vec3 tangentNormal = texture(normalMap, TexCoords).rgb * 2.0 - 1.0;
        normal = normalize(TBN * tangentNormal);
    }

    // occlusionMap と emissiveMap は G-Buffer に置く枠がまだ無いので、読み込むだけで使っていない。
    // AO は SSAO と合流させる必要があり、emissive はライティングの後に足す必要がある

    gPosition = FragPos;
    gNormal = vec4(normal, metallicValue);
    gAlbedoRoughness = vec4(albedo, roughnessValue);
}
