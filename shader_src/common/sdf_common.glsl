// SDF (Signed Distance Function) レイマーチ用の共通関数
#ifndef SDF_COMMON_GLSL
#define SDF_COMMON_GLSL

#include "pbr_common.glsl"

// 配列長は Scene.h の kSdfMaxBoxes と一致させること
const int SDF_MAX_BOXES = 8;
// 配列長は SdfOcclusionPass.cpp の kSdfMaxModels と一致させること
const int SDF_MAX_MODELS = 4;

layout(std140, binding = 2) uniform SdfScene {
    vec4 boxCenters[SDF_MAX_BOXES];
    vec4 wallCenters[2];
    vec4 boxHalfSize;
    vec4 wallHalfSize;
    vec4 sceneParams; // x: floorY, y: sizeof cubePositions, z: 2.0, w: floor's size
    // 静的メッシュの距離場ぶん　先頭から modelCounts.x 個だけが有効で それ以降の枠は読まない
    mat4 modelWorldToLocalMatrices[SDF_MAX_MODELS]; // ワールド座標をそのメッシュのローカル座標へ変換する(ワールド変換の逆行列)
    vec4 modelBoundsMin[SDF_MAX_MODELS];
    vec4 modelBoundsMax[SDF_MAX_MODELS];
    ivec4 modelCounts; // x: 有効な静的メッシュ距離場の数
};

// UBO には入れられないので 普通の uniform として別に受け取っている
uniform sampler3D modelDistanceFields[SDF_MAX_MODELS];

const int SDF_MAX_STEPS = 48;

const float SDF_STEP_HEATMAP_REF = 96.0;
const float SDF_HIT_EPSILON = 0.002; // 衝突の閾値
const float SDF_NORMAL_BIAS = 0.02; // 自己交差になるのを防ぐバイアス
// 半球の立体角 2π を N 本で負担するので 1 本当たり 2π/N
// cosΘ = 1 - 1/N
// N = 8 なので $\theta = 28.9° -> tan\theta = 0.55$
const float SDF_DIFFUSE_CONE_TANGENT = 0.55; // 1 本が受け持つ立体角に相当する太さ

const float SDF_DISCONTINUE_DIST = 8.0; // 計算効率のために SDF の計算を打ち切る距離

const float SDF_RES_THRESHOLD = 0.02; // これ以下まで res が落ちたら遮蔽とみなして打ち切る値

const float SDF_RANGE_FADE_RATIO = 0.3; // 打ち切り距離の手前 何割から遮蔽をフェードさせるか

// この粗さ以上の画素は鏡面のトレースをしない 半解像度パスと Lighting パスで同じ画素を対象にするため共有する
const float SDF_ROUGHNESS_THRESHOLD = 0.7;

// 打ち切り距離に近い遮蔽ほど寄与を下げる重み
// これがないと最後の1歩が tMax を跨ぐかどうかで res が跳ね縞模様が出る 
// faceRange: tMax の手前どのくらいの距離からフェードアウトを始めるのか
float sdfRangeWeight (float t, float tMax, float fadeRange) {
    // smoothstep は edge0 < edge1 でないと未定義なので反転させてから 1 から引く
    return 1.0 - smoothstep(tMax - max(fadeRange, 1e-5), tMax, t); // max の 1e-5 としているのはsmoothstep の第1,2引数が同じ値んあるのを防ぐため
}

// 点から箱までの最短距離距離を計算
float sdBox (vec3 p, vec3 center, vec3 halfSize) {
    vec3 d = abs(p - center) - halfSize;
    return length(max(d, 0.0)) + min(max(d.x, max(d.y, d.z)), 0.0);
}

// 解析的な形状（床・壁・キューブ）までの距離　厳密なので歩幅にも遮蔽にもそのまま使える
float analyticSDF (vec3 p) {
    float dist = p.y - sceneParams.x;
    // int boxCount = int(sceneParams.y);
    for (int i = 0; i < SDF_MAX_BOXES; ++i) {
        dist = min(dist, sdBox(p, boxCenters[i].xyz, boxHalfSize.xyz));
    }
    //int wallCount = int(sceneParams.z);
    for (int i = 0; i < 2; ++i) {
        dist = min(dist, sdBox(p, wallCenters[i].xyz, wallHalfSize.xyz));
    }
    return dist;
}

// レイをモデル i のローカル空間へ移し AABB と交わる区間 [tNear, tFar] を返す
bool modelRayInterval (int i, vec3 origin, vec3 dir, float tMax, out vec3 localOrigin, out vec3 localDir,
                       out float tNear, out float tFar) {
    localOrigin = (modelWorldToLocalMatrices[i] * vec4(origin, 1.0)).xyz;
    localDir = mat3(modelWorldToLocalMatrices[i]) * dir;

    // 軸に平行な成分が 0 除算になるのを避ける
    vec3 safeDir = mix(localDir, vec3(1e-9), lessThan(abs(localDir), vec3(1e-9)));
    vec3 tA = (modelBoundsMin[i].xyz - localOrigin) / safeDir;
    vec3 tB = (modelBoundsMax[i].xyz - localOrigin) / safeDir;
    vec3 tEnter = min(tA, tB);
    vec3 tExit = max(tA, tB);

    tNear = max(max(tEnter.x, max(tEnter.y, tEnter.z)), 0.0);
    tFar = min(min(tExit.x, min(tExit.y, tExit.z)), tMax);
    return tNear <= tFar;
}

// モデル i の距離場を レイが AABB を通る区間だけコーントレースする
// 注意：距離場は AABB の中にしか無く 外側で返せるのは箱の形の値だけなので 区間外を評価すると AABB の形が遮蔽に焼き付く
float coneVisibilityModel (int i, vec3 origin, vec3 dir, float coneTangent, float tMax, float fadeRange, inout int steps) {
    vec3 localOrigin, localDir;
    float t, tFar;
    if (!modelRayInterval(i, origin, dir, tMax, localOrigin, localDir, t, tFar))
        return 1.0;

    float res = 1.0;
    float prevD = 1e20;
    for (int step = 0; step < SDF_MAX_STEPS; ++step) {
        if (t > tFar)
            return res;

        vec3 localP = localOrigin + localDir * t;
        vec3 uvw = (localP - modelBoundsMin[i].xyz) / (modelBoundsMax[i].xyz - modelBoundsMin[i].xyz);
        float d = texture(modelDistanceFields[i], uvw).r;
        ++steps;
        if (d < SDF_HIT_EPSILON)
            return min(res, 1.0 - sdfRangeWeight(t, tMax, fadeRange));

        float y = d * d / (2.0 * prevD);
        float tClosest = t - y;
        // y > d は厳密な距離場では起こらず 焼いた距離場の補間誤差で歩幅より遠くへ跳んだ印なので 遮蔽ではなく推定不能として捨てる
        if (tClosest > 0.0 && y <= d) {
            float closest = sqrt(max(d * d - y * y, 0.0));
            float visibility = closest / max(tClosest * coneTangent, 1e-4);
            res = min(res, mix(1.0, visibility, sdfRangeWeight(tClosest, tMax, fadeRange)));
            if (res <= SDF_RES_THRESHOLD)
                return 0.0;
        }

        prevD = d;
        t += d;
    }
    return res;
}

// 解析的な形状だけをコーントレースする
float coneVisibilityAnalytic (vec3 origin, vec3 dir, float coneTangent, float tMax, float fadeRange,
                              float startPhase, inout int steps) {
    float res = 1.0;
    float t = 0.0;
    float prevD = 1e20;
    for (int step = 0; step < SDF_MAX_STEPS; ++step) {
        float d = analyticSDF(origin + dir * t);
        ++steps;
        if (d < SDF_HIT_EPSILON)
            return min(res, 1.0 - sdfRangeWeight(t, tMax, fadeRange));

        float y = d * d / (2.0 * prevD);
        float tClosest = t - y;
        // 最接近点が出発点より手前なら面から離れていく途中なので判定しない
        if (tClosest > 0.0) {
            float closest = sqrt(max(d * d - y * y, 0.0));
            float visibility = closest / max(tClosest * coneTangent, 1e-4);
            res = min(res, mix(1.0, visibility, sdfRangeWeight(tClosest, tMax, fadeRange)));
            // 打ち切り時の res は最後のサンプル位置で一桁振れる HDR だと波紋になるので 0 に倒す
            if (res <= SDF_RES_THRESHOLD)
                return 0.0;
        }

        prevD = d;
        // 最初の一歩だけ歩幅を変えることで 何歩目でどの距離を測るかの位相をずらす
        t += (step == 0) ? d * startPhase : d;
        if (t > tMax)
            return res;
    }
    // ステップ切れは面すれすれを這っている状態なのでヒットと同じ扱いにする
    return min(res, 1.0 - sdfRangeWeight(t, tMax, fadeRange));
}

// 距離 d を進み面との衝突を検出
float sdfVisibility (vec3 pos, vec3 normal, vec3 dir, out int steps) {
    vec3 origin = pos + normal * SDF_NORMAL_BIAS; // 現在地点
    steps = 0;
    float t = 0.0;
    for (int step = 0; step < SDF_MAX_STEPS && t <= SDF_DISCONTINUE_DIST; ++step) {
        float d = analyticSDF(origin + dir * t);
        ++steps;
        if (d < SDF_HIT_EPSILON)
            return 0.0;
        t += d;
    }

    for (int i = 0; i < modelCounts.x; ++i) {
        vec3 localOrigin, localDir;
        float tModel, tFar;
        if (!modelRayInterval(i, origin, dir, SDF_DISCONTINUE_DIST, localOrigin, localDir, tModel, tFar))
            continue;
        for (int step = 0; step < SDF_MAX_STEPS && tModel <= tFar; ++step) {
            vec3 localP = localOrigin + localDir * tModel;
            vec3 uvw = (localP - modelBoundsMin[i].xyz) / (modelBoundsMax[i].xyz - modelBoundsMin[i].xyz);
            float d = texture(modelDistanceFields[i], uvw).r;
            ++steps;
            if (d < SDF_HIT_EPSILON)
                return 0.0;
            tModel += d;
        }
    }
    return 1.0;
}

// 注意：coneTangent が実質的な円錐の角度になるので、0 だと実質的に線になる
// コーントレースで途中での最小距離を返す steps に実際に使ったステップ数を書き出す
// startPhase: 最初の一歩だけ歩幅を伸縮させて 歩数の位相をずらすための係数（1.0 で無効）
//  各距離場を有効範囲の外で評価しないようにするために形状ごとに独立してトレースし 可視性を min で合成する
float sdfConeVisibility(vec3 pos, vec3 normal, vec3 dir, float coneTangent, float tMax, float fadeRange, float startPhase, out int steps) { // `tMax` は tをどこまで伸ばしたら打ち切るかという値
    vec3 origin = pos + normal * SDF_NORMAL_BIAS;
    steps = 0;
    float res = coneVisibilityAnalytic(origin, dir, coneTangent, tMax, fadeRange, startPhase, steps);
    for (int i = 0; i < modelCounts.x; ++i) {
        res = min(res, coneVisibilityModel(i, origin, dir, coneTangent, tMax, fadeRange, steps));
    }
    return res;
}

// ステップ数を必要としない呼び出し元向けのラッパー
float sdfConeVisibility(vec3 pos, vec3 normal, vec3 dir, float coneTangent, float tMax, float fadeRange) {
    int steps;
    return sdfConeVisibility(pos, normal, dir, coneTangent, tMax, fadeRange, 1.0, steps);
}

// 空へ抜けるレイ用 打ち切り距離の手前の遮蔽をフェードさせて縞を防ぐ
// 位相の異なる2本を平均する 
float sdfEnvVisibility(vec3 pos, vec3 normal, vec3 dir, float coneTangent, float tMax, out int steps) {
    float fadeRange = tMax * SDF_RANGE_FADE_RATIO;
    int stepsB;
    // 最初の一歩を 0.25 と 0.75 にしたものでコーントレーシング
    float a = sdfConeVisibility(pos, normal, dir, coneTangent, tMax, fadeRange, 0.25, steps);
    float b = sdfConeVisibility(pos, normal, dir, coneTangent, tMax, fadeRange, 0.75, stepsB);
    steps = max(steps, stepsB);
    return (a + b) * 0.5;
}

float sdfEnvVisibility(vec3 pos, vec3 normal, vec3 dir, float coneTangent, float tMax) {
    int steps;
    return sdfEnvVisibility(pos, normal, dir, coneTangent, tMax, steps);
}

// レイが光源で終わるので打ち切りによる縞は起きない フェードは掛けない
float sdfLightVisibility(vec3 pos, vec3 normal, vec3 lightPos, float sourceRadius) {
    vec3 toLight = lightPos - (pos + normal * SDF_NORMAL_BIAS);
    float distToLight = length(toLight);
    return sdfConeVisibility(pos, normal, toLight / distToLight, sourceRadius / distToLight, distToLight, 0.0);
}

const int SDF_HEMISPHERE_SAMPLES = 4;
// Normal 周りの半球を cosine 重みでサンプル詩平均化姿勢を返す maxSteps に 8 方向中の最大ステップ数を書き出す
float sdfSkyVisibility ( vec3 pos, vec3 normal, vec2 rotation, out int maxSteps) {
    // ここでの up はワールドの上ではなく cross がゼロにならないためのもの
    vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 t0 = normalize(cross(up, normal));
    vec3 b0 = cross(normal, t0);
    vec3 tangent = t0 * rotation.x + b0 * rotation.y;
    vec3 bitangent = -t0 * rotation.y + b0 * rotation.x;

    float visible = 0.0;
    maxSteps = 0;
    for (int i = 0; i < SDF_HEMISPHERE_SAMPLES; ++i) {
        vec2 xi = Hammersley(uint(i), uint(SDF_HEMISPHERE_SAMPLES));
        float phi = 2.0 * PI * xi.x;
        float cosTheta = sqrt(1.0 - xi.y);
        float sinTheta = sqrt(xi.y);
        vec3 local = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
        vec3 dir = tangent * local.x + bitangent * local.y + normal * local.z;

        int steps;
        visible += sdfEnvVisibility(pos, normal, dir, SDF_DIFFUSE_CONE_TANGENT, SDF_DISCONTINUE_DIST, steps);
        maxSteps = max(maxSteps, steps);
    }
    return visible / float(SDF_HEMISPHERE_SAMPLES);
}

float sdfSkyVisibility ( vec3 pos, vec3 normal, vec2 rotation) {
    int maxSteps;
    return sdfSkyVisibility(pos, normal, rotation, maxSteps);
}


#endif
