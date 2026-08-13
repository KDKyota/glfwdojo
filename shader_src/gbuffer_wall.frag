#version 460 core

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec4 gNormal; // a = metallic
layout (location = 2) out vec4 gAlbedoSpec;

in vec3 FragPos;
in vec2 TexCoords;
in mat3 TBN;

uniform sampler2D diffuseMap;
uniform sampler2D normalMap;
uniform float metallic;
uniform float roughness;

void main()
{
      vec3 normal = texture(normalMap, TexCoords).rgb;
      normal = normalize(normal * 2.0 - 1.0);
      normal = normalize(TBN * normal); // タンジェント空間からワールド空間へ

      vec4 diffuseColor = texture(diffuseMap, TexCoords);

      gPosition = FragPos;
      gNormal = vec4(normal, metallic);
      gAlbedoSpec.rgb = diffuseColor.rgb;
      gAlbedoSpec.a = roughness;
}
