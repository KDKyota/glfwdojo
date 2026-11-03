// 縮小するときのぼかし方を自作して一つのミップとするフラグメントシェーダー
#version 460 core

uniform sampler2D reductionSrcTex; // 縮小元のテクスチャ
uniform bool isKarisAvg; // カリス平均をするか（最初の1回だけカリス平均をする）
layout(location = 0) out vec4 downsample;
in vec2 TexCoords;

vec3 PowVec3(vec3 v, float p) {
    return vec3(pow(v.x, p), pow(v.y, p), pow(v.z, p));
}

const float invGamma = 1.0 / 2.2;
vec3 ToSRGB(vec3 v) {
    return PowVec3(v, invGamma);
}

float RGBToLuminance(vec3 col) {
    return dot(col, vec3(0.2126, 0.7152, 0.0722));
}

float KarisAverage(vec3 col) {
    // Formula is 1 / (1 + luma)
    float luma = RGBToLuminance(ToSRGB(col)) * 0.25;
    return 1.0 / (1.0 + luma);
}

void main() {
    vec2 srcTexelSize = 1.0 / textureSize(reductionSrcTex, 0); // 縮小前の1テクセルの大きさ
    float x = srcTexelSize.x;
    float y = srcTexelSize.y;

    // Take 13 samples around current texel:
    // a - b - c
    // - j - k -
    // d - e - f
    // - l - m -
    // g - h - i
    // === ('e' is the current texel) ===
    vec4 a = texture(reductionSrcTex, vec2(TexCoords.x - 2 * x, TexCoords.y + 2 * y));
    vec4 b = texture(reductionSrcTex, vec2(TexCoords.x, TexCoords.y + 2 * y));
    vec4 c = texture(reductionSrcTex, vec2(TexCoords.x + 2 * x, TexCoords.y + 2 * y));

    vec4 d = texture(reductionSrcTex, vec2(TexCoords.x - 2 * x, TexCoords.y));
    vec4 e = texture(reductionSrcTex, vec2(TexCoords.x, TexCoords.y));
    vec4 f = texture(reductionSrcTex, vec2(TexCoords.x + 2 * x, TexCoords.y));

    vec4 g = texture(reductionSrcTex, vec2(TexCoords.x - 2 * x, TexCoords.y - 2 * y));
    vec4 h = texture(reductionSrcTex, vec2(TexCoords.x, TexCoords.y - 2 * y));
    vec4 i = texture(reductionSrcTex, vec2(TexCoords.x + 2 * x, TexCoords.y - 2 * y));

    vec4 j = texture(reductionSrcTex, vec2(TexCoords.x - x, TexCoords.y + y));
    vec4 k = texture(reductionSrcTex, vec2(TexCoords.x + x, TexCoords.y + y));
    vec4 l = texture(reductionSrcTex, vec2(TexCoords.x - x, TexCoords.y - y));
    vec4 m = texture(reductionSrcTex, vec2(TexCoords.x + x, TexCoords.y - y));

    vec4 groups[5];
    float groupWeight[5] = {0.125, 0.125, 0.125, 0.125, 0.5};
    vec4 weightedSum = vec4(0.0);
    float totalWeight = 0;
    if(isKarisAvg) {
    // We are writing to mip 0, so we need to apply Karis average to each block
    // of 4 samples to prevent fireflies (very bright subpixels, leads to pulsating
    // artifacts).
        groups[0] = (a + b + d + e) / 4.0;
        groups[1] = (b + c + e + f) / 4.0;
        groups[2] = (d + e + g + h) / 4.0;
        groups[3] = (e + f + h + i) / 4.0;
        groups[4] = (j + k + l + m) / 4.0;
        for(int i = 0; i < 5; ++i)  {
            float karis = KarisAverage(groups[i].rgb); // グループのカリス平均
            float weight = groupWeight[i] * karis; // 重み
            totalWeight += weight;
            weightedSum += groups[i] * weight;
        }

        downsample = weightedSum / totalWeight;
    } else {
        // Apply weighted distribution:
        // 0.5 + 0.125 + 0.125 + 0.125 + 0.125 = 1
        // a,b,d,e * 0.125
        // b,c,e,f * 0.125
        // d,e,g,h * 0.125
        // e,f,h,i * 0.125
        // j,k,l,m * 0.5
        // This shows 5 square areas that are being sampled. But some of them overlap,
        // so to have an energy preserving downsample we need to make some adjustments.
        // The weights are the distributed, so that the sum of j,k,l,m (e.g.)
        // contribute 0.5 to the final color output. The code below is written
        // to effectively yield this sum. We get:
        // 0.125*5 + 0.03125*4 + 0.0625*4 = 1
        downsample = e * 0.125;
        downsample += (a + c + g + i) * 0.03125;
        downsample += (b + d + f + h) * 0.0625;
        downsample += (j + k + l + m) * 0.125;
    }


    return;
}