#version 460 core

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec4 gNormal; // a = metallic
layout (location = 2) out vec4 gAlbedoSpec;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D diffuseMap;
uniform float metallic;

// 1 にすると床が固定色を書く。debugMode 6 でマゼンタだけなら 0/1 に届いていない
#define GBUFFER_WRITE_TEST 0

void main()
{
#if GBUFFER_WRITE_TEST
      gPosition   = vec3(25.0, 0.0, 0.0);          // Position象限で abs(p)/25.0 → 赤
      gNormal     = vec4(-1.0, -1.0, 1.0, 0.0);    // Normal象限で n*0.5+0.5 → 青
      gAlbedoSpec = vec4(1.0, 0.0, 1.0, 1.0);      // Albedo象限 → マゼンタ
#else
      vec4 diffuseColor = texture(diffuseMap, TexCoords);

      gPosition = FragPos;
      gNormal = vec4(normalize(Normal), metallic);
      gAlbedoSpec.rgb = diffuseColor.rgb;
      // 専用specularテクスチャが無いのでdiffuseの輝度を代用
      gAlbedoSpec.a = dot(diffuseColor.rgb, vec3(0.2126, 0.7152, 0.0722));
#endif
}