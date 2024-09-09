#version 430 core

// In variables
layout (location = 0) in vec3 _N;  /* Normal vector */
layout (location = 1) in vec3 _V;  /* View vector   */
layout (location = 2) in vec3 _L;  /* Light vector  */

// Uniform variables
layout (location = 3) uniform bool smooth_shading;
layout (location = 4) uniform int level;
layout (location = 5) uniform int variation;

// Out color
layout (location = 0) out vec4 color;



#define AMBIENT_COLOR vec3(1.f, 0.8f, 1.f)
#define Ka 0.1f
#define DIFFUSE_COLOR vec3(.88f, .75f, 0.43f)
#define Kd .8f
#define SPECULAR_COLOR vec3(1.f, 1.f, 1.f)
#define Ks 0.1f
#define shininess 8


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
		//vec3 R = 2.f * dot(N, L) * N - L;
		vec3 R = reflect(-L, N);
		float ca = max(dot(R, V), 0.f); 
		Is = pow(ca, shininess) * shininess / 4;
	}

	/* Hack rock vs grass */
	float rock_ratio = clamp(8.f  * (0.6f - N.z), 0, 1);
	//rock_ratio = 1;
	vec3 diffuse = vec3(0.67f, 0.64f, 0.46f) * rock_ratio + vec3(0.12, 0.25,
			0.08) * (1.f - rock_ratio);

	vec3 full = Ka * AMBIENT_COLOR;

	//full     += Kd * Id * DIFFUSE_COLOR;
	full += Kd * Id * diffuse;
	full     += Ks * Is * SPECULAR_COLOR;
	color = vec4(full, 1.0f);
	if (level == 0) {color.r *= (0.5f - 0.07f * variation);}
	if (level == 1) {color.g *= (0.5f - 0.07f * variation);}
	if (level == 2) {color.b *= (0.5f - 0.07f * variation);}
}
