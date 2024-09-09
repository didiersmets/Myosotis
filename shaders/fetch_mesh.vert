#version 430 core

/* In variables */
layout (location = 3) in int parent_idx;

/* SSBO */
layout(std430, binding = 1) restrict readonly buffer positions {float Pos[];};
layout(std430, binding = 2) restrict readonly buffer normals   {float Nml[];};

/* Uniform variables (cell independent) */
layout (location = 0) uniform mat4 vm;
layout (location = 1) uniform mat4 proj;
layout (location = 2) uniform vec3 camera_pos;
layout (location = 3) uniform bool continuous_lod;
layout (location = 4) uniform bool smooth_shading;
/* Uniform variables (cell dependent) */
layout (location = 6) uniform int  level;
layout (location = 7) uniform int  vtx_offset;
layout (location = 8) uniform int  parent_vtx_offset;

/* Out variables */
layout (location = 0) out vec3 N;  /* Normal vector */
layout (location = 1) out vec3 V;  /* View vector   */
layout (location = 2) out vec3 L;  /* Light vector  */

void main() 
{
	uint i = 3 * (gl_VertexID + vtx_offset);

	vec3 pos = vec3(Pos[i + 0], Pos[i + 1], Pos[i + 2]);

	vec3 nml = vec3(0, 0, 0);
	if (smooth_shading)
	{
		nml = vec3(Nml[i + 0], Nml[i + 1], Nml[i + 2]);
	}

	if (continuous_lod)
	{
		float ratio = 1.0; /* TODO */
		uint j = 3 * (parent_idx +  parent_vtx_offset);
		pos *= ratio;
		pos += (1.0 - ratio) * vec3(Pos[j + 0], Pos[j + 1], Pos[j + 2]);

		if (smooth_shading)
		{
			nml *= ratio;
			nml += (1.0 - ratio) * vec3(Nml[j + 0], Nml[j + 1], Nml[j + 2]);
		}
	}

	N = nml;
	V = camera_pos - pos;
	L = V;
	gl_Position = proj * vm * vec4(pos, 1.0f);
}
