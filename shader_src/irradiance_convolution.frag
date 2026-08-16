// 環境マップを半球にわたって畳み込み、拡散反射用の irradiance を求める
// キューブマップは動かないため，初回の一度だけ実行する
#version 460 core

in vec3 LocalPos;
out vec4 FragColor;

uniform samplerCube environmentMap;

const float PI = 3.14159265359;

void main() {
    // 面の法線＝この方向。この向きの半球から来る光を全部集める
    vec3 N = normalize(LocalPos);

    // N を軸とする接空間。N が真上/真下だと cross が 0 になり normalize が NaN を返すので、
    // そのときだけ別の軸を使う（半球積分は N まわりの回転に対して不変なので結果は変わらない）
    vec3 up = abs(N.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(0.0, 0.0, 1.0);
    vec3 right = normalize(cross(up, N));
    up = normalize(cross(N, right));

    vec3 irradiance = vec3(0.0);
    float nrSamples = 0.0;
    const float sampleDelta = 0.025;

    for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta) {
        for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta) {
            vec3 tangentSample =
                vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            vec3 sampleVec =
                tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;

            // sin(theta) は球面の面積要素、cos(theta) は入射角による減衰
            irradiance += texture(environmentMap, sampleVec).rgb * cos(theta) * sin(theta);
            nrSamples++;
        }
    }
    irradiance = PI * irradiance * (1.0 / nrSamples);

    FragColor = vec4(irradiance, 1.0);
}
