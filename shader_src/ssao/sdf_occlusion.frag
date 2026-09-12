// SDF レイマーチで拡散 IBL の可視性を求める専用パス
#version 460 core

out float FragColor;

in vec2 TexCoords;

#include "sdf_common.glsl"

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D texNoise; // 4x4 のランダム回転ベクトル SSAO と共用
uniform bool debugShowSteps; // true なら AO 値の代わりに正規化したステップ数を出力する

void main() {
    vec3 normal = texture(gNormal, TexCoords).rgb;
    // 背景は gNormal がゼロなので原点からレイが出ないよう弾く
    if (dot(normal, normal) < 0.5) {
        FragColor = debugShowSteps ? 0.0 : 1.0;
        return;
    }

    // 全画素で同じ 16 方向を使うと階調の境目が縞になるので法線まわりに回す
    // gPosition の解像度から作るとこのパスが半解像度のときタイル周期がずれる
    float angle = texture(texNoise, gl_FragCoord.xy / 4.0).x * PI;
    vec2 rotation = vec2(cos(angle), sin(angle));

    int maxSteps;
    float visibility = sdfSkyVisibility(texture(gPosition, TexCoords).rgb, normalize(normal), rotation, maxSteps);
    FragColor = debugShowSteps ? float(maxSteps) / float(SDF_MAX_STEPS) : visibility;
}
