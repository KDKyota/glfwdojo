// SDF (Signed Distance Function) レイマーチ用の共通関数
#ifndef SDF_COMMON_GLSL
#define SDF_COMMON_GLSL

#include "pbr_common.glsl"

// 配列長は Scene.h の kSdfMaxBoxes と一致させること
const int SDF_MAX_BOXES = 8;

layout(std140, binding = 2) uniform SdfScene {
    vec4 boxCenters[SDF_MAX_BOXES];
    vec4 wallCenters[2];
    vec4 boxHalfSize;
    vec4 wallHalfSize;
    vec4 sceneParams; // x: floorY, y: sizeof cubePositions, z: 2.0, w: floor's size
};

const int SDF_MAX_STEPS = 48;

const float SDF_STEP_HEATMAP_REF = 96.0;
const float SDF_HIT_EPSILON = 0.002; // 衝突の閾値
const float SDF_NORMAL_BIAS = 0.02; // 自己交差になるのを防ぐバイアス
// 半球の立体角 2π を N 本で負担するので 1 本当たり 2π/N
// cosΘ = 1 - 1/N
// N = 8 なので $\theta = 28.9° -> tan\theta = 0.55$
const float SDF_DIFFUSE_CONE_TANGENT = 0.55; // 1 本が受け持つ立体角に相当する太さ

const float SDF_DISCONTINUE_DIST = 8.0; // 計算効率のために SDF の計算を打ち切る距離

const float SDF_RES_THRESHOLD = 0.02; // res がこれ以上小さくならないであろう値を閾値とする

// 点から箱までの最短距離距離を計算
float sdBox (vec3 p, vec3 center, vec3 halfSize) {
    vec3 d = abs(p - center) - halfSize;
    return length(max(d, 0.0)) + min(max(d.x, max(d.y, d.z)), 0.0);
}

// シーン全体で最も近い面までの距離を計算
float sceneSDF (vec3 p) {
    float dist = p.y - sceneParams.x;
    int boxCount = int(sceneParams.y);
    for (int i = 0; i < boxCount; ++i) {
        dist = min(dist, sdBox(p, boxCenters[i].xyz, boxHalfSize.xyz));
    }
    int wallCount = int(sceneParams.z);
    for (int i = 0; i < wallCount; ++i) {
        dist = min(dist, sdBox(p, wallCenters[i].xyz, wallHalfSize.xyz));
    }
    return dist;
}

// 距離 d を進み面との衝突を検出
float sdfVisibility (vec3 pos, vec3 normal, vec3 dir, out int steps) {
    vec3 origin = pos + normal * SDF_NORMAL_BIAS; // 現在地点
    float t = 0.0;
    for (steps = 0; steps < SDF_MAX_STEPS; ++steps) {
        float d = sceneSDF(origin + dir * t);
        if (d < SDF_HIT_EPSILON)
            return 0.0;
        t += d;
        if (t > SDF_DISCONTINUE_DIST)
            return 1.0;
    }
    return 0.0;
}

// 注意：coneTangent が実質的な円錐の角度になるので、0 だと実質的に線になる
// コーントレースで途中での最小距離を返す steps に実際に使ったステップ数を書き出す
float sdfConeVisibility(vec3 pos, vec3 normal, vec3 dir, float coneTangent, float tMax, out int steps) { // `tMax` は tをどこまで伸ばしたら打ち切るかという値
    vec3 origin = pos + normal * SDF_NORMAL_BIAS;
    float res = 1.0;
    float t = 0.0;
    float prevD = 1e20;
    for (steps = 0; steps < SDF_MAX_STEPS; ++steps) {
        float d = sceneSDF(origin + dir * t);
        if (d < SDF_HIT_EPSILON) return 0.0;

        // 止まった地点だけで測ると最接近点を取り逃がして縞が出る
        float y = d * d / (2.0 * prevD);
        float tClosest = t - y;
        // 最接近点が出発点より手前なら面から離れていく途中なので判定しない
        if (tClosest > 0.0) {
            float closest = sqrt(max(d * d - y * y, 0.0));
            res = min(res, closest / max(tClosest * coneTangent, 1e-4));
            if (res <= SDF_RES_THRESHOLD) return res;
        }

        prevD = d;
        t += d;
        if (t > tMax) return res;
    }
    return 0.0;
}

// ステップ数を必要としない呼び出し元向けの薄いラッパー
float sdfConeVisibility(vec3 pos, vec3 normal, vec3 dir, float coneTangent, float tMax) {
    int steps;
    return sdfConeVisibility(pos, normal, dir, coneTangent, tMax, steps);
}

float sdfLightVisibility(vec3 pos, vec3 normal, vec3 lightPos, float sourceRadius) {
    vec3 toLight = lightPos - (pos + normal * SDF_NORMAL_BIAS);
    float distToLight = length(toLight);
    return sdfConeVisibility(pos, normal, toLight / distToLight, sourceRadius / distToLight, distToLight);
}

const int SDF_HEMISPHERE_SAMPLES = 8;
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
        visible += sdfConeVisibility(pos, normal, dir, SDF_DIFFUSE_CONE_TANGENT, SDF_DISCONTINUE_DIST, steps);
        maxSteps = max(maxSteps, steps);
    }
    return visible / float(SDF_HEMISPHERE_SAMPLES);
}

float sdfSkyVisibility ( vec3 pos, vec3 normal, vec2 rotation) {
    int maxSteps;
    return sdfSkyVisibility(pos, normal, rotation, maxSteps);
}


#endif
