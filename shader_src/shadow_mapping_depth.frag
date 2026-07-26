#version 330 core

void main()
{
    // 出力色は使わないため、OpenGL が gl_Position.z / w から
    // 正規化深度 [0,1] を自動計算して depth attachment（2Dテクスチャ）に書き込む
    // → shader.frag 側では projCoords.z としてこの値を読み出して比較する
}