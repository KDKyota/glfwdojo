// SDF 遮蔽の後処理 ピクセルごとの回転が残したノイズを均す
#version 460 core

out vec2 FragColor;

in vec2 TexCoords;

uniform sampler2D sdfOcclusionInput;

void main() {
    // 4x4 のノイズを敷いた代償のノイズを ちょうど 4x4 の平均で打ち消す
    // -2..1 の4回 5回だとノイズの周期が割り切れず消えない
    vec2 texelSize = 1.0 / vec2(textureSize(sdfOcclusionInput, 0));

    float result = 0.0;
    for (int x = -2; x < 2; ++x) {
        for (int y = -2; y < 2; ++y) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(sdfOcclusionInput, TexCoords + offset).r;
        }
    }
    // 鏡面はノイズを敷いていないので均す必要がなく ぼかすと輪郭を越えて染み出すのでそのまま渡す
    FragColor = vec2(result / 16.0, texture(sdfOcclusionInput, TexCoords).g);
}
