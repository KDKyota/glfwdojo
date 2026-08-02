#version 330 core
layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

in vec2 vTexCoords[];

// 光源位置を中心とした6方向（cubemapの各面）の view*projection 行列
// CPU側 (Scene.cpp の shadowTransforms) で計算されて渡される
uniform mat4 shadowMatrices[6];

// 各面に描画するフラグメントのワールド座標（透視除算前）
// point_shadow_depth.frag 側で光源からの実距離を求めるために使う
out vec4 FragPos;
out vec2 TexCoords;

void main()
{
    for (int face = 0; face < 6; ++face)
    {
        // gl_Layer: 書き込み先を depthCubemap_ の6面のうちどれにするか選択する
        gl_Layer = face;
        for (int i = 0; i < 3; ++i)
        {
            // 頂点シェーダーから来たワールド座標をそのままフラグメントシェーダーへ中継
            FragPos = gl_in[i].gl_Position;
            // 面ごとの光源視点行列で変換したクリップ座標（ラスタライズ用）
            gl_Position = shadowMatrices[face] * FragPos;
            TexCoords = vTexCoords[i];
            EmitVertex();
        }
        EndPrimitive();
    }
}
