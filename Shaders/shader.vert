#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex
{
	vec4 gl_Position;
};

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec2 in_texCoord;

layout(binding = 0) uniform SCENE_BLOCK
{
	mat4    view_matrix;
	mat4    proj_matrix;
	mat4    view_proj_matrix;
};

layout (binding = 1) uniform MODEL_MATRIX_BLOCK
{
	mat4 model_matrix[2];
};

layout (push_constant) uniform pushConstants_t
{
    layout (offset = 0) uint drawId;
} pushConstants;

void main()
{
	gl_Position = view_proj_matrix * model_matrix[pushConstants.drawId] * vec4(in_position, 1.0);
	fragColor = vec3(1.0, 1.0, 1.0);	//in_color;
	fragTexCoord = in_texCoord;
}
