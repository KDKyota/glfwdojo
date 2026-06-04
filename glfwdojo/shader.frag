#version 330 core
out vec4 FragColor;

in vec3 vertexColor;
in vec2 TexCoord;

uniform sampler2D texture1;
uniform sampler2D texture2;

void main()
{
	// texture1を80%, texture2を20%で線形補完
	FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), 0.2);
}