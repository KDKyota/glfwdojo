#version 460 core

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D diffuseMap;

void main()
{
      vec4 diffuseColor = texture(diffuseMap, TexCoords);

      gPosition = FragPos;
      gNormal = normalize(Normal);
      gAlbedoSpec.rgb = diffuseColor.rgb;
      // 専用specularテクスチャが無いのでdiffuseの輝度を代用
      gAlbedoSpec.a = dot(diffuseColor.rgb, vec3(0.2126, 0.7152, 0.0722));
}  