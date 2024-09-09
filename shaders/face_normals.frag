#version 430 core

layout (location = 0) in vec3 nml;

// Out color
layout (location = 0) out vec4 color;

void main()
{
	color = vec4(0.5f * (nml + 1), 1);
}
