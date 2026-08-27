// gbuffer_cube.frag 用の頂点シェーダー。Tangent Space の各種ベクトルも出力する。
#version 460 core
//#extension GL_ARB_shading_language_420pack : enable
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;
layout (location = 5) in vec3 aOffset; // インスタンスごとの位置（非インスタンス描画では未使用＝(0,0,0)）
layout (std140, binding = 0) uniform Matrices {
	mat4 view;
	mat4 projection;
};

uniform mat4 model;
uniform mat3 normalMatrix;

uniform vec3 lightPos;
uniform vec3 viewPos;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec3 TangentLightPos;
out vec3 TangentViewPos;
out vec3 TangentFragPos;
out mat3 TBNtoWorld;

void main()
{
	FragPos = vec3(model * vec4(aPos + aOffset, 1.0));
	TexCoords = aTexCoords;
	Normal  = normalMatrix * aNormal;

	vec3 T = normalize(normalMatrix * aTangent);
	vec3 B = normalize(normalMatrix * aBitangent);
	vec3 N = normalize(normalMatrix * aNormal);
	mat3 TBN = transpose(mat3(T, B, N));
	TBNtoWorld = mat3(T, B, N);

	TangentLightPos = TBN * lightPos;
	TangentViewPos = TBN * viewPos;
	TangentFragPos = TBN * FragPos;
	gl_Position = projection * view * model * vec4(aPos + aOffset, 1.0);
}
