#version 330 core

in vec4 FragPos;

uniform vec3 lightPos;
uniform float farPlane;

void main()
{
    float lightDistance = length(FragPos.xyz - lightPos);
    // 線形深度として [0,1] に正規化して書き込む
    gl_FragDepth = lightDistance / farPlane;
}
