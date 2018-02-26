#pragma once

struct GPUMemAllocInfo;

enum
{ 
	kRTFlagsExternallyAllocated = 1
};

struct RenderTarget
{
	friend struct RenderDevice;

	RenderTarget(ScopeStack& scope, RenderDevice& renderDevice, uint32_t width, uint32_t height, VkFormat format, VkSampleCountFlagBits samples);
	RenderTarget(RenderDevice& renderDevice, VkImage image, VkFormat format, VkSampleCountFlagBits samples);
	~RenderTarget();

	bool resize(uint32_t width, uint32_t height);

	RenderDevice&	m_renderDevice;
	VkImage			m_vkImage;
	VkImageView		m_vkImageView;
	std::reference_wrapper<GPUMemAllocInfo> m_memAllocInfo;
	VkFormat		m_vkFormat;
	VkSampleCountFlagBits m_vkSamples;
	uint32_t		m_flags;

private:
	bool recreate(VkImage image);

};
