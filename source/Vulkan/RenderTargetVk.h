#pragma once

struct GPUMemAllocInfo;

struct RenderTarget
{
	RenderTarget(ScopeStack& scope, RenderDevice& renderDevice, uint32_t width, uint32_t height, VkFormat format, VkSampleCountFlagBits samples);
	~RenderTarget();

	RenderDevice&	m_renderDevice;
	VkImage			m_vkImage;
	VkImageView		m_vkImageView;
	std::reference_wrapper<GPUMemAllocInfo> m_memAllocInfo;
};
