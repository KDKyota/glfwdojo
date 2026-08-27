// #include される側なので #version は書かない
// ライト関連の uniform はここが唯一の宣言場所。include する側では宣言しないこと
#ifndef SHADOW_COMMON_GLSL
#define SHADOW_COMMON_GLSL

const int NR_LIGHTS = 4;

struct PointLight {
    vec3 position;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    // PointLight::calcRadius() が減衰式から逆算して送ってくる
    float radius;
};

uniform PointLight pointLights[NR_LIGHTS];
uniform samplerCube shadowMap[NR_LIGHTS];
// ガラスを透過した光の色。ガラスを通らない方向は白
uniform samplerCube shadowColor[NR_LIGHTS];
uniform float farPlane;
// ぼかし量とバイアスをテクセル単位で決めるため、面の一辺の解像度を受け取る
uniform float shadowMapSize;

const int PCF_SAMPLE_COUNT = 16;
// 以下3つがぼけ具合と acne / peter-panning のトレードオフを決める
const float PCF_RADIUS_TEXELS = 2.5;
const float NORMAL_OFFSET_TEXELS = 2.0;
const float DEPTH_BIAS_TEXELS = 1.0;

// 単位円盤上のポアソンディスク
// 等間隔の格子より少ないサンプル数で均一に散る（モデル化されている変数を利用）
const vec2 poissonDisk[16] =
    vec2[](vec2(-0.942016, -0.399062), vec2(0.945586, -0.768907),
           vec2(-0.094184, -0.929389), vec2(0.344959, 0.293878),
           vec2(-0.915886, 0.457714), vec2(-0.815442, -0.879125),
           vec2(-0.382775, 0.276768), vec2(0.974844, 0.756484),
           vec2(0.443233, -0.975116), vec2(0.537430, -0.473734),
           vec2(-0.264969, -0.418930), vec2(0.791975, 0.190902),
           vec2(-0.241888, 0.997065), vec2(-0.814100, 0.914376),
           vec2(0.199841, 0.786414), vec2(0.143832, -0.141008));

// 固定パターンのままだと縞が残るので、ピクセルごとに回してノイズに変える （Interleaved Gradient Noize(IGN)というらしい）
// 基本の単位でずらすのではなくテクセル単位でずらす
float PcfRotationAngle(vec2 fragCoord) {
    float noise = fract(52.9829189 * fract(dot(fragCoord, vec2(0.06711056, 0.00583715))));
    return noise * 6.28318530;
}

// fragPos が影の中にあるかを [0, 1] で返す（0=完全に照らされる, 1=完全な影）。
float ShadowCalculation(vec3 fragPos, vec3 normal, vec3 lightDir, vec3 lightPos,
                        samplerCube shadowMap) {
    float distToLight = length(fragPos - lightPos);
    // 面の FOV が 90° 固定なので、この距離でテクセル1個が覆う実寸はこう!
    float texelWorld = distToLight * 2.0 / shadowMapSize;
    float slope = clamp(1.0 - dot(normal, lightDir), 0.0, 1.0);

    // 深度をずらす代わりに参照位置を法線方向へ逃がす。peter-panning が出にくい
    vec3 fragToLight =
        fragPos + normal * texelWorld * NORMAL_OFFSET_TEXELS * (1.0 + 2.0 * slope) - lightPos;
    float currentDepth = length(fragToLight);
    // 下限は 16bit 深度の量子化幅（farPlane 50m で約 0.8mm）を必ず上回るための保険
    float bias = max(texelWorld * DEPTH_BIAS_TEXELS, 0.005);

    // samplerCube は方向しか見ないので、fragToLight に平行な成分を足しても同じテクセルに落ちる
    vec3 dir = fragToLight / currentDepth;
    vec3 up = abs(dir.y) < 0.99 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0); // 光源からのベクトルと up が平行にならないようにする
    /* fragToLight と up が平行になるとtangent, bitangent での外積の結果が 0 になったりするので上の行のように対策している
     * 一応 dir.y < 0.99 になるときは tangent と bitangent の向きが変わってしまうが晴れる平面は同じなので問題ない
     */
    vec3 tangent = normalize(cross(up, dir));
    vec3 bitangent = cross(dir, tangent);

    float angle = PcfRotationAngle(gl_FragCoord.xy);
    float sinA = sin(angle);
    float cosA = cos(angle);
    // 距離に比例させることで、ぼかし幅がどの距離でもテクセル単位で一定になる
    float radius = texelWorld * PCF_RADIUS_TEXELS; // 2.5 テクセル分

    float shadow = 0.0;
    for (int i = 0; i < PCF_SAMPLE_COUNT; ++i) {
        vec2 disk = poissonDisk[i];
        vec2 rotated = vec2(disk.x * cosA - disk.y * sinA, disk.x * sinA + disk.y * cosA);     // 原点周りに A だけ回転
        vec3 sampleDir = fragToLight + (tangent * rotated.x + bitangent * rotated.y) * radius; // ssao.frag の samplePos と同じ
        float closestDepth = texture(shadowMap, sampleDir).r * farPlane;
        if (currentDepth - bias > closestDepth)
            shadow += 1.0;
    }
    return shadow / float(PCF_SAMPLE_COUNT);
}

#endif
