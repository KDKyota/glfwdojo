// SDF (Signed Distance Function) レイマーチ用の共通関数
#ifndef SDF_COMMON_GLSL
#define SDF_COMMON_GLSL

#include "pbr_common.glsl"

// TODO: Scene.h の cubePosition_ と SceneUnits.h の写し 検証後に UBO へ流す
// 厚さゼロの板ポリに厚さをつける
const int SDF_WALL_COUNT = 2;
const vec3 sdfWallCenters[2] = vec3[](vec3(0.0, 4.75, -25.0), vec3(0.0, 4.75, 25.0));
const vec3 SDF_WALL_HALF_SIZE = vec3(25.0, 5.25, 0.1);

const int SDF_BOX_COUNT = 7;
const vec3 sdfBoxCenters[7] = 
    vec3[](vec3(-1.0, 0.0, -1.0), 
    vec3(2.0, 0.0, 0.0),
    vec3(0.0, 0.0, 2.5),
    vec3(1.2, 0.0, 2.5),
    vec3(-1.0, 1.0, -1.0),
    vec3(0.0, 0.0, -20.0),
    vec3(0.0, 0.0, 20.0));

const vec3 SDF_BOX_HALF_SIZE = vec3(0.5);
const float SDF_FLOOR_Y = -0.5;

const float SDF_MAX_DIST = 50.0;
const int SDF_MAX_STEPS = 96;
const float SDF_HIT_EPSILON = 0.002; // 衝突の閾値
const float SDF_NORMAL_BIAS = 0.02; // 自己交差になるのを防ぐバイアス

// 点から箱までの最短距離距離を計算
float sdBox (vec3 p, vec3 center, vec3 halfSize) {
    vec3 d = abs(p - center) - halfSize;
    return length(max(d, 0.0)) + min(max(d.x, max(d.y, d.z)), 0.0);
}

// シーン全体で最も近い面までの距離を計算
float sceneSDF (vec3 p) {
    float dist = p.y - SDF_FLOOR_Y; 
    for (int i = 0; i < SDF_BOX_COUNT; ++i) {
        dist = min(dist, sdBox(p, sdfBoxCenters[i], SDF_BOX_HALF_SIZE));
    }
    for (int i = 0; i < SDF_WALL_COUNT; ++i) { 
        dist = min(dist, sdBox(p, sdfWallCenters[i], SDF_WALL_HALF_SIZE));
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
        if (t > SDF_MAX_DIST) 
            return 1.0;
    }
    return 1.0;
}

const int SDF_HEMISPHERE_SAMPLES = 16;
// Normal 周りの半球を cosine 重みでサンプル詩平均化姿勢を返す
float sdfSkyVisibility ( vec3 pos, vec3 normal) {
    vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, normal));
    vec3 bitangent = cross(normal, tangent);

    float visible = 0.0;
    for (int i = 0; i < SDF_HEMISPHERE_SAMPLES; ++i) {
        vec2 xi = Hammersley(uint(i), uint(SDF_HEMISPHERE_SAMPLES));
        float phi = 2.0 * PI * xi.x;
        float cosTheta = sqrt(1.0 - xi.y);
        float sinTheta = sqrt(xi.y);
        vec3 local = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
        vec3 dir = tangent * local.x + bitangent * local.y + normal * local.z;

        int steps;
        visible += sdfVisibility(pos, normal, dir, steps);
    }
    return visible / float(SDF_HEMISPHERE_SAMPLES);
}

#endif
