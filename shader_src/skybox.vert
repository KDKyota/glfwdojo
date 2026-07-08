#version 460 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

layout (std140, binding = 0) uniform Matrices {
    mat4 view;
    mat4 projection;
};

void main()
{
    TexCoords = aPos;
    mat4 skyView = mat4(mat3(view)); // 平行移動を除去
    vec4 pos = projection * skyView * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}