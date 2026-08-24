// gbuffer_model.frag 用の頂点シェーダー。aBoneIDs/aWeights はまだ計算には使わない。
#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;
// location 5 はインスタンス描画の aOffset 用に空けてある（point_shadow_depth.vert と VAO を共有するため）
// 以下2つはスキニング（#9）で使う。段1では受け取るだけで何もしない
layout (location = 6) in ivec4 aBoneIDs;
layout (location = 7) in vec4 aWeights;

layout (std140, binding = 0) uniform Matrices {
    mat4 view;
    mat4 projection;
};

uniform mat4 model;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out mat3 TBN;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;
    TexCoords = aTexCoords;

    mat3 normalMat = transpose(inverse(mat3(model)));
    Normal = normalize(normalMat * aNormal);
    vec3 T = normalize(normalMat * aTangent);
    vec3 B = normalize(normalMat * aBitangent);
    TBN = mat3(T, B, Normal);

    gl_Position = projection * view * worldPos;
}
