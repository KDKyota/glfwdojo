// Deferred Shading の合成パス debugMode != 0 のときは中間バッファを可視化する
#version 460 core

#include "sdf_common.glsl"
#include "lighting_common.glsl"
out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

in vec2 TexCoords;

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
// SDF レイマーチで焼いた拡散側の可視性 鏡面は視線依存なので焼けずここには入らない
uniform sampler2D sdfOcclusion;
// 床で折り返した鏡像カメラで描いた反射像
uniform sampler2D reflectionColor;
uniform bool hasReflection;
// 反射像へ SDF の鏡面遮蔽も重ねるか 見比べのために切り替えられるようにしている
uniform bool floorSdfSpecularOcclusion;
const float MAX_REFLECTION_LOD = 4.0;
// 床の判定 gPosition の量子化誤差より大きく取る
const float FLOOR_PLANE_EPSILON = 0.05;
// reflectionColor のミップを何段まで使うか roughness=1 で最もぼけたレベルを選ぶ
const float MAX_REFLECTION_COLOR_LOD = 5.0;

uniform vec3 viewPos;

// 対応表は main.cpp の kDebugModes hdr.frag の debugRawOutput も要有効
uniform int debugMode;

uniform float ssaoStrength;

// debugMode 14 でレイを飛ばす方向(ImGui から変更可能にする)
uniform vec3 sdfDebugDir;

// 代表点の面との距離がこの範囲を超えたら別の面とみなす gPosition の量子化誤差(cm 単位)より大きくする
const float SDF_UPSAMPLE_PLANE_SIGMA = 0.05;
// 有効な代表点の重みの合計がこれ未満なら 補間せずフル解像度でトレースし直す
const float SDF_UPSAMPLE_MIN_WEIGHT = 0.05;

// 半解像度パスが書いた鏡面の可視性を フル解像度の画素へ補間して返す
// 半解像度のテクセル (i,j) はフル解像度の画素 (2i,2j) を代表点としてトレースしているので
// 同じ面の代表点だけを重みに使い 物体の輪郭を越えて値が染み出さないようにする
// 使える代表点が無ければ -1
float upsampleSpecularVisibility(vec3 pos, vec3 normal, ivec2 pixel) {
    ivec2 halfSize = textureSize(sdfOcclusion, 0);
    ivec2 base = pixel >> 1;
    // 偶数の画素は代表点そのもの 奇数の画素は隣の代表点との中点
    vec2 fraction = vec2(pixel & 1) * 0.5;

    float sum = 0.0;
    float weightSum = 0.0;
    for (int j = 0; j < 2; ++j) {
        for (int i = 0; i < 2; ++i) {
            float bilinear = (i == 0 ? 1.0 - fraction.x : fraction.x) * (j == 0 ? 1.0 - fraction.y : fraction.y);
            if (bilinear <= 0.0)
                continue;

            ivec2 texel = min(base + ivec2(i, j), halfSize - 1);
            ivec2 representative = texel * 2;
            vec4 repNormal = texelFetch(gNormal, representative, 0);
            float repRoughness = texelFetch(gAlbedoRoughness, representative, 0).a;
            // 背景と 粗くて鏡面をトレースしていない画素の値は使えない
            if (dot(repNormal.xyz, repNormal.xyz) < 0.5 || repRoughness >= SDF_ROUGHNESS_THRESHOLD)
                continue;

            vec3 repPos = texelFetch(gPosition, representative, 0).xyz;
            float planeDistance = dot(normal, repPos - pos);
            float planeWeight = exp(-planeDistance * planeDistance / (SDF_UPSAMPLE_PLANE_SIGMA * SDF_UPSAMPLE_PLANE_SIGMA));
            float normalWeight = pow(max(dot(normal, normalize(repNormal.xyz)), 0.0), 8.0);

            float weight = bilinear * planeWeight * normalWeight;
            sum += weight * texelFetch(sdfOcclusion, texel, 0).g;
            weightSum += weight;
        }
    }
    return weightSum >= SDF_UPSAMPLE_MIN_WEIGHT ? sum / weightSum : -1.0;
}

void main() {
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 Normal = texture(gNormal, TexCoords).rgb;
    vec3 Albedo = texture(gAlbedoRoughness, TexCoords).rgb;
    float Roughness = texture(gAlbedoRoughness, TexCoords).a;
    float Metallic = texture(gNormal, TexCoords).a;

    if (debugMode == 0) {
        // ---- 通常のライティング ----
        if (dot(Normal, Normal) < 0.5) {
            FragColor = vec4(0.0, 0.0, 0.0, 1.0);
            BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
            return;
        }
        float AmbientOcclusion =
            mix(1.0, texture(ssao, TexCoords).r, ssaoStrength);

        vec3 viewDir = normalize(viewPos - FragPos);

        // 非金属は一律 0.04 金属は鏡面反射がアルベドの色を持つ
        vec3 F0 = mix(vec3(0.04), Albedo, Metallic);

        // 環境光は IBL から ループの外で1回だけ求める
        float NdotV = max(dot(Normal, viewDir), 0.0);
        vec3 kS = fresnelSchlickRoughness(NdotV, F0, Roughness);
        vec3 kD = (1.0 - kS) * (1.0 - Metallic);

        // SSAO はcm SDF は m で守備範囲が違うので両方をかける
        float skyVisibility = mix(1.0, texture(sdfOcclusion, TexCoords).r, sdfOcclusionStrength);
        vec3 diffuseIBL = texture(irradianceMap, Normal).rgb * Albedo * skyVisibility;

        // 反射方向の環境光を roughness に応じたミップから引き LUT で反射率を補正する
        vec3 R = reflect(-viewDir, Normal);
        vec3 prefiltered = textureLod(prefilterMap, R, Roughness * MAX_REFLECTION_LOD).rgb;
        vec2 brdf = texture(brdfLUT, vec2(NdotV, Roughness)).rg;

        vec3 iblSpecularWeight = kS * brdf.x + brdf.y;
        bool isFloor = hasReflection && abs(FragPos.y - sceneParams.x) < FLOOR_PLANE_EPSILON && Normal.y > 0.9;
        // 反射像だけを使う床では specVisibility が不要なのでトレースごと省く
        bool needsSpecVisibility = !isFloor || floorSdfSpecularOcclusion;

        float specVisibility = 1.0;
        if (needsSpecVisibility) {
            // Roughness が大きいものは specVisibility を計算してもあまりメリットがない
            if (Roughness < SDF_ROUGHNESS_THRESHOLD) {
                float specConeTangent = Roughness * Roughness;
                if (sdfOcclusionStrength > 0.0) { // 処理速度向上のための分岐
                    float traced = upsampleSpecularVisibility(FragPos, normalize(Normal), ivec2(gl_FragCoord.xy));
                    // 同じ面の代表点が無い画素（細い物体や輪郭）は 半解像度の値を使わずここでトレースし直す
                    if (traced < 0.0) {
                        // 鏡面反射は遠くの壁も映り込む必要があるので AO 用の短い tMax ではなくシーン全体を抜ける距離を使う
                        traced =
                            sdfEnvVisibility(FragPos, normalize(Normal), normalize(R), specConeTangent, sceneParams.w);
                    }
                    specVisibility = mix(1.0, traced, sdfOcclusionStrength);
                }
            } else
                // そもそも roughness が大きいなら広い範囲を平均している skyVisibility で済む
                specVisibility = skyVisibility;
        }

        vec3 specularIBL;
        if (isFloor) {
            // 遮蔽で暗くする代わりに その方向を実際に映る色へ置き換える
            vec3 reflected = textureLod(reflectionColor, TexCoords, Roughness * MAX_REFLECTION_COLOR_LOD).rgb;
            // 反射像は遮蔽込みの実測なので重ねると二重に遮ることになる
            if (floorSdfSpecularOcclusion)
                reflected = mix(reflected, prefiltered, specVisibility);
            specularIBL = reflected * iblSpecularWeight;
        } else {
            specularIBL = prefiltered * iblSpecularWeight * specVisibility;
        }

        // kD が掛かるのは拡散だけ 鏡面は LUT 経由で kS を内包している
        vec3 result = (kD * diffuseIBL + specularIBL) * AmbientOcclusion * ambientStrength;

        for (int i = 0; i < NR_LIGHTS; ++i) {
            // PCF で複数回サンプリングする ShadowCalculation() の手前で弾く
            float dist = length(pointLights[i].position - FragPos);
            if (dist >= pointLights[i].radius)
                continue;

            vec3 lightDir = normalize(pointLights[i].position - FragPos);
            // NdotL が 0 以下では shadow は寄与しないので早期にはじく
            if (dot(Normal, lightDir) <= 0.0)
                continue;
            float shadow = ShadowCalculation(FragPos, Normal, lightDir,
                    pointLights[i].position, shadowMap[i]);
            if (sdfShadowStrength > 0.0 && shadow < 1.0) { // 処理効率工場のための条件
                float sdfShadow = 1.0 - sdfLightVisibility(FragPos, normalize(Normal), pointLights[i].position,
                            pointLights[i].sourceRadius);
                shadow = max(shadow, sdfShadow * sdfShadowStrength);
            }
            // 窓枠は shadow≈1 で黒い影 ガラスは shadow=0 のままここで色付きに減衰する
            vec3 transmit =
                texture(shadowColor[i], FragPos - pointLights[i].position).rgb;
            result += directLightStrength * transmit * CalcPointLight(pointLights[i], Normal, FragPos, viewDir,
                        Albedo, Roughness, Metallic, F0, shadow);
        }

        FragColor = vec4(result, 1.0);

        // 明るいピクセルだけを BrightColor に残し Bloom の素材にする
        float brightness = dot(result, vec3(0.2126, 0.7152, 0.0722));
        if (brightness > 1.0)
            BrightColor = vec4(result, 1.0);
        else
            BrightColor = vec4(0.0, 0.0, 0.0, 1.0);

        return;
    }

    // ---- デバッグ表示 ----
    // Bloom が乗ると判定できなくなるので デバッグ中は BrightColor を常に黒にする
    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);

    if (debugMode == 1) {
        // 必ず1灯だけで見ること 4灯を max() でまとめるとほぼ全面が白くなり判定できない
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
        // 上向きの面が空色 下向きが地面色になっていれば畳み込みは成功
        FragColor = vec4(texture(irradianceMap, Normal).rgb, 1.0);
    } else if (debugMode == 12) {
        // roughness を上げるほど映り込みがぼけていけば正常
        vec3 R = reflect(-normalize(viewPos - FragPos), Normal);
        FragColor = vec4(textureLod(prefilterMap, R, Roughness * MAX_REFLECTION_LOD).rgb, 1.0);
    } else if (debugMode == 13) {
        // 画面全体に LUT を貼る 左下が暗く右上が明るい赤緑のグラデーションが正解
        FragColor = vec4(texture(brdfLUT, TexCoords).rg, 0.0, 1.0);
    } else if (debugMode == 14) {
        // SDF関数のデバッグ
        if (dot(Normal, Normal) < 0.5) { // オブジェクトと HDR を区別して処理
            FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        } else {
            FragColor = vec4(vec3(texture(sdfOcclusion, TexCoords).r), 1.0);
        }
    } else if (debugMode == 15) {
        // ソフトシャドウのデバッグ
        if (dot(Normal, Normal) < 0.5) {
            FragColor = vec4(0.0, 0.0, 0.0, 0.0);
        } else {
            float visibility =
                sdfLightVisibility(FragPos, normalize(Normal), pointLights[0].position, pointLights[0].sourceRadius);
            FragColor = vec4(vec3(visibility), 1.0);
        }
    } else if (debugMode == 16) {
        // AO パスのステップ数を正規化して表示 白いほど SDF_MAX_STEPS に近い（重い）
        if (dot(Normal, Normal) < 0.5) {
            FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        } else {
            FragColor = vec4(vec3(texture(sdfOcclusion, TexCoords).r), 1.0);
        }
    } else if (debugMode == 17 || debugMode == 18) {
        // 鏡面のコーントレースの生の結果 17 は可視性 18 は使ったステップ数
        if (dot(Normal, Normal) < 0.5) {
            FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        } else {
            vec3 viewDir = normalize(viewPos - FragPos);
            vec3 R = reflect(-viewDir, Normal);
            int steps;
            float visibility = sdfEnvVisibility(FragPos, normalize(Normal), normalize(R),
                    Roughness * Roughness, sceneParams.w, steps);
            float value = debugMode == 17 ? visibility : float(steps) / float(SDF_MAX_STEPS);
            FragColor = vec4(vec3(value), 1.0);
        }
    } else if (debugMode == 19) {
        // 実際にライティングが使う鏡面の可視性（半解像度＋補間） 17 と見比べる 赤はトレースし直した画素
        if (dot(Normal, Normal) < 0.5) {
            FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        } else if (Roughness >= SDF_ROUGHNESS_THRESHOLD) {
            FragColor = vec4(0.0, 0.0, 1.0, 1.0); // 粗い画素は鏡面をトレースしない 青
        } else {
            float visibility = upsampleSpecularVisibility(FragPos, normalize(Normal), ivec2(gl_FragCoord.xy));
            FragColor = visibility < 0.0 ? vec4(1.0, 0.0, 0.0, 1.0) : vec4(vec3(visibility), 1.0);
        }
    } else {
        FragColor = vec4(1.0, 0.0, 1.0, 1.0); // 未定義の debugMode（マゼンタ）
    }
}
