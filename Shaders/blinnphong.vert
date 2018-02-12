#version 450
#extension GL_ARB_separate_shader_objects : enable

#ifdef GL_ARB_shader_draw_parameters
#extension GL_ARB_shader_draw_parameters : enable
#endif

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

layout(binding = 0, std430) readonly buffer MODEL_MATRIX_BLOCK
{
	mat4    model_matrix[];
};

layout(binding = 0, std140) uniform TRANSFORM_BLOCK
{
	mat4    view_matrix;
	mat4    proj_matrix;
	mat4    view_proj_matrix;
};

out VS_OUT
{
    vec3 N;
    vec3 L;
    vec3 V;
	vec2 tex_coord;
	flat int material_id;
} vs_out;

uniform vec3 light_pos = vec3(3.0, 10.0, 0.0);

void main(void)
{
#ifdef GL_ARB_shader_draw_parameters
	mat4 mv_matrix = view_matrix * model_matrix[gl_DrawIDARB];
	vs_out.N = mat3(model_matrix[gl_DrawIDARB]) * normal;
	vs_out.material_id = gl_BaseInstanceARB;
#else
	mat4 mv_matrix = view_matrix * model_matrix[0];
	vs_out.N = mat3(model_matrix[0]) * normal;
	vs_out.material_id = 0;
#endif

	vec4 P = mv_matrix * vec4(position, 1.0);
    vs_out.L = light_pos - P.xyz;
    vs_out.V = -P.xyz;
	vs_out.tex_coord = texCoord;
	gl_Position = proj_matrix * P;
}
