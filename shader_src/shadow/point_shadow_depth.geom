// 入力の三角形を6面ぶん複製し、gl_Layer でキューブマップの面を選びながら出力する。
#version 330 core
layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

in vec2 vTexCoords[];

// 光源を中心とした6方向（cubemap の各面）の view*projection 行列
uniform mat4 shadowMatrices[6];

// 透視除算前のワールド座標。frag 側が光源からの実距離を求めるのに使う
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
