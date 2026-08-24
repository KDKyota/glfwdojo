// Point Light 用シャドウマップの深度パス。ワールド座標のまま geometry shader へ渡す。
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 2) in vec2 aTexCoords;
// 有効化していないVAOでは既定値 (0,0,0) が読まれて無効化される。この依存は意図的
layout(location = 5) in vec3 aOffset;
// スキン 3D モデル用
layout(location = 6) in ivec4 aBoneIDs;
layout(location = 7) in vec4 aWeights;

const int MAX_BONES = 128;
uniform mat4 finalBones[MAX_BONES]; // ボーンごとの最終返還行列を並べた配列
uniform mat4 model;
uniform bool hasBones;

out vec2 vTexCoords;

void main()
{
    vec4 localPos = vec4(aPos, 1.0);

    if (hasBones) {
        mat4 skin = mat4(0.0);
        float totalWeight = 0.0;
        for (int i = 0; i < 4; ++i) {
            skin += finalBones[aBoneIDs[i]] * aWeights[i];
            totalWeight += aWeights[i];
        }
        if (totalWeight < 1e-5)
            skin = mat4(1.0);
        localPos = skin * localPos;
        mat3 skinRotation = mat3(skin);
    }

    // 光源視点への変換は geometry shader が面ごとに行うのでワールド座標のまま渡す
    gl_Position = model * vec4(localPos.xyz + aOffset, 1.0);
    vTexCoords = aTexCoords;
}
