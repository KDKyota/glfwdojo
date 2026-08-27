// Point Light 用シャドウマップの深度パス。光源からの正規化距離を書き込む。
#version 330 core

// geometry shader から中継されたフラグメントのワールド座標
in vec4 FragPos;
in vec2 TexCoords;

// 光源のワールド座標（点光源の位置）
uniform vec3 lightPos;
// 光源からの距離を正規化するための基準値（Scene.h の shadowFarPlane_ と同じ値）
uniform float farPlane;
uniform bool useAlphaTest; // Alpha値を使うのかどうか（今のところ透過窓だけ使う）
uniform sampler2D diffuseMap;

void main()
{
    if (useAlphaTest && texture(diffuseMap, TexCoords).a < 0.5)
        discard;
    // 光源からフラグメントまでの実際のユークリッド距離
    float lightDistance = length(FragPos.xyz - lightPos);
    // 投影の z ではなく実距離を使うと、cubemap の面の継ぎ目でも深度が連続になる
    gl_FragDepth = lightDistance / farPlane;
}
