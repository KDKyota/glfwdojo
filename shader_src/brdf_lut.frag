// F0 に掛けるスケールとバイアスを (dot(N,V), roughness) の表として焼く。
// 環境にも材質の色にも依存しない普遍的なテーブルなので、起動時に1回だけ
#version 460 core

in vec2 TexCoords;
out vec2 FragColor;

#include "pbr_common.glsl"

vec2 IntegrateBRDF(float NdotV, float roughness) {
    // 法線を +Z に固定し、NdotV から視線を逆算する
    vec3 V = vec3(sqrt(1.0 - NdotV * NdotV), 0.0, NdotV);
    vec3 N = vec3(0.0, 0.0, 1.0);

    float A = 0.0;
    float B = 0.0;

    const uint SAMPLE_COUNT = 1024u;
    for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H = ImportanceSampleGGX(Xi, N, roughness);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(L.z, 0.0);
        if (NdotL <= 0.0)
            continue;

        float NdotH = max(H.z, 0.0);
        float VdotH = max(dot(V, H), 0.0);

        // 直接光版ではなく GeometrySmithIBL を使うこと
        float G = GeometrySmithIBL(N, V, L, roughness);
        float G_Vis = (G * VdotH) / (NdotH * NdotV);
        float Fc = pow(1.0 - VdotH, 5.0);

        A += (1.0 - Fc) * G_Vis;
        B += Fc * G_Vis;
    }
    return vec2(A, B) / float(SAMPLE_COUNT);
}

void main() {
    FragColor = IntegrateBRDF(TexCoords.x, TexCoords.y);
}
