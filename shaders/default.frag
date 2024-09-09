#version 430 core

// In variables
layout (location = 0) in vec3 _N;  /* Normal vector */
layout (location = 1) in vec3 _V;  /* View vector   */
layout (location = 2) in vec3 _L;  /* Light vector  */
layout (location = 3) in float lambda;  /* Morphing param  */

/* Uniform variables (cell independent) */
layout (location = 4) uniform bool smooth_shading;
layout (location = 5) uniform bool colorize_lod;
layout (location = 6) uniform bool colorize_cells;

/* Uniform variables (cell dependent) */
layout (location = 9)  uniform int  level;
layout (location = 10) uniform int  cell_x;
layout (location = 11) uniform int  cell_y;
layout (location = 12) uniform int  cell_z;

// Out color
layout (location = 0) out vec4 color;


#define AMBIENT_COLOR vec3(1.f, 0.8f, 1.f)
#define Ka 0.1f
#define DIFFUSE_COLOR vec3(.88f, .75f, 0.43f)
#define Kd .8f
#define SPECULAR_COLOR vec3(1.f, 1.f, 1.f)
#define Ks 0.1f
#define shininess 8

const ivec3 cell_colors[8] = {
	{120,28,129},
	{64,67,153},
	{72,139,194},
	{107,178,140},
	{159,190,87},
	{210,179,63},
	{231,126,49},
	{217,33,32}
};


void main() 
{

	vec3 V = normalize(_V);
	vec3 L = normalize(_L);
	vec3 N;
	if (smooth_shading)
	{
		N = normalize(_N);
		if (dot(V, N) < 0)
		{
			N = -N;
		}
	}
	else /* flat shading */
	{
		/**
		 * Yields face normal, using unormalized _V is equiv to 
		 * using world pos but avoids an additional in variable
		 */
		N = normalize(cross(dFdx(_V), dFdy(_V)));
	}

	float Id = max(dot(N, L), 0.f);
	float Is = 0.f;
	if (Id > 0)
	{
		vec3 R = reflect(-L, N);
		float ca = max(dot(R, V), 0.f); 
		Is = pow(ca, shininess) * shininess / 4;
	}

	vec3 full = vec3(0.);
	
	if (colorize_lod)
	{
		float l = level + (1 - lambda);
		vec3 c = vec3(0, 0, 0);
		float lg = 3.0;
		if (l < 1) 
		{
			c.r = 1 - l;
			c.g = l;
		}
		else if (l < 2)
		{
			c.g = 2 - l;
			c.b = l - 1;
		}
		else
		{
			c.r = smoothstep(2, 6, l);
			c.b = 1.0 - c.r;
		}
		full += (Ka + Kd * Id + Ks * Is) * c;
	}
	else if (colorize_cells)
	{
		int idx = 31 * level + 7 * cell_x + 13 * cell_y + 17 * cell_z;
		idx = idx & 7;
		vec3 c = vec3(cell_colors[idx]) / 255.f;
		full += (Ka + Kd * Id + Ks * Is) * c;
	}
	else
	{
		full += Ka * AMBIENT_COLOR;
		full += Kd * Id * DIFFUSE_COLOR;
		full += Ks * Is * SPECULAR_COLOR;
	}

	color = vec4(full, 1.0f);
}
