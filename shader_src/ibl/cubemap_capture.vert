// キューブマップの6面を1面ずつ描くための頂点シェーダー
// フラグメント側でサンプリング方向として使うためローカル座標をそのまま渡す
#version 460 core
layout(location = 0) in vec3 aPos;

uniform mat4 projection;
uniform mat4 view;

out vec3 LocalPos;

void main() {
    LocalPos = aPos;
    gl_Position = projection * view * vec4(aPos, 1.0);
}
