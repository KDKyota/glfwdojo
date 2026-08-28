// Deferred Shading の合成パス。debugMode != 0 のときは中間バッファを可視化する。
#version 460 core
out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

in vec2 TexCoords;

#include "lighting_common.glsl"

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoRoughness;
// 1.0=遮蔽なし, 0.0=完全遮蔽
uniform sampler2D ssao;
// スカイボックスを半球で畳み込んだ拡散反射用の環境光
uniform samplerCube irradianceMap;
// ミップの各レベルが roughness に対応する鏡面反射用の環境光
uniform samplerCube prefilterMap;
// (dot(N,V), roughness) -> F0 に掛けるスケールとバイアス
uniform sampler2D brdfLUT;
const float MAX_REFLECTION_LOD = 4.0;

uniform vec3 viewPos;

// 対応表は main.cpp の kDebugModes。hdr.frag の debugRawOutput も要有効
uniform int debugMode;

uniform float ssaoStrength;

void main() {
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 Normal = texture(gNormal, TexCoords).rgb;
    vec3 Albedo = texture(gAlbedoRoughness, TexCoords).rgb;
    float Roughness = texture(gAlbedoRoughness, TexCoords).a;
    float Metallic = texture(gNormal, TexCoords).a;

    if (debugMode == 0) {
        // ---- 通常のライティング ----
        float AmbientOcclusion =
            mix(1.0, texture(ssao, TexCoords).r, ssaoStrength);

        vec3 viewDir = normalize(viewPos - FragPos);

        // 非金属は一律 0.04、金属は鏡面反射がアルベドの色を持つ
        vec3 F0 = mix(vec3(0.04), Albedo, Metallic);

        // 環境光は IBL から。ループの外で1回だけ求める
        float NdotV = max(dot(Normal, viewDir), 0.0);
        vec3 kS = fresnelSchlickRoughness(NdotV, F0, Roughness);
        vec3 kD = (1.0 - kS) * (1.0 - Metallic);

        vec3 diffuseIBL = texture(irradianceMap, Normal).rgb * Albedo;

        // 反射方向の環境光を roughness に応じたミップから引き、LUT で反射率を補正する
        vec3 R = reflect(-viewDir, Normal);
        vec3 prefiltered = textureLod(prefilterMap, R, Roughness * MAX_REFLECTION_LOD).rgb;
        vec2 brdf = texture(brdfLUT, vec2(NdotV, Roughness)).rg;
        vec3 specularIBL = prefiltered * (kS * brdf.x + brdf.y);

        // kD が掛かるのは拡散だけ。鏡面は LUT 経由で kS を内包している
        vec3 result = (kD * diffuseIBL + specularIBL) * AmbientOcclusion * ambientStrength;

        for (int i = 0; i < NR_LIGHTS; ++i) {
            // PCF で複数回サンプリングする ShadowCalculation() の手前で弾く
            float dist = length(pointLights[i].position - FragPos);
            if (dist >= pointLights[i].radius)
                continue;

            vec3 lightDir = normalize(pointLights[i].position - FragPos);
            float shadow = ShadowCalculation(FragPos, Normal, lightDir,
                    pointLights[i].position, shadowMap[i]);
            // 窓枠は shadow≈1 で黒い影、ガラスは shadow=0 のままここで色付きに減衰する
            vec3 transmit =
                texture(shadowColor[i], FragPos - pointLights[i].position).rgb;
            result += transmit * CalcPointLight(pointLights[i], Normal, FragPos, viewDir,
                        Albedo, Roughness, Metallic, F0, shadow);
        }

        FragColor = vec4(result, 1.0);

        // 明るいピクセルだけを BrightColor に残し、Bloom の素材にする
        float brightness = dot(result, vec3(0.2126, 0.7152, 0.0722));
        if (brightness > 1.0)
            BrightColor = vec4(result, 1.0);
        else
            BrightColor = vec4(0.0, 0.0, 0.0, 1.0);

        return;
    }

    // ---- デバッグ表示 ----
    // Bloom が乗ると判定できなくなるので、デバッグ中は BrightColor を常に黒にする
    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);

    if (debugMode == 1) {
        // 必ず1灯だけで見ること。4灯を max() でまとめるとほぼ全面が白くなり判定できない
        vec3 lightDir0 = normalize(pointLights[0].position - FragPos);
        float shadow0 = ShadowCalculation(FragPos, Normal, lightDir0,
                pointLights[0].position, shadowMap[0]);
        FragColor = vec4(vec3(shadow0), 1.0);
    } else if (debugMode == 2) {
        vec3 fragToLight0 = FragPos - pointLights[0].position;
        float closest = texture(shadowMap[0], fragToLight0).r;
        FragColor = vec4(vec3(closest), 1.0);
    } else if (debugMode == 3) {
        FragColor = vec4(Albedo, 1.0);
    } else if (debugMode == 4) {
        FragColor = vec4(Normal * 0.5 + 0.5, 1.0);
    } else if (debugMode == 5) {
        FragColor = vec4(abs(FragPos) / farPlane, 1.0);
    } else if (debugMode == 6) {
        // 4分割して G-Buffer とシャドウマップを同時に見る
        vec2 uv = TexCoords * 2.0;
        vec2 quad = floor(uv); // (0,0)=左下 (1,0)=右下 (0,1)=左上 (1,1)=右上
        vec2 localUV = fract(uv);

        vec3 qPos = texture(gPosition, localUV).rgb;
        vec3 qNormal = texture(gNormal, localUV).rgb;
        vec4 qAlbedo = texture(gAlbedoRoughness, localUV);

        vec3 debugColor;
        if (quad.y > 0.5 && quad.x < 0.5)
            debugColor = qAlbedo.rgb; // 左上: Albedo
        else if (quad.y > 0.5)
            debugColor = qNormal * 0.5 + 0.5; // 右上: Normal
        else if (quad.x < 0.5)
            debugColor = abs(qPos) / 25.0; // 左下: Position
        else {
            // 右下: shadowMap[0] の生の深度値
            vec3 dir = qPos - pointLights[0].position;
            debugColor = vec3(texture(shadowMap[0], dir).r);
        }

        if (abs(TexCoords.x - 0.5) < 0.001 || abs(TexCoords.y - 0.5) < 0.001)
            debugColor = vec3(1.0, 0.0, 0.0);

        FragColor = vec4(debugColor, 1.0);
    } else if (debugMode == 7) {
        // 真っ黒なら FBO・サンプラー・パスの配線が繋がっていない
        float ao = texture(ssao, TexCoords).r;
        FragColor = vec4(vec3(ao), 1.0);
    } else if (debugMode == 8) {
        // 白地にガラスのシルエットが色付きで写れば配線は正しい
        vec3 dir0 = FragPos - pointLights[0].position;
        FragColor = vec4(texture(shadowColor[0], dir0).rgb, 1.0);
    } else if (debugMode == 9) {
        // オブジェクトごとに一様な明るさで見えれば gNormal.a への書き込みは正しい
        FragColor = vec4(vec3(Metallic), 1.0);
    } else if (debugMode == 10) {
        FragColor = vec4(vec3(Roughness), 1.0);
    } else if (debugMode == 11) {
        // 上向きの面が空色、下向きが地面色になっていれば畳み込みは成功
        FragColor = vec4(texture(irradianceMap, Normal).rgb, 1.0);
    } else if (debugMode == 12) {
        // roughness を上げるほど映り込みがぼけていけば正常
        vec3 R = reflect(-normalize(viewPos - FragPos), Normal);
        FragColor = vec4(textureLod(prefilterMap, R, Roughness * MAX_REFLECTION_LOD).rgb, 1.0);
    } else if (debugMode == 13) {
        // 画面全体に LUT を貼る。左下が暗く右上が明るい赤緑のグラデーションが正解
        FragColor = vec4(texture(brdfLUT, TexCoords).rg, 0.0, 1.0);
    } else {
        FragColor = vec4(1.0, 0.0, 1.0, 1.0); // 未定義の debugMode（マゼンタ）
    }
}
