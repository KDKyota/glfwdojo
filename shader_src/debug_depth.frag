// 深度値を可視化するデバッグ用シェーダー。現在は描画呼び出しがコメントアウトされている。
#version 460 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D depthMap;
uniform float near_plane;
uniform float far_plane;

// 非線形 z-buffer 値をビュー空間の線形デプスに変換する
float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // NDC に戻す
    return (2.0 * near_plane * far_plane) / (far_plane + near_plane - z * (far_plane - near_plane));
}

void main()
{
    float depthValue = texture(depthMap, TexCoords).r;
    FragColor = vec4(vec3(LinearizeDepth(depthValue) / far_plane), 1.0);
}
