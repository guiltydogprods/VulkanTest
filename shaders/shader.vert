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

layout (binding = 0) uniform ubo_t
{
	mat4 modelViewProj;
} ubo;

void main()
{
	gl_Position = ubo.modelViewProj * vec4(in_position, 1.0);
	fragColor = in_color;
	fragTexCoord = in_texCoord;
}
