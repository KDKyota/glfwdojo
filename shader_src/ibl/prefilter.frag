// 環境マップを roughness ごとにぼかし、ミップの各レベルへ焼く。起動時に1回だけ
#version 460 core

in vec3 LocalPos;
out vec4 FragColor;

#include "pbr_common.glsl"

uniform samplerCube environmentMap;
uniform float roughness;
// ミップ選択に元の解像度が必要
uniform float envResolution;

void main() {
    vec3 N = normalize(LocalPos);
    // 視線＝法線＝反射方向と仮定する。分割和近似の主な誤差源はここ
    vec3 R = N;
    vec3 V = R;

    const uint SAMPLE_COUNT = 1024u;
    vec3 prefilteredColor = vec3(0.0);
    float totalWeight = 0.0;

    for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H = ImportanceSampleGGX(Xi, N, roughness);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        if (NdotL <= 0.0)
            continue;

        // サンプルが疎な方向ほど粗いミップを引き、ちらつく白い点を抑える
        float D = DistributionGGX(N, H, roughness);
        float NdotH = max(dot(N, H), 0.0);
        float HdotV = max(dot(H, V), 0.0);
        float pdf = D * NdotH / (4.0 * HdotV) + 0.0001;

        float saTexel = 4.0 * PI / (6.0 * envResolution * envResolution);
        float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);
        float mipLevel = roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel);

        prefilteredColor += textureLod(environmentMap, L, mipLevel).rgb * NdotL;
        totalWeight += NdotL;
    }

    FragColor = vec4(prefilteredColor / totalWeight, 1.0);
}
