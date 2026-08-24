// SSAO の後処理。ノイズ起因の格子模様をブラーで消す。
#version 460 core

out float FragColor;

in vec2 TexCoords;

uniform sampler2D ssaoInput;
// AO のコントラスト。1.0 で素の値、上げるほど中間調が暗くなる。
uniform float power;

void main()
{
	// ssao.frag が 4x4 のノイズを敷いた代償の格子模様を、ちょうど 4x4 の平均で打ち消す。
	// -2..1 の4回。5回だとノイズの周期が割り切れず格子模様が消えない
	vec2 texelSize = 1.0 / vec2(textureSize(ssaoInput, 0));

	float result = 0.0;
	for (int x = -2; x < 2; ++x)
	{
		for (int y = -2; y < 2; ++y)
		{
			vec2 offset = vec2(float(x), float(y)) * texelSize;
			result += texture(ssaoInput, TexCoords + offset).r;
		}
	}

	float ao = result / 16.0; // 4 x 4 = 16 サンプル

	// ブラーの前だと pow(平均) != 平均(pow) でノイズまで増幅される
	FragColor = pow(ao, power);
}
