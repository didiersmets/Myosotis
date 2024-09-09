#version 430 core

#define NML_LEN 0.03

/* In variables */
/* None         */

/* Uniform variables */
layout (location = 0) uniform mat4 vm;
layout (location = 1) uniform mat4 proj;
layout (location = 2) uniform vec3 camera_pos;

layout(std430, binding = 0) restrict readonly buffer indices   {uint  Idx[];};
layout(std430, binding = 1) restrict readonly buffer positions {float Pos[];};

layout (location = 0) out vec3 nml;

void main(void) 
{

	uint tri_idx = gl_VertexID / 2;
	bool end_pt  = (gl_VertexID & 1) == 1;

	uint idx1 = Idx[3 * tri_idx + 0];
	uint idx2 = Idx[3 * tri_idx + 1];
	uint idx3 = Idx[3 * tri_idx + 2];

	vec3 v1 = vec3(Pos[3 * idx1 + 0], Pos[3 * idx1 + 1], Pos[3 * idx1 + 2]);
	vec3 v2 = vec3(Pos[3 * idx2 + 0], Pos[3 * idx2 + 1], Pos[3 * idx2 + 2]);
	vec3 v3 = vec3(Pos[3 * idx3 + 0], Pos[3 * idx3 + 1], Pos[3 * idx3 + 2]);

	vec3 v = (v1 + v2 + v3) * 0.33333333f;

	float dist = length(camera_pos - v);

	nml = normalize(cross(v2 - v1, v3 - v1));

	if (end_pt)
	{
		v += NML_LEN * dist * nml;
	}

	gl_Position = proj * vm * vec4(v, 1.f);
}
