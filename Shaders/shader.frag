#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler texSampler;
layout(binding = 2) uniform texture2D textures[2];
 
layout (push_constant) uniform pushConstants_t
{
    layout (offset = 0) uint drawId;
} pushConstants;

void main()
{
	vec4 albedo = texture(sampler2D(textures[pushConstants.drawId], texSampler), fragTexCoord);
	outColor = vec4(fragColor * albedo.rgb, albedo.a);
}
