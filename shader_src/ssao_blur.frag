#version 460 core

out float FragColor;

in vec2 TexCoords;

uniform sampler2D ssaoInput;
// AO のコントラスト。1.0 で素の値、上げるほど中間調が暗くなる。
uniform float power;

void main()
{
	// ssao.frag は 4x4 のノイズテクスチャをタイル状に敷いてカーネルを回転させている。
	// その代償として 4x4 周期の格子模様（ノイズ）が出るので、ちょうど 4x4 の平均で打ち消す。
	//
	// ループが -2..1 の「4回」であって -2..2 の「5回」ではない点に注意。
	// ノイズの周期が4なので、4サンプルの平均でないと周期が割り切れず消えない。
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

	// コントラスト調整は「ブラーの後」に行う。
	// ブラー前にかけると、まだノイズが乗った値を pow で増幅してから平均することになり、
	// pow(平均) != 平均(pow) のためノイズまで強調されてしまう。
	// ここが AO パイプラインの最終段なので、以降 ssao テクスチャを読む側
	// （Lighting パスも DEBUG_MODE 7 も）は同じ値を見ることになる。
	FragColor = pow(ao, power);
}
