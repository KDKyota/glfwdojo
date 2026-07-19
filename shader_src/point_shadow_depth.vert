#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 3) in vec3 aOffset;

uniform mat4 model;

void main()
{
    gl_Position = model * vec4(aPos + aOffset, 1.0);
}
