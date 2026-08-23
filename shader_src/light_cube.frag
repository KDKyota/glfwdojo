// ライティング計算はせず、PointLight::diffuse をそのまま塗るだけ（光源自体を発光体として見せる）。
#version 460 core
out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

uniform vec3 lightColor;

void main()
{
	FragColor = vec4(lightColor, 1.0);

	float brightness = dot(lightColor, vec3(0.2126, 0.7152, 0.0722));
	if(brightness > 1.0)
		BrightColor = vec4(lightColor, 1.0);
	else
		BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
};
