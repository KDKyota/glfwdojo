#version 330 core

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D bloomBlur;
uniform float exposure;

out vec4 FragColor;

// Bloom・トーンマッピング・ガンマ補正を飛ばす。G-Buffer の可視化には必須
uniform bool debugRawOutput;

// Bloom の合成強度。0.0 で完全に無効化できる
uniform float bloomStrength;

/*
 * screenTexture から HDR の色を読み、
 * exposure を使って 0~1 に圧縮して、ガンマ補正結果を出力
 */
// レンダーパイプラインの最終段。Bloom 合成 → トーンマッピング → ガンマ補正を行う。
void main() {
  const float gamma = 2.2;
  vec3 hdrColor = texture(screenTexture, TexCoords).rgb;
  vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;

  if (debugRawOutput) {
    FragColor = vec4(hdrColor, 1.0);
    return;
  }

  hdrColor += bloomColor * bloomStrength; // 合成処理

  vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);

  // ガンマ補正
  mapped = pow(mapped, vec3(1.0 / gamma));

  FragColor = vec4(mapped, 1.0);
}
