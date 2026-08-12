#version 460 core

out float FragColor; // 遮蔽率のスカラー1つだけ

in vec2 TexCoords;

uniform sampler2D gPosition;  // ワールド座標
uniform sampler2D gNormal;    // ワールド法線
uniform sampler2D texNoise;   // 4x4 のランダム回転ベクトル

// メンバの順序と型は .vert 側と完全に一致させること
layout (std140, binding = 0) uniform Matrices {
	mat4 view;
	mat4 projection;
};

// Scene.h の SSAO_KERNEL_SIZE と一致させること
const int KERNEL_SIZE = 64;

uniform vec3 samples[KERNEL_SIZE]; // 接空間の半球状サンプル点（+Z が法線方向）
uniform float radius;              // 遮蔽物とみなす近傍の半径
uniform float bias;                // 自己遮蔽によるアクネ対策

void main()
{
	vec3 worldPos    = texture(gPosition, TexCoords).xyz;
	vec3 worldNormal = texture(gNormal,   TexCoords).xyz;

	// 背景の normalize(vec3(0)) が NaN になり、後段のブラーで周囲へ伝播して壊す
	if (dot(worldNormal, worldNormal) < 0.001)
	{
		FragColor = 1.0;
		return;
	}

	// サンプル点を projection で画面へ投影する必要があるのでビュー空間で計算する
	vec3 fragPos = (view * vec4(worldPos, 1.0)).xyz;
	vec3 normal  = normalize(mat3(view) * worldNormal);

	// 全ピクセルで同じカーネルを使うと縞が出るので法線まわりにランダム回転させる
	vec2 noiseScale = vec2(textureSize(gPosition, 0)) / 4.0;
	vec3 randomVec  = normalize(texture(texNoise, TexCoords * noiseScale).xyz);

	// グラム・シュミットの直交化。法線を向いた半球かつランダム回転した基底になる
	vec3 tangent   = normalize(randomVec - normal * dot(randomVec, normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 TBN       = mat3(tangent, bitangent, normal);

	// view 行列の3行目。GLSL の view[i] は「列」なので各列の .z を集める
	vec4 viewRowZ = vec4(view[0].z, view[1].z, view[2].z, view[3].z);

	float occlusion = 0.0;
	for (int i = 0; i < KERNEL_SIZE; ++i)
	{
		vec3 samplePos = fragPos + TBN * samples[i] * radius;

		vec4 offset = projection * vec4(samplePos, 1.0);
		offset.xyz /= offset.w;              // 透視除算
		offset.xyz = offset.xyz * 0.5 + 0.5; // [-1,1] -> [0,1]

		// w = 1.0 を忘れると平行移動が消え、カメラを動かしたときだけずれる
		vec3  sampleWorld = texture(gPosition, offset.xy).xyz;
		float sampleDepth = dot(viewRowZ, vec4(sampleWorld, 1.0));

		// 遠くの面による誤判定を防ぐ。無いと物体の輪郭に黒い縁取り（ハロー）が出る
		float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));

		// 不等号を逆にすると AO が反転する（隅が明るく、平坦面が暗くなる）
		occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
	}

	// 遮蔽されていない割合に変換する（1.0 = 遮蔽なし, 0.0 = 完全遮蔽）
	FragColor = 1.0 - (occlusion / float(KERNEL_SIZE));
}
