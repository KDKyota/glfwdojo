#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 3) in vec3 aOffset; // インスタンスごとの位置オフセット（床など非インスタンスは 0,0,0）

uniform mat4 lightSpaceMatrix;
uniform mat4 model;

void main()
{
    gl_Position = lightSpaceMatrix * model * vec4(aPos + aOffset, 1.0);
}