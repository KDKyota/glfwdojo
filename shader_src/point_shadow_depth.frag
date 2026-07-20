#version 330 core

// geometry shader から中継されたフラグメントのワールド座標
in vec4 FragPos;

// 光源のワールド座標（点光源の位置）
uniform vec3 lightPos;
// 光源からの距離を正規化するための基準値（Scene.h の shadowFarPlane_ と同じ値）
uniform float farPlane;

void main()
{
    // 光源からフラグメントまでの実際のユークリッド距離
    float lightDistance = length(FragPos.xyz - lightPos);
    // 線形深度として [0,1] に正規化して書き込む
    // （投影のzではなく実距離を使うことで、cubemapの面の継ぎ目でも深度が連続になる）
    gl_FragDepth = lightDistance / farPlane;
}
