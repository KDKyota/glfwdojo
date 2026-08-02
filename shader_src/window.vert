#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 5) in vec3 aOffset; // インスタンスごとの位置
layout(std140, binding = 0) uniform Matrices
{
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
    // mat4 instanceModel = mat4(1.0f);
    // instanceModel[3] = vec4(aOffset, 1.0f);
    gl_Position = projection * view * model * vec4(aPos + aOffset, 1.0);
    TexCoords = aTexCoords;
    FragPos = vec3(model * vec4(aPos + aOffset, 1.0));
    Normal = normalMatrix * aNormal;
}
