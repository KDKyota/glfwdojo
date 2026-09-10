// SDF レイマーチで拡散 IBL の可視性を求める専用パス
#version 460 core

out float FragColor;

in vec2 TexCoords;

#include "sdf_common.glsl"

uniform sampler2D gPosition;
uniform sampler2D gNormal;

void main() {
    vec3 normal = texture(gNormal, TexCoords).rgb;
    // 背景は gNormal がゼロなので原点からレイが出ないよう弾く
    if (dot(normal, normal) < 0.5) {
        FragColor = 1.0;
        return;
    }
    FragColor = sdfSkyVisibility(texture(gPosition, TexCoords).rgb, normalize(normal));
}
