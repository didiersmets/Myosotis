#version 430 core

/* In variables */
layout (location = 0) in vec3 _pos;
layout (location = 1) in vec3 _nml;
layout (location = 2) in vec2 _tex;
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
layout (location = 7) uniform float kappa;
layout (location = 8) uniform float step;

/* Uniform variables (cell dependent) */
layout (location = 9)  uniform int  level;
layout (location = 13) uniform int  vtx_offset;
layout (location = 14) uniform int  parent_vtx_offset;


/* Out variables */
layout (location = 0) out vec3 N;  /* Normal vector */
layout (location = 1) out vec3 V;  /* View vector   */
layout (location = 2) out vec3 L;  /* Light vector  */
layout (location = 3) out float lambda;  /* Morphing param */

float norminf(vec3 v)
{
	return max(max(abs(v.x), abs(v.y)), abs(v.z));
}

#define sigma0 0.1
#define SQRT3_OVER_2 0.8660254

void main() 
{
	vec3 pos = _pos;
	vec3 nml = _nml;
	lambda = 1.0;
	if (continuous_lod)
	{
		// d_\infty distance version
		//float r = norminf(camera_pos - pos) / (step * (1 << level));
		//lambda = 1 - smoothstep(kappa + 1 + sigma0, 2 * (kappa - sigma0), r);
		
		// euclidean distance version
		float r = length(camera_pos - pos) / (step * (1 << level) * SQRT3_OVER_2);
		//lambda = 1 - smoothstep(kappa + 1, 2 * (kappa - 1), r);
		lambda = clamp((2 * (kappa -1) - r) / (kappa - 3), 0, 1);

		
		uint j = 3 * (parent_idx +  parent_vtx_offset);
		pos = lambda * _pos + (1 - lambda) * vec3(Pos[j + 0], Pos[j + 1], Pos[j + 2]);

		if (smooth_shading)
		{
			nml = lambda * nml + (1 - lambda) * vec3(Nml[j + 0], Nml[j + 1], Nml[j + 2]);
		}
	}

	N = nml;
	V = camera_pos - pos;
	L = V;
	gl_Position = proj * vm * vec4(pos, 1.0f);
}
