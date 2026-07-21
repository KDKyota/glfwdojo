#version 330 core

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D bloomBlur;
uniform float exposure;

out vec4 FragColor;

/*
 * screenTexture から HDR の色を読み、
 * exposure を使って 0~1 に圧縮して、ガンマ補正結果を出力
*/
void main ()
{
	const float gamma = 2.2;
	vec3 hdrColor = texture(screenTexture, TexCoords).rgb;
	vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;
	hdrColor += bloomColor; // 合成処理

	// exposure tone mapping (Reinhardなら hdrColor / (hdrColor + vec3(1.0))でもいい）
	vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);

	// ガンマ補正
	mapped = pow(mapped, vec3(1.0 / gamma));

	FragColor = vec4(mapped, 1.0);
}