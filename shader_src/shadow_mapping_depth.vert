#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 3) in vec3 aOffset; // インスタンスごとの位置オフセット（床など非インスタンスは 0,0,0）

// ディレクショナルライト視点の view*projection 行列（光源を1つのカメラと見なした変換）
// point light と違い光源に「向き」があるので、cubemapではなく1枚の2Dマップで済む
uniform mat4 lightSpaceMatrix;
uniform mat4 model;

void main()
{
    // 通常の描画と同じ流れだが、view/projection の代わりに lightSpaceMatrix を使うことで
    // 「光源から見た」クリップ座標・深度を depth map に書き込む
    gl_Position = lightSpaceMatrix * model * vec4(aPos + aOffset, 1.0);
}
