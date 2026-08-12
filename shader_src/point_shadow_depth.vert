#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 2) in vec2 aTexCoords;
// 有効化していないVAOでは既定値 (0,0,0) が読まれて無効化される。この依存は意図的
layout(location = 5) in vec3 aOffset;

uniform mat4 model;

out vec2 vTexCoords;

void main()
{
    // 光源視点への変換は geometry shader が面ごとに行うのでワールド座標のまま渡す
    gl_Position = model * vec4(aPos + aOffset, 1.0);
    vTexCoords = aTexCoords;
}
