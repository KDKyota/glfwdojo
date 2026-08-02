#version 460 core
out vec4 FragColor;
// framebuffer_ は glDrawBuffers で2枚のカラーアタッチメントを同時に有効にしているため、
// 有効な全ての draw buffer に書き込まないとその中身が「未定義」になる。
// ここを書き忘れると brightColorBuffer_ に空でスカイボックス領域全体にゴミが入り、
// それが Bloom のブラーで拡散して画面全体に白い靄がかかる。
// スカイボックスは Bloom させたくないので常に黒を書く。
layout(location = 1) out vec4 BrightColor;

in vec3 TexCoords;

uniform samplerCube skybox;

void main()
{
    FragColor = texture(skybox, TexCoords);
    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}
