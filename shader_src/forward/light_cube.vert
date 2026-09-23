// 各 PointLight の位置を示す 光らせるだけの小さなキューブを描くための頂点シェーダー
#version 460 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform vec4 clipPlane; // 平面反射で床より上だけを残す境界面
layout (std140, binding = 0) uniform Matrices {
    mat4 view;
    mat4 projection;
};

void main ()
{
	vec4 worldPos = model * vec4(aPos, 1.0);
	gl_ClipDistance[0] = dot(worldPos, clipPlane);
	gl_Position = projection * view * worldPos;
}
