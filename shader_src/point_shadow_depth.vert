#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 3) in vec3 aOffset;

uniform mat4 model;

void main()
{
    // ここではまだ view/projection をかけない
    // （光源視点への変換は後段の geometry shader が面ごとの shadowMatrices で行うため、
    //   ここではワールド座標のまま gl_Position に渡す）
    gl_Position = model * vec4(aPos + aOffset, 1.0);
}
