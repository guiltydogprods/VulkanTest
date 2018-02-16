#include "stdafx.h"
#include "RenderTargetVk.h"
#include "RenderDeviceVk.h"

RenderTarget::RenderTarget(ScopeStack& scope, RenderDevice& renderDevice, uint32_t width, uint32_t height, VkFormat format, VkSampleCountFlagBits samples)
	: m_renderDevice(renderDevice)
	, m_vkImage(nullptr)
	, m_vkImageView(nullptr)
	, m_memAllocInfo{ _dummyMemAllocInfo }
{
	VkFormatProperties properties = {};
	vkGetPhysicalDeviceFormatProperties(renderDevice.m_vkPhysicalDevices[renderDevice.m_selectedDevice], format, &properties);

	bool bColorAttachment = properties.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
	bool bDepthAttachment = properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
	bool bSampledImage = properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
	VkImageUsageFlags usage = bColorAttachment ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : 0;
	usage |= bDepthAttachment ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : 0;
	usage |= bSampledImage ? VK_IMAGE_USAGE_SAMPLED_BIT : 0;

	VkImageCreateInfo  imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.pNext = nullptr;
	imageCreateInfo.flags = 0;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = format;
	imageCreateInfo.extent.width = width;
	imageCreateInfo.extent.height = height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = samples;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = usage;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.queueFamilyIndexCount = 0;
	imageCreateInfo.pQueueFamilyIndices = nullptr;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

	if (vkCreateImage(renderDevice.m_vkDevice, &imageCreateInfo, nullptr, &m_vkImage) != VK_SUCCESS)
	{
		print("failed to create image for depth buffer\n");
		exit(1);
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(renderDevice.m_vkDevice, m_vkImage, &memRequirements);
	uint32_t memoryTypeIndex = renderDevice.getMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	m_memAllocInfo = RenderDevice::GPUMemoryManager::Instance().allocate(scope, memRequirements.size, memRequirements.alignment, imageCreateInfo.tiling == VK_IMAGE_TILING_OPTIMAL ? (1 << 8) | memoryTypeIndex : memoryTypeIndex);
	if (vkBindImageMemory(renderDevice.m_vkDevice, m_vkImage, m_memAllocInfo.get().memoryBlock.m_memory, m_memAllocInfo.get().offset) != VK_SUCCESS)
	{
		print("failed to bind memory to image\n");
		exit(1);
	}

	VkImageViewCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = m_vkImage;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = format;
	createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(renderDevice.m_vkDevice, &createInfo, nullptr, &m_vkImageView) != VK_SUCCESS)
	{
		print("failed to create image view image.\n");
		exit(1);
	}
}

RenderTarget::~RenderTarget()
{
	vkDestroyImageView(m_renderDevice.m_vkDevice, m_vkImageView, nullptr);
	vkDestroyImage(m_renderDevice.m_vkDevice, m_vkImage, nullptr);
}
