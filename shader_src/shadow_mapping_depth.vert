#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 3) in vec3 aOffset; // インスタンスごとの位置オフセット（床など非インスタンスは 0,0,0）

// point light と違い光源に向きがあるので、cubemap ではなく1枚の2Dマップで済む
uniform mat4 lightSpaceMatrix;
uniform mat4 model;

void main()
{
    // view/projection の代わりに lightSpaceMatrix を使い、光源から見た深度を書く
    gl_Position = lightSpaceMatrix * model * vec4(aPos + aOffset, 1.0);
}
