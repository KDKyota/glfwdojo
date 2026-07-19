#version 330 core
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

layout (std140, binding = 0) uniform Matrices {
    mat4 view;
    mat4 projection;
};

uniform mat4 model;

out vec3 FragPos;
out vec2 TexCoords;
out mat3 TBN;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    TexCoords = aTexCoords;

    mat3 normalMat = transpose(inverse(mat3(model)));
    vec3 T = normalize(normalMat * aTangent);
    vec3 B = normalize(normalMat * aBitangent);
    vec3 N = normalize(normalMat * aNormal);
    TBN = mat3(T, B, N);

    gl_Position = projection * view * vec4(FragPos, 1.0);
}
