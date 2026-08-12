#version 460 core
out vec4 FragColor;
// framebuffer_ は glDrawBuffers で2枚のカラーアタッチメントを同時に有効にしているため、
// 全 draw buffer に書かないと中身が未定義になり、Bloom で画面全体に白い靄がかかる
layout(location = 1) out vec4 BrightColor;

in vec3 TexCoords;

uniform samplerCube skybox;

void main()
{
    FragColor = texture(skybox, TexCoords);
    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}
