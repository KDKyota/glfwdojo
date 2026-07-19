#version 330 core

out vec4 FragColor;

struct PointLight {
    vec3 position;
    float constant;
    float linear;
    float quadratic;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct Material {
    float shininess;
};

#define NR_POINT_LIGHTS 1

in vec3 FragPos;
in vec2 TexCoords;
in mat3 TBN;

uniform sampler2D texture1;
uniform sampler2D normalMap;
uniform samplerCube shadowMap;
uniform float farPlane;

uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform Material material;
uniform vec3 viewPos;

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, float shadow);
float ShadowCalculation(vec3 fragPos, vec3 normal, vec3 lightDir);

void main()
{
    // ノーマルマップをサンプルしてタンジェント空間 [-1,1] に変換
    vec3 normal = texture(normalMap, TexCoords).rgb;
    normal = normalize(normal * 2.0 - 1.0);
    // TBN でタンジェント空間からワールド空間へ変換
    normal = normalize(TBN * normal);

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 lightDir = normalize(pointLights[0].position - FragPos);
    float shadow = ShadowCalculation(FragPos, normal, lightDir);

    vec3 result = vec3(0.0);
    for (int i = 0; i < NR_POINT_LIGHTS; i++)
        result += CalcPointLight(pointLights[i], normal, FragPos, viewDir, shadow);

    vec4 texColor = texture(texture1, TexCoords);
    FragColor = vec4(result, texColor.a);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, float shadow)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * vec3(texture(texture1, TexCoords));

    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * vec3(texture(texture1, TexCoords));

    vec3 ambient = light.ambient * vec3(texture(texture1, TexCoords));

    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;

    return ambient + (1.0 - shadow) * (diffuse + specular);
}

float ShadowCalculation(vec3 fragPos, vec3 normal, vec3 lightDir)
{
    vec3 fragToLight = fragPos - pointLights[0].position;
    float currentDepth = length(fragToLight);
    float bias = max(0.15 * (1.0 - dot(normal, lightDir)), 0.05);

    float shadow = 0.0;
    float offset = 0.05;
    vec3 sampleOffsetDirections[20] = vec3[]
    (
        vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1),
        vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
        vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
        vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
        vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
    );
    for(int i = 0; i < 20; ++i)
    {
        float closestDepth = texture(shadowMap, fragToLight + sampleOffsetDirections[i] * offset).r;
        closestDepth *= farPlane;
        if(currentDepth - bias > closestDepth)
            shadow += 1.0;
    }
    return shadow / 20.0;
}
