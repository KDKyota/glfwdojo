#version 330 core
layout (location = 0) in vec3 aPos;
// インスタンスごとの位置。cube のようなインスタンス描画でのみVAO側で有効化される。
// 床や壁のように location 5 を有効化していないVAOでは、OpenGLの規定によりカレント汎用頂点属性値
// (初期値 (0,0,0,1)) が読まれるため aOffset は (0,0,0) となり、aPos + aOffset は元の座標のままになる。
// 全VAOで location 5 を「インスタンス位置」に統一しているので、この既定値への依存は意図的なもの。
layout (location = 5) in vec3 aOffset;

uniform mat4 model;

void main()
{
    // ここではまだ view/projection をかけない
    // （光源視点への変換は後段の geometry shader が面ごとの shadowMatrices で行うため、
    //   ここではワールド座標のまま gl_Position に渡す）
    gl_Position = model * vec4(aPos + aOffset, 1.0);
}
