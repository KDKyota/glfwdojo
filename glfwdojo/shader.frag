#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;

uniform vec3 lightPos; // 光源の位置
uniform vec3 viewPos; // カメラの位置

uniform vec3 objectColor;
uniform vec3 lightColor;

uniform float ambientStrength;
uniform float specularStrength;
uniform int shininess;

void main()
{
	// Ambient(環境光)
	vec3 ambient = ambientStrength * lightColor;

	// Diffuse
	vec3 norm = normalize(Normal);
	vec3 lightDir = normalize(lightPos - FragPos);
	float diff = max(dot(norm, lightDir), 0.0); // 内積がマイナスにならないように調整
	vec3 diffuse = diff * lightColor;

	// specular
	vec3 viewDir = normalize(viewPos - FragPos);
	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
	vec3 specular = specularStrength * spec * lightColor;


	FragColor = vec4((ambient + diffuse + specular) * objectColor, 1.0);
}