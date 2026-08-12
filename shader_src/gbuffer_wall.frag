#version 460 core

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec4 gNormal; // a = metallic
layout (location = 2) out vec4 gAlbedoSpec;

in vec3 FragPos;
in vec2 TexCoords;
in mat3 TBN;

uniform sampler2D diffuseMap;
uniform sampler2D normalMap;

void main()
{
      vec3 normal = texture(normalMap, TexCoords).rgb;
      normal = normalize(normal * 2.0 - 1.0);
      normal = normalize(TBN * normal); // タンジェント空間からワールド空間へ

      vec4 diffuseColor = texture(diffuseMap, TexCoords);

      gPosition = FragPos;
      gNormal = vec4(normal, 0.75); // TODO: 配線確認用の仮値。確認後 0.0 に戻す
      gAlbedoSpec.rgb = diffuseColor.rgb;
      gAlbedoSpec.a = dot(diffuseColor.rgb, vec3(0.2126, 0.7152, 0.0722));
}
