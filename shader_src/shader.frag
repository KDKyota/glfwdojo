#version 330 core

out vec4 FragColor;

struct Material {
	vec3 ambient; // 環境光の影響
	sampler2D diffuse;
	// ふつうの物体はambientとdiffuseは同じ色
	sampler2D specular;
	float shininess;
};

struct DirLight {
	vec3 direction;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

struct PointLight {
	vec3 position;

	float constant;
	float linear;
	float quadratic;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

struct SpotLight {
	vec3 position;
	vec3 direction;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;

	// Attenuation(減衰)の値
	float constant;
	float linear;
	float quadratic;
	// スポットライトの角度
	float cutOff;
	float outerCutOff;
};

#define NR_POINT_LIGHTS 1 

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 FragPosLightSpace;

uniform sampler2D texture1;
uniform sampler2D shadowMap;

uniform DirLight dirLight;
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform SpotLight spotLight;
uniform Material material;

uniform vec3 lightPos; // 光源の位置
uniform vec3 viewPos; // カメラの位置

//function
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, float shadow);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewdir);
float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir);

void main()
{
	// Diffuse
    vec3 normal = normalize(Normal);
	vec3 viewDir = normalize(viewPos - FragPos);

	// 1. directional lighting
	// directional lightは今は使わないのでコメントアウト
	//vec3 result = CalcDirLight(dirLight, norm, viewDir);
	// 2. point lighting
    vec3 lightDir = normalize(pointLights[0].position - FragPos);
    float shadow = ShadowCalculation(FragPosLightSpace, normal, lightDir);
	vec3 result = vec3(0.0f);
	for(int i = 0; i < NR_POINT_LIGHTS; i++)
	{
	  result += CalcPointLight(pointLights[i], normal, FragPos, viewDir, shadow);
	}
	// 3. spot lighting
	// result += CalcSpotLight(spotLight, norm, FragPos, viewDir);

	vec4 texColor = texture(texture1, TexCoords);
	FragColor = vec4(result, texColor.a);
}

// DirLightの計算関数
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
      vec3 lightDir = normalize(-light.direction);
      // Diffuse
      float diff = max(dot(normal, lightDir), 0.0);
      vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoords));
      // Specular
      vec3 reflectDir = reflect(-lightDir, normal);
      float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
      vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
      // Combine results
      vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));
      return (ambient + diffuse + specular);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, float shadow)
{
      vec3 lightDir = normalize(light.position - fragPos);
      // Diffuse
      float diff = max(dot(normal, lightDir), 0.0);
      //vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoords));
      vec3 diffuse = light.diffuse * diff * vec3(texture(texture1, TexCoords));
      // Specular
      vec3 reflectDir = reflect(-lightDir, normal);
      float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
      //vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
      vec3 specular = light.specular * spec * vec3(texture(texture1, TexCoords));

      // Combine results
      //vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));
      vec3 ambient = light.ambient * vec3(texture(texture1, TexCoords));

      // attenuation
      float distance = length(light.position - fragPos);
      float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

      ambient *= attenuation;
      diffuse *= attenuation;
      specular *= attenuation;
      // 返し値にshadowを考慮
      return (ambient + (1.0f - shadow) * (diffuse + specular));
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir   )
{
      vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    // spotlight intensity
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    // combine results
    vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    return (ambient + diffuse + specular);
}

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // bias to reduce shadow acne
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    // PCF: 3x3 カーネルで周辺テクセルをサンプリングして平均を取る
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -3; x <= 3; ++x)
    {
        for(int y = -3; y <= 3; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 49.0;

    // ライトのファープレーン外は影なし
    if(projCoords.z > 1.0)
        shadow = 0.0;

    return shadow;
}
