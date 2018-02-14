#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

layout(location = 0) out VS_OUT
{
    vec3 N;
    vec3 L;
    vec3 V;
	vec2 tex_coord;
	flat int material_id;
} vs_out;

out gl_PerVertex
{
	vec4 gl_Position;
};

layout(binding = 0) uniform SCENE_BLOCK
{
	mat4    view_matrix;
	mat4    proj_matrix;
	mat4    view_proj_matrix;
};

layout (binding = 1) uniform MODEL_MATRIX_BLOCK
{
	mat4    model_matrix[2];
};

layout (push_constant) uniform pushConstants_t
{
    layout (offset = 0) uint drawId;
};

void main(void)
{
	vec3 light_pos = vec3(1.5, -1.5, 0.0);

	mat4 mv_matrix = view_matrix * model_matrix[drawId];
	vs_out.N = mat3(model_matrix[drawId]) * normal;
	vs_out.material_id = 0;

	vec4 P = mv_matrix * vec4(position, 1.0);
    vs_out.L = light_pos - P.xyz;
    vs_out.V = -P.xyz;
	vs_out.tex_coord = texCoord;
	gl_Position = proj_matrix * P;
}
