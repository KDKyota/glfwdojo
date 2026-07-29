#version 460 core

// 出力は遮蔽率のスカラー1つだけ。FBO のアタッチメントも1枚なので、
// MRT の「有効な全 draw buffer に書き込む」問題は起きない。
out float FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;  // ワールド座標
uniform sampler2D gNormal;    // ワールド法線
uniform sampler2D texNoise;   // 4x4 のランダム回転ベクトル

// UBO（binding = 0）はフラグメントシェーダーからも参照できる。
// メンバの順序と型は既存の .vert と完全に一致させること（ずれると値が入れ違う）。
// SSAO は投影が必要なので projection、ワールド→ビュー変換のために view を使う。
layout (std140, binding = 0) uniform Matrices {
	mat4 view;
	mat4 projection;
};

// Scene.h の SSAO_KERNEL_SIZE と一致させること
const int KERNEL_SIZE = 64;

uniform vec3 samples[KERNEL_SIZE]; // 接空間の半球状サンプル点（+Z が法線方向）
uniform float radius;              // 遮蔽物とみなす近傍の半径（ワールド空間の長さ）
uniform float bias;                // 自己遮蔽によるアクネ対策

void main()
{
	vec3 worldPos    = texture(gPosition, TexCoords).xyz;
	vec3 worldNormal = texture(gNormal,   TexCoords).xyz;

	// 背景（ジオメトリが描かれていない領域）は G-Buffer がクリア値 (0,0,0) のまま。
	// そのまま normalize(vec3(0)) すると 0/0 で NaN になり、しかもその NaN が
	// 後段の 4x4 ブラーで周囲のピクセルへ伝播して、シーン側まで壊す。
	// 遮蔽なしとして早期に打ち切る。
	if (dot(worldNormal, worldNormal) < 0.001)
	{
		FragColor = 1.0;
		return;
	}

	// --- ビュー空間へ変換 ---
	// SSAO はサンプル点を projection で画面に投影する必要があるため、
	// 投影行列を掛ける直前の空間、つまりビュー空間で計算する。
	vec3 fragPos = (view * vec4(worldPos, 1.0)).xyz;
	vec3 normal  = normalize(mat3(view) * worldNormal);

	// --- ランダム回転 ---
	// ノイズを 1テクセル = 1ピクセル でタイルするための倍率。
	// 全ピクセルで同じカーネルを使うと規則的な縞（バンディング）が出るため、
	// ピクセルごとに法線まわりでカーネルを回転させる。
	vec2 noiseScale = vec2(textureSize(gPosition, 0)) / 4.0;
	vec3 randomVec  = normalize(texture(texNoise, TexCoords * noiseScale).xyz);

	// グラム・シュミットの直交化。randomVec から法線方向の成分を引くことで
	// 法線に垂直な成分だけを取り出す。これで TBN は
	// 「法線を向いた半球」かつ「ピクセルごとにランダム回転」の2つを同時に満たす。
	vec3 tangent   = normalize(randomVec - normal * dot(randomVec, normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 TBN       = mat3(tangent, bitangent, normal);

	// view 行列の3行目。ループ内でビュー空間 z だけを内積1回で得るために使う。
	// GLSL の view[i] は「列」を返すので、3行目は各列の .z を集める。
	// （view[2] と書くと3列目になってしまうので注意）
	// ループ内で毎回組み立てないよう、必ず外で1回だけ作る。
	vec4 viewRowZ = vec4(view[0].z, view[1].z, view[2].z, view[3].z);

	float occlusion = 0.0;
	for (int i = 0; i < KERNEL_SIZE; ++i)
	{
		// サンプル点をビュー空間で作る
		vec3 samplePos = fragPos + TBN * samples[i] * radius;

		// スクリーン座標へ投影する
		vec4 offset = projection * vec4(samplePos, 1.0);
		offset.xyz /= offset.w;              // 透視除算
		offset.xyz = offset.xyz * 0.5 + 0.5; // [-1,1] -> [0,1]

		// その画面位置に「実際に写っている面」のビュー空間 z を取ってくる。
		// 必要なのは深度だけなので、mat4 の乗算ではなく内積1回で済ませる。
		// w = 1.0 を忘れると平行移動成分が消え、カメラを動かしたときだけずれる。
		vec3  sampleWorld = texture(gPosition, offset.xy).xyz;
		float sampleDepth = dot(viewRowZ, vec4(sampleWorld, 1.0));

		// 遠くの面による誤判定を防ぐ。深度差が radius より大きければ寄与を0に近づける。
		// これが無いと物体の輪郭のまわりに黒い縁取り（ハロー）が出る。
		float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));

		// OpenGL のビュー空間はカメラ前方が -Z なので、手前ほど z が大きい（0に近い）。
		// 実際の面がサンプル点より手前にある = sampleDepth の方が大きい = 遮蔽されている。
		// 不等号を逆にすると AO が反転する（隅が明るく、平坦面が暗くなる）。
		occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
	}

	// 「遮蔽されていない割合」に変換する（1.0 = 遮蔽なし, 0.0 = 完全遮蔽）。
	// 後で ambient に掛けるのでこの向きにしておく。
	FragColor = 1.0 - (occlusion / float(KERNEL_SIZE));
}
