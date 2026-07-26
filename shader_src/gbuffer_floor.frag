#version 460 core

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D diffuseMap;

// ==== G-Buffer 書き込みテスト ====
// 1 にすると、床の描画時に G-Buffer の3枚へ「位置や法線とは無関係な固定色」を書き込む。
// deferred_lighting.frag の DEBUG_MODE 6 と組み合わせて、床のある領域が
//   Albedo象限   → マゼンタ
//   Normal象限   → 青
//   Position象限 → 赤
// になるかを見る。
//   3つとも出る   → 書き込み自体は正常。原因は FragPos / Normal の値が壊れていること
//   マゼンタだけ  → アタッチメント0と1に書き込みが届いていない（FBO / glDrawBuffers 側の問題）
// 確認が終わったら 0 に戻すこと。
#define GBUFFER_WRITE_TEST 0

void main()
{
#if GBUFFER_WRITE_TEST
      gPosition   = vec3(25.0, 0.0, 0.0);          // Position象限で abs(p)/25.0 → 赤
      gNormal     = vec3(-1.0, -1.0, 1.0);         // Normal象限で n*0.5+0.5 → 青
      gAlbedoSpec = vec4(1.0, 0.0, 1.0, 1.0);      // Albedo象限 → マゼンタ
#else
      vec4 diffuseColor = texture(diffuseMap, TexCoords);

      gPosition = FragPos;
      gNormal = normalize(Normal);
      gAlbedoSpec.rgb = diffuseColor.rgb;
      // 専用specularテクスチャが無いのでdiffuseの輝度を代用
      gAlbedoSpec.a = dot(diffuseColor.rgb, vec3(0.2126, 0.7152, 0.0722));
#endif
}