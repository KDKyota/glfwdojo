#version 460 core

layout(location = 0) out vec3 gPosition;
layout(location = 1) out vec4 gNormal; // a = metallic
layout(location = 2) out vec4 gAlbedoSpec;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D diffuseMap;
uniform float metallic;

void main()
{
    // 窓の透過部分と窓枠で処理を分ける
    vec4 texColor = texture(diffuseMap, TexCoords);
    if (texColor.a < 0.5)
        discard; // ガラス部分は別処理

    vec3 normal = normalize(Normal);
    if (!gl_FrontFacing)
        normal = -normal;

    gPosition = FragPos;
    gNormal = vec4(normal, metallic);
    gAlbedoSpec.rgb = texColor.rgb;
    gAlbedoSpec.a = dot(texColor.rgb, vec3(0.2126, 0.7152, 0.0722));
}
