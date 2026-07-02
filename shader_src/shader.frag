#version 330 core
out vec4 FragColor;


struct Material {
      vec3 ambient; // 環境光の影響
      sampler2D diffuse; // 反射しやすい色を指定
      // ふつうの物体はambientとdiffuseは同じ色
      sampler2D specular; // 光沢の強さを指定するための値
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

#define NR_POINT_LIGHTS 4

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

uniform DirLight dirLight;
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform SpotLight spotLight;
uniform Material material;

uniform vec3 lightPos; // 光源の位置
uniform vec3 viewPos; // カメラの位置

//function
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewdir);

void main()
{
// Diffuse
vec3 norm = normalize(Normal);
vec3 viewDir = normalize(viewPos - FragPos);

  // 1. directional lighting
  vec3 result = CalcDirLight(dirLight, norm, viewDir);
  // 2. point lighting
  for(int i = 0; i < NR_POINT_LIGHTS; i++)
  {
		  result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);
  }
  // 3. spot lighting
  result += CalcSpotLight(spotLight, norm, FragPos, viewDir);

  FragColor = vec4(result, 1.0);
}

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

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
      vec3 lightDir = normalize(light.position - fragPos);
      // Diffuse
      float diff = max(dot(normal, lightDir), 0.0);
      vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoords));
      // Specular
      vec3 reflectDir = reflect(-lightDir, normal);
      float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
      vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
      // Combine results
      vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));

      // attenuation
      float distance = length(light.position - fragPos);
      float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

      ambient *= attenuation;
      diffuse *= attenuation;
      specular *= attenuation;
      return (ambient + diffuse + specular);
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
