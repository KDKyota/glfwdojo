#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

struct Material {
	vec3 ambient; // 環境光の影響
	sampler2D diffuse; // 反射しやすい色を指定
	// ふつうの物体はambientとdiffuseは同じ色
	sampler2D specular; // 光沢の強さを指定するための値
	float shininess;
};

struct Light {
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

uniform Material material;
uniform Light light;

uniform vec3 lightPos; // 光源の位置
uniform vec3 viewPos; // カメラの位置

//uniform vec3 objectColor;
uniform vec3 lightColor;

void main()
{
	// Ambient(環境光)
	vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));

	// Diffuse
	vec3 norm = normalize(Normal);
	vec3 lightDir = normalize(light.position - FragPos);
	float diff = max(dot(norm, lightDir), 0.0); // 内積がマイナスにならないように調整
	vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoords));

	// specular
	vec3 viewDir = normalize(viewPos - FragPos);
	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
	vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));

	// スポットライト(soft edge)
	float theta = dot(lightDir, normalize(-light.direction));
	float epsilon = light.cutOff - light.outerCutOff;
	float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

	diffuse *= intensity;
	specular *= intensity;

	float distance = length(light.position - FragPos);
	float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    

	diffuse *= attenuation;
	specular *= attenuation;

	FragColor = vec4((ambient + diffuse + specular) , 1.0);
}