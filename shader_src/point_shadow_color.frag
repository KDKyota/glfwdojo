// カラー付き透過シャドウ用: 光源から見たガラスの透過色をキューブマップに乗算で焼き込む。
// 頂点/ジオメトリシェーダーは point_shadow_depth.vert / .geom を共用する。
//
// 出力先は白 (1,1,1) でクリアされた RGBA8 キューブマップで、
// glBlendFunc(GL_ZERO, GL_SRC_COLOR) により「dst * src」の乗算ブレンドで書き込まれる。
// 乗算は順序に依存しないので、ガラスが複数枚重なってもソート不要。
#version 330 core

// geometry shader から中継されたフラグメントのワールド座標
in vec4 FragPos;
in vec2 TexCoords;

// 光源のワールド座標（点光源の位置）
uniform vec3 lightPos;
// 光源からの距離を正規化するための基準値（Scene.h の shadowFarPlane_ と同じ値）
uniform float farPlane;
uniform sampler2D diffuseMap;

out vec4 FragColor;

void main()
{
    vec4 tex = texture(diffuseMap, TexCoords);
    // glass.frag と同じ条件でガラス部分だけを残す（枠と完全透明部は捨てる）
    if (tex.a >= 0.5 || tex.a < 0.01)
        discard;

    // 深度パス（point_shadow_depth.frag）と同じ正規化距離を書くことで、
    // 深度テスト（書き込みは glDepthMask(GL_FALSE) で停止中）により
    // 不透明物より奥にあるガラスが色を焼き込むのを防ぐ
    gl_FragDepth = length(FragPos.xyz - lightPos) / farPlane;

    // glass.frag の透過パスと同じ透過率。視線方向の透過と光線方向の透過で色が揃う
    FragColor = vec4(mix(vec3(1.0), tex.rgb, tex.a), 1.0);
}
