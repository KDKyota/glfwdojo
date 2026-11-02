// G-Bufferから受け取ったテクスチャの床面だけを1とするマスクテクスチャを作成する
#version 460 core

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform float floorY;
layout(location = 0) out vec4 mask; // 床部分だけを1とするマスクテクスチャ

in vec2 TexCoords;

const float FLOOR_PLANE_EPSILON = 0.05;

void main () {
    ivec2 gSize = textureSize(gPosition, 0); // gPosition の解像度サイズを取得

    // 半解像度のテクスチャなので2x2の4ピクセルで考える
    vec2 centerPx = TexCoords * gSize; // 全解像度でのピクセル座標（texCoords からの変換）
    ivec2 topLeft = ivec2(floor(centerPx - vec2(0.5))); 

    bool isAllFloor = true;
    for (int dy = 0; dy <= 1; ++dy) {
        for (int dx = 0; dx <= 1; ++dx) {
            ivec2 texel = clamp(topLeft + ivec2(dx, dy), ivec2(0), ivec2(gSize - 1)); // 注目しているテクセル
            vec4 pos = texelFetch(gPosition, texel, 0);
            vec4 normal = texelFetch(gNormal, texel, 0);
            bool onFloorPlane = abs(pos.y - floorY) < FLOOR_PLANE_EPSILON; // 座標が床面か
            bool facesUp = normal.y > 0.9; // 法線が真上を向いているか
            if (!(onFloorPlane && facesUp))
                isAllFloor = false;
        }
    }
    if (isAllFloor) // 4ピクセルとも床なら1を返す
        mask = vec4(1.0);
    else 
        mask = vec4(0.0);

    return;
}