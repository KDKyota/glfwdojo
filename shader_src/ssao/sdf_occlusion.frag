// SDF レイマーチで拡散と鏡面の IBL の可視性を求める専用パス 半解像度で動く
#version 460 core

// r: 拡散の可視性 g: 鏡面の可視性
out vec2 FragColor;

in vec2 TexCoords;

#include "sdf_common.glsl"

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoRoughness;
uniform sampler2D texNoise; // 4x4 のランダム回転ベクトル SSAO と共用
uniform vec3 viewPos;
uniform float sdfOcclusionStrength; // 0 なら鏡面のトレースを省く
uniform bool debugShowSteps; // true なら AO 値の代わりに正規化したステップ数を出力する

void main() {
    vec3 normal = texture(gNormal, TexCoords).rgb;
    // 背景は gNormal がゼロなので原点からレイが出ないよう弾く
    if (dot(normal, normal) < 0.5) {
        FragColor = vec2(debugShowSteps ? 0.0 : 1.0);
        return;
    }

    // 全画素で同じ 16 方向を使うと階調の境目が縞になるので法線まわりに回す
    // gPosition の解像度から作るとこのパスが半解像度のときタイル周期がずれる
    float angle = texture(texNoise, gl_FragCoord.xy / 4.0).x * PI;
    vec2 rotation = vec2(cos(angle), sin(angle));

    int maxSteps;
    float visibility = sdfSkyVisibility(texture(gPosition, TexCoords).rgb, normalize(normal), rotation, maxSteps);

    // 鏡面の代表点はフル解像度の左上の画素 Lighting 側の補間がこの位置を前提にしている
    float specularVisibility = 1.0;
    if (!debugShowSteps && sdfOcclusionStrength > 0.0) {
        // 半解像度を扱うので (i, j) -> (2i, 2j) として扱う
        ivec2 representative = 2 * ivec2(gl_FragCoord.xy);
        vec3 repNormal = texelFetch(gNormal, representative, 0).xyz;
        float roughness = texelFetch(gAlbedoRoughness, representative, 0).a;
        if (dot(repNormal, repNormal) >= 0.5 && roughness < SDF_ROUGHNESS_THRESHOLD) {
            vec3 repPos = texelFetch(gPosition, representative, 0).xyz;
            repNormal = normalize(repNormal);
            vec3 reflected = reflect(-normalize(viewPos - repPos), repNormal);
            // 鏡面は遠くの壁も映り込むので AO 用の短い tMax ではなくシーン全体を抜ける距離を使う
            specularVisibility = sdfEnvVisibility(repPos, repNormal, normalize(reflected), roughness * roughness, sceneParams.w);
        }
    }

    FragColor = vec2(debugShowSteps ? float(maxSteps) / SDF_STEP_HEATMAP_REF : visibility, specularVisibility);
}
