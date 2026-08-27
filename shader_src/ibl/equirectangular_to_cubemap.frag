// 横長1枚（正距円筒図法）の HDR をキューブマップの6面へ焼き直す。起動時に1回だけ
#version 460 core

in vec3 LocalPos;
out vec4 FragColor;

uniform sampler2D equirectangularMap;

// 1/(2π), 1/π。方向ベクトルの角度を [0,1] のUVに畳み込むための係数
const vec2 invAtan = vec2(0.1591, 0.3183);

vec2 SampleSphericalMap(vec3 v) {
    // 方位角と仰角を求めてUVに直す
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main() {
    vec2 uv = SampleSphericalMap(normalize(LocalPos));
    FragColor = vec4(texture(equirectangularMap, uv).rgb, 1.0);
}
