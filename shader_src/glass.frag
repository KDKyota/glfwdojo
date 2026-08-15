// 透過窓のガラス部分を前方描画するシェーダー。
// 窓枠は gbuffer_window.frag が Deferred 側で描くので、ここでは alpha >= 0.5 を discard する。
//
// サンプラー配列をループ変数で添字するため 4.60 が必要。330 以前は定数式のみ許され、
// Mesa などではコンパイルエラーになる（NVIDIA/AMD は黙って通してしまう）
#version 460 core

out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

#include "lighting_common.glsl"

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D texture1;
uniform sampler2D ssao;

uniform vec3 viewPos;

// deferred_lighting.frag と同じ値を受け取り、不透明面と透過窓で扱いを揃える
uniform float ambientStrength;
uniform bool reflectionPass; // 透過と反射を切り替えられるようにする

uniform float metallic;
uniform float roughness;

void main()
{
    // Diffuse
    vec3 normal = normalize(Normal);

    if (!gl_FrontFacing)
        normal = -normal;

    vec2 screenUV = gl_FragCoord.xy / vec2(textureSize(ssao, 0));
    float ao = texture(ssao, screenUV).r;

    vec3 viewDir = normalize(viewPos - FragPos);

    vec4 texColor = texture(texture1, TexCoords);
    // transparent windowのガラス部分だけをレンダリングする
    if (texColor.a >= 0.5 || texColor.a < 0.01)
        discard;

    const vec3 F0 = vec3(0.04);
    // 面全体として何割を反射に回すか。透過パスの (1.0 - fresnel) と対になっている
    vec3 fresnel = fresnelSchlick(max(dot(normal, viewDir), 0.0), F0);
    // 環境光はループの外で1回だけ。距離減衰を掛けないので光源から遠くても効く
    // vec3 result = ambientStrength * texColor.rgb * ao;

    if (reflectionPass) // 反射(足し算)
    {
        vec3 reflected = vec3(0.0);
        for (int i = 0; i < NR_LIGHTS; i++)
        {
            vec3 lightDir = normalize(pointLights[i].position - FragPos);
            float shadow = ShadowCalculation(FragPos, normal, lightDir, pointLights[i].position, shadowMap[i]);
            // 自分自身の透過色も乗るのでスペキュラがガラスの色に少し染まる
            vec3 transmit = texture(shadowColor[i], FragPos - pointLights[i].position).rgb;
            // ガラスは拡散反射を持たせないので albedo は 0
            reflected += transmit * CalcPointLight(pointLights[i], normal, FragPos, viewDir,
                    vec3(0.0), roughness, metallic, F0, shadow);
        }

        // BRDF 内の fresnelSchlick が既にフレネルを含むので、ここでは掛けない
        vec3 result = reflected;

        float brightness = dot(result, vec3(0.2126, 0.7152, 0.0722));
        if (brightness > 1.0)
            BrightColor = vec4(result, 1.0);
        else
            BrightColor = vec4(0.0);

        FragColor = vec4(result, 0.0);
    }
    else // 透過(掛け算)
    {
        BrightColor = vec4(1.0);

        vec3 transmittance = mix(vec3(1.0), texColor.rgb, texColor.a);

        transmittance *= (1.0 - fresnel);
        FragColor = vec4(transmittance, 1.0);
    }
}
