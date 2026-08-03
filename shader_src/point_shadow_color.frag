// 光源から見たガラスの透過色を乗算ブレンドで焼き込む。vert / geom は point_shadow_depth と共用
#version 330 core

in vec4 FragPos;
in vec2 TexCoords;

uniform vec3 lightPos;
uniform float farPlane;
uniform sampler2D diffuseMap;

out vec4 FragColor;

void main()
{
    vec4 tex = texture(diffuseMap, TexCoords);
    // glass.frag と同じ条件でガラス部分だけを残す
    if (tex.a >= 0.5 || tex.a < 0.01)
        discard;

    // 深度パスと同じ値を書き、不透明物より奥のガラスを深度テストで弾く
    gl_FragDepth = length(FragPos.xyz - lightPos) / farPlane;

    // glass.frag の透過パスと同じ式にしないと視線方向と光線方向で色がずれる
    FragColor = vec4(mix(vec3(1.0), tex.rgb, tex.a), 1.0);
}
