#include "stdafx.h"
#include "RenderTargetVk.h"
#include "RenderDeviceVk.h"

RenderTarget::RenderTarget(ScopeStack& scope, RenderDevice& renderDevice, uint32_t width, uint32_t height, VkFormat format, VkSampleCountFlagBits samples)
	: m_renderDevice(renderDevice)
	, m_vkImage(nullptr)
	, m_vkImageView(nullptr)
	, m_memAllocInfo{ _dummyMemAllocInfo }
	, m_vkFormat(format)
	, m_vkSamples(samples)
	, m_flags(0)
{
	VkFormatProperties properties = {};
	vkGetPhysicalDeviceFormatProperties(renderDevice.m_vkPhysicalDevices[renderDevice.m_selectedDevice], format, &properties);

	bool bColorAttachment = properties.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
	bool bDepthAttachment = properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
	bool bSampledImage = properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
	VkImageUsageFlags usage = bSampledImage ? VK_IMAGE_USAGE_SAMPLED_BIT : 0;	
	VkImageLayout defaultLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkImageAspectFlags aspectFlags = 0;
	if (bColorAttachment)
	{
		usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		defaultLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	else if (bDepthAttachment)
	{
		usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		defaultLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
	}

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
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

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
	createInfo.subresourceRange.aspectMask = aspectFlags;
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

RenderTarget::RenderTarget(RenderDevice& renderDevice, VkImage image, VkFormat format, VkSampleCountFlagBits samples)
	: m_renderDevice(renderDevice)
	, m_vkImage(image)
	, m_vkImageView(nullptr)
	, m_memAllocInfo{ _dummyMemAllocInfo }
	, m_vkFormat(format)
	, m_vkSamples(samples)
	, m_flags(kRTFlagsExternallyAllocated)
{
	VkFormatProperties properties = {};
	vkGetPhysicalDeviceFormatProperties(renderDevice.m_vkPhysicalDevices[renderDevice.m_selectedDevice], format, &properties);

	bool bColorAttachment = properties.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
	bool bDepthAttachment = properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
	bool bSampledImage = properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
	VkImageUsageFlags usage = bSampledImage ? VK_IMAGE_USAGE_SAMPLED_BIT : 0;
	VkImageLayout defaultLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkImageAspectFlags aspectFlags = 0;
	if (bColorAttachment)
	{
		usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		defaultLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	else if (bDepthAttachment)
	{
		usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		defaultLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
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
	createInfo.subresourceRange.aspectMask = aspectFlags;
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
	if (!(m_flags & kRTFlagsExternallyAllocated))
		vkDestroyImage(m_renderDevice.m_vkDevice, m_vkImage, nullptr);
}

bool RenderTarget::resize(uint32_t width, uint32_t height)
{
	if (m_vkImageView)
	{
		vkDestroyImageView(m_renderDevice.m_vkDevice, m_vkImageView, nullptr);
		vkDestroyImage(m_renderDevice.m_vkDevice, m_vkImage, nullptr);
		m_vkImageView = nullptr;
		m_vkImage = nullptr;
	}

	VkFormatProperties properties = {};
	vkGetPhysicalDeviceFormatProperties(m_renderDevice.m_vkPhysicalDevices[m_renderDevice.m_selectedDevice], m_vkFormat, &properties);

	bool bColorAttachment = properties.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
	bool bDepthAttachment = properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
	bool bSampledImage = properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
	VkImageUsageFlags usage = bSampledImage ? VK_IMAGE_USAGE_SAMPLED_BIT : 0;
	VkImageLayout defaultLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkImageAspectFlags aspectFlags = 0;
	if (bColorAttachment)
	{
		usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		defaultLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	else if (bDepthAttachment)
	{
		usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		defaultLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
	}

	VkImageCreateInfo  imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.pNext = nullptr;
	imageCreateInfo.flags = 0;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = m_vkFormat;
	imageCreateInfo.extent.width = width;
	imageCreateInfo.extent.height = height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = m_vkSamples;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = usage;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.queueFamilyIndexCount = 0;
	imageCreateInfo.pQueueFamilyIndices = nullptr;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	if (vkCreateImage(m_renderDevice.m_vkDevice, &imageCreateInfo, nullptr, &m_vkImage) != VK_SUCCESS)
	{
		print("failed to create image for depth buffer\n");
		exit(1);
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(m_renderDevice.m_vkDevice, m_vkImage, &memRequirements);
	uint32_t memoryTypeIndex = m_renderDevice.getMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	AssertMsg((memRequirements.size < m_memAllocInfo.get().size), "RenderTarget::resize() requires more memory than already allocated.\n");
	if (vkBindImageMemory(m_renderDevice.m_vkDevice, m_vkImage, m_memAllocInfo.get().memoryBlock.m_memory, m_memAllocInfo.get().offset) != VK_SUCCESS)
	{
		print("failed to bind memory to image\n");
		exit(1);
	}

	VkImageViewCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = m_vkImage;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = m_vkFormat;
	createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.subresourceRange.aspectMask = aspectFlags;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(m_renderDevice.m_vkDevice, &createInfo, nullptr, &m_vkImageView) != VK_SUCCESS)
	{
		print("failed to create image view image.\n");
		exit(1);
	}
	return true;
}

bool RenderTarget::recreate(VkImage image)
{
	AssertMsg((m_flags & kRTFlagsExternallyAllocated), "RenderTarget must have been constructed from externally allocate VkImage.\n");
	m_vkImage = image;

	VkFormatProperties properties = {};
	vkGetPhysicalDeviceFormatProperties(m_renderDevice.m_vkPhysicalDevices[m_renderDevice.m_selectedDevice], m_vkFormat, &properties);

	bool bColorAttachment = properties.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
	bool bDepthAttachment = properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
	bool bSampledImage = properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
	VkImageUsageFlags usage = bSampledImage ? VK_IMAGE_USAGE_SAMPLED_BIT : 0;
	VkImageLayout defaultLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkImageAspectFlags aspectFlags = 0;
	if (bColorAttachment)
	{
		usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		defaultLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	else if (bDepthAttachment)
	{
		usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		defaultLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
	}

	if (m_vkImageView)
	{
		vkDestroyImageView(m_renderDevice.m_vkDevice, m_vkImageView, nullptr);
		m_vkImageView = nullptr;
	}
	VkImageViewCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = m_vkImage;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = m_vkFormat;
	createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.subresourceRange.aspectMask = aspectFlags;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(m_renderDevice.m_vkDevice, &createInfo, nullptr, &m_vkImageView) != VK_SUCCESS)
	{
		print("failed to create image view image.\n");
		exit(1);
	}
	return true;
}