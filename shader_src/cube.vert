#version 330 core
#extension GL_ARB_shading_language_420pack : enable
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aOffset;
layout (std140, binding = 0) uniform Matrices {
	mat4 view;
	mat4 projection;
};

uniform mat4 model;
uniform mat3 normalMatrix;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

void main()
{
	FragPos = vec3(model * vec4(aPos + aOffset, 1.0));
	TexCoords = aTexCoords;
	Normal  = normalMatrix * aNormal;
	gl_Position = projection * view * model * vec4(aPos + aOffset, 1.0);
}
