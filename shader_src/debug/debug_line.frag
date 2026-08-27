// デバッグ用の単色塗り。ライティングもトーンマッピングも通さない。
#version 460 core
out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

uniform vec3 color;

void main()
{
    FragColor = vec4(color, 1.0);
    // 注意: MRT では全ての draw buffer へ書かないと前の内容が Bloom に残る
    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}
