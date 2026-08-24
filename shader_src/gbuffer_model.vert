#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;
// location 5 はインスタンス描画の aOffset 用に空けてある（point_shadow_depth.vert と VAO を共有するため）
layout(location = 6) in ivec4 aBoneIDs;
layout(location = 7) in vec4 aWeights;

layout(std140, binding = 0) uniform Matrices {
    mat4 view;
    mat4 projection;
};

const int MAX_BONES = 128;

uniform mat4 finalBones[MAX_BONES]; // ボーンごとの最終返還行列を並べた配列
uniform mat4 model;
uniform bool hasBones;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out mat3 TBN;

void main()
{
    vec4 localPos = vec4(aPos, 1.0);
    vec3 localNormal = aNormal;
    vec3 localTangent = aTangent;
    vec3 localBitangent = aBitangent;

    if (hasBones) {
        mat4 skin = mat4(0.0);
        float totalWeight = 0.0;
        for (int i = 0; i < 4; ++i) {
            skin += finalBones[aBoneIDs[i]] * aWeights[i];
            totalWeight += aWeights[i];
        }
        if (totalWeight < 1e-5) // 10のマイナス5乗
            skin = mat4(1.0);
        localPos = skin * localPos;
        mat3 skinRotation = mat3(skin);
        localNormal = skinRotation * aNormal;
        localTangent = skinRotation * aTangent;
        localBitangent = skinRotation * aBitangent;
    }

    vec4 worldPos = model * localPos;
    FragPos = worldPos.xyz;
    TexCoords = aTexCoords;

    mat3 normalMat = transpose(inverse(mat3(model)));
    Normal = normalize(normalMat * localNormal);
    vec3 T = normalize(normalMat * localTangent);
    vec3 B = normalize(normalMat * localBitangent);
    TBN = mat3(T, B, Normal);

    gl_Position = projection * view * worldPos;
}
