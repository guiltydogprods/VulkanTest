#version 450
#extension GL_ARB_separate_shader_objects : enable

#ifdef GL_ARB_bindless_texture
#extension GL_ARB_bindless_texture : enable
#endif

// Output
layout (location = 0) out vec4 color;

// Input from vertex shader
in VS_OUT
{
    vec3 N;
    vec3 L;
    vec3 V;
	vec2 tex_coord;
	flat int material_id;
} fs_in;

struct MaterialProperties
{
	vec4	diffuse;
	vec3	specular;
	float	specularPower;
	int		texture_id;
};

// Material properties
layout (std430, binding = 2) readonly buffer MATERIALS
{
	MaterialProperties materials[];
};

#ifdef GL_ARB_bindless_texture
layout (std430, binding = 3) readonly buffer TEXTURES
{
	sampler2D textures[];
};
#else
	uniform sampler2D albedo;
#endif

void main(void)
{
#ifdef GL_ARB_bindless_texture
	vec3 Kd = materials[fs_in.material_id].texture_id >= 0 ? materials[fs_in.material_id].diffuse.xyz * texture(textures[materials[fs_in.material_id].texture_id], fs_in.tex_coord).xyz : materials[fs_in.material_id].diffuse.xyz;
#else
	vec3 Kd = materials[fs_in.material_id].texture_id >= 0 ? materials[fs_in.material_id].diffuse.xyz * texture(albedo, fs_in.tex_coord).xyz : materials[fs_in.material_id].diffuse.xyz;
#endif

	vec3 Ks = materials[fs_in.material_id].specular;
	float m = materials[fs_in.material_id].specularPower;

    vec3 n = normalize(fs_in.N);
    vec3 l = normalize(fs_in.L);
    vec3 v = normalize(fs_in.V);
    vec3 h = normalize(l + v);
	vec3 Lo = vec3(0.0);
 
	float nDotL = clamp(dot(n, l), 0.0, 1.0);
	float nDotH = clamp(dot(n, h), 0.0, 1.0);
	Lo += (Kd + Ks * pow(nDotH, m)) * nDotL;

	color = vec4(Lo, 1.0);
}

