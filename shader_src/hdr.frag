#version 330 core

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D bloomBlur;
uniform float exposure;

out vec4 FragColor;

// Bloom合成・トーンマッピング・ガンマ補正を全て飛ばす。
// これらは値を大きく持ち上げるため、G-Buffer や AO を可視化するときは必ず有効にすること
uniform bool debugRawOutput;

// Bloom の合成強度。0.0 で完全に無効化できる
uniform float bloomStrength;

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

  hdrColor += bloomColor * bloomStrength; // 合成処理

  // exposure tone mapping (Reinhardなら hdrColor / (hdrColor +
  // vec3(1.0))でもいい）
  vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);

  // ガンマ補正
  mapped = pow(mapped, vec3(1.0 / gamma));

  FragColor = vec4(mapped, 1.0);
}
