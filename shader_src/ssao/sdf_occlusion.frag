// SDF レイマーチで拡散 IBL の可視性を求める専用パス
#version 460 core

out float FragColor;

in vec2 TexCoords;

#include "sdf_common.glsl"

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D texNoise; // 4x4 のランダム回転ベクトル SSAO と共用

void main() {
    vec3 normal = texture(gNormal, TexCoords).rgb;
    // 背景は gNormal がゼロなので原点からレイが出ないよう弾く
    if (dot(normal, normal) < 0.5) {
        FragColor = 1.0;
        return;
    }

    // 全画素で同じ 16 方向を使うと階調の境目が縞になるので法線まわりに回す
    // gPosition の解像度から作るとこのパスが半解像度のときタイル周期がずれる
    float angle = texture(texNoise, gl_FragCoord.xy / 4.0).x * PI;
    vec2 rotation = vec2(cos(angle), sin(angle));

    FragColor = sdfSkyVisibility(texture(gPosition, TexCoords).rgb, normalize(normal), rotation);
}
