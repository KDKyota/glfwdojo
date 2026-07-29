#version 330 core

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D bloomBlur;
uniform float exposure;

out vec4 FragColor;

// デバッグ用スイッチ。
// true にすると Bloom の合成・トーンマッピング・ガンマ補正をすべて飛ばし、
// screenTexture の値をそのまま出力する。
//
// deferred_lighting.frag の debugMode で G-Buffer や AO を可視化するときは必ず有効にすること。
// 通常経路の mapped = 1 - exp(-c * exposure) と pow(c, 1/2.2) は値を大きく持ち上げるため、
// 例えば 0.5 が 0.89 として表示され、正常な値でも「真っ白」に見えて判別できなくなる。
uniform bool debugRawOutput;

/*
 * screenTexture から HDR の色を読み、
 * exposure を使って 0~1 に圧縮して、ガンマ補正結果を出力
 */
void main() {
  const float gamma = 2.2;
  vec3 hdrColor = texture(screenTexture, TexCoords).rgb;
  vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;

  if (debugRawOutput) {
    FragColor = vec4(hdrColor, 1.0);
    return;
  }

  hdrColor += bloomColor; // 合成処理

  // exposure tone mapping (Reinhardなら hdrColor / (hdrColor +
  // vec3(1.0))でもいい）
  vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);

  // ガンマ補正
  mapped = pow(mapped, vec3(1.0 / gamma));

  FragColor = vec4(mapped, 1.0);
}
