#include "stdafx.h"
#include "TextureVk.h"
#include "BufferVk.h"
#include "RenderDeviceVk.h"

Texture::Texture(ScopeStack& scope, RenderDevice& renderDevice, const char *filename)
	: m_renderDevice(renderDevice)
	, m_vkImage(nullptr)
	, m_vkImageView(nullptr)
	, m_memAllocInfo{_dummyMemAllocInfo}
{
	char modifiedFilename[1024];
	strcpy_s(modifiedFilename, sizeof(modifiedFilename), filename);
	strncpy_s(modifiedFilename + (strlen(filename) - 3), sizeof(modifiedFilename), "dds", 3);
	File file(modifiedFilename);

	uint8_t buffer[sizeof(DDS_HEADER) + 4];
	size_t bytesRead = file.readBytes(sizeof(buffer), buffer);

	const char *ptr = (const char *)buffer;
	if (strncmp(ptr, "DDS ", 4) != 0)
	{
		print("File is not in .dds format.\n");
		exit(EXIT_FAILURE);
	}

	DDS_HEADER *header = reinterpret_cast<DDS_HEADER *>(buffer + 4);
	uint32_t headerSize = header->dwSize;
	uint32_t flags = header->dwFlags;
	uint32_t texHeight = header->dwHeight;
	uint32_t texWidth = header->dwWidth;
	uint32_t linearSize = header->dwPitchOrLinearSize;
	uint32_t texDepth = header->dwDepth;
	uint32_t mipmapCount = header->dwMipMapCount;
	uint32_t fourCC = header->ddspf.dwFourCC;

	uint32_t components = (fourCC == FOURCC_DXT1) ? 3 : 4;

	VkFormat format;
	switch (fourCC)
	{
	case FOURCC_DXT1:
		format = VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
		break;
	case FOURCC_DXT3:
		format = VK_FORMAT_BC3_UNORM_BLOCK;
		break;
	case FOURCC_DXT5:
		format = VK_FORMAT_BC5_UNORM_BLOCK;
		break;
	default:
		return;
	}

	uint32_t blockSize = (format == VK_FORMAT_BC1_RGB_UNORM_BLOCK || format == VK_FORMAT_BC1_RGBA_UNORM_BLOCK) ? 8 : 16;
	uint64_t size = file.m_sizeInBytes - 128;

	StagingBuffer stagingBuffer(renderDevice, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	uint8_t *data = static_cast<uint8_t *>(stagingBuffer.mapMemory());
	if (data)
	{
		file.readBytes(size, data);
		stagingBuffer.unmapMemory();
		stagingBuffer.bindMemory();
	}

	// Setup buffer copy regions for each mip level
	VkBufferImageCopy *bufferCopyRegions = static_cast<VkBufferImageCopy *>(alloca(sizeof(VkBufferImageCopy) * mipmapCount));
	memset(bufferCopyRegions, 0, sizeof(VkBufferImageCopy) * mipmapCount);
	uint32_t offset = 0;
	uint32_t mipWidth = texWidth;
	uint32_t mipHeight = texHeight;
	for (uint32_t i = 0; i < mipmapCount && (mipWidth || mipHeight); i++)
	{
		uint32_t regionSize = ((mipWidth + 3) / 4) * ((mipHeight + 3) / 4) * blockSize;

		VkBufferImageCopy &bufferCopyRegion = bufferCopyRegions[i];
		bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		bufferCopyRegion.imageSubresource.mipLevel = i;
		bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
		bufferCopyRegion.imageSubresource.layerCount = 1;
		bufferCopyRegion.imageExtent.width = mipWidth;
		bufferCopyRegion.imageExtent.height = mipHeight;
		bufferCopyRegion.imageExtent.depth = 1;
		bufferCopyRegion.bufferOffset = offset;
		offset += regionSize > 8 ? regionSize : 8;
		mipWidth /= 2;
		mipHeight /= 2;
	}

	VkImageCreateInfo  imageCreateInfo = {};
	createImage(texWidth, texHeight, mipmapCount, format, imageCreateInfo);
	allocate(scope, imageCreateInfo);

	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = mipmapCount;
	subresourceRange.layerCount = 1;

	VkCommandBuffer commandBuffer = renderDevice.beginSingleUseCommandBuffer();
	renderDevice.transitionImageLayout(commandBuffer, m_vkImage, subresourceRange, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	// Copy mip levels from staging buffer
	vkCmdCopyBufferToImage(commandBuffer, stagingBuffer.m_buffer, m_vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipmapCount, bufferCopyRegions);
	renderDevice.transitionImageLayout(commandBuffer, m_vkImage, subresourceRange, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	renderDevice.endSingleUseCommandBuffer(commandBuffer);

	createImageView(format, subresourceRange);
	createSampler(mipmapCount);
}

Texture::Texture(ScopeStack& scope, RenderDevice& renderDevice, uint32_t width, uint32_t height, VkFormat format) 
	: m_renderDevice(renderDevice)
	, m_vkImage(nullptr)
	, m_vkImageView(nullptr)
	, m_memAllocInfo{ _dummyMemAllocInfo }
{
	uint32_t mipmapCount = calcNumMips(width, height);

	VkImageCreateInfo  imageCreateInfo = {};
	createImage(width, height, mipmapCount, format, imageCreateInfo);
	allocate(scope, imageCreateInfo);

	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = mipmapCount;
	subresourceRange.layerCount = 1;

	//	VkCommandBuffer commandBuffer = renderDevice.beginSingleUseCommandBuffer();
	//	renderDevice.transitionImageLayout(commandBuffer, m_vkImage, subresourceRange, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	// Copy mip levels from staging buffer
	//	vkCmdCopyBufferToImage(commandBuffer, stagingBuffer.m_buffer, m_vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipMapCount, bufferCopyRegions);
	//	renderDevice.transitionImageLayout(commandBuffer, m_vkImage, subresourceRange, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	//	renderDevice.endSingleUseCommandBuffer(commandBuffer);

	createImageView(format, subresourceRange);
	createSampler(mipmapCount);
}

Texture::~Texture()
{
	vkDestroySampler(m_renderDevice.m_vkDevice, m_vkSampler, nullptr);
	vkDestroyImageView(m_renderDevice.m_vkDevice, m_vkImageView, nullptr);
	vkDestroyImage(m_renderDevice.m_vkDevice, m_vkImage, nullptr);
}

void Texture::createImage(uint32_t width, uint32_t height, uint32_t mipmapCount, VkFormat format, VkImageCreateInfo& imageCreateInfo)
{
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.pNext = nullptr;
	imageCreateInfo.flags = 0;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = format;
	imageCreateInfo.extent.width = width;
	imageCreateInfo.extent.height = height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = mipmapCount;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.queueFamilyIndexCount = 0;
	imageCreateInfo.pQueueFamilyIndices = nullptr;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	if (vkCreateImage(m_renderDevice.m_vkDevice, &imageCreateInfo, nullptr, &m_vkImage) != VK_SUCCESS)
	{
		print("failed to create empty image.\n");
		exit(1);
	}
}

void Texture::allocate(ScopeStack& scope, const VkImageCreateInfo& imageCreateInfo)
{
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(m_renderDevice.m_vkDevice, m_vkImage, &memRequirements);

	uint32_t memoryTypeIndex = m_renderDevice.getMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	m_memAllocInfo = RenderDevice::GPUMemoryManager::Instance().allocate(scope, memRequirements.size, memRequirements.alignment, imageCreateInfo.tiling == VK_IMAGE_TILING_OPTIMAL ? (1 << 8) | memoryTypeIndex : memoryTypeIndex);
	if (vkBindImageMemory(m_renderDevice.m_vkDevice, m_vkImage, m_memAllocInfo.get().memoryBlock.m_memory, m_memAllocInfo.get().offset) != VK_SUCCESS)
	{
		print("failed to bind memory to image\n");
		exit(1);
	}
}

void Texture::createImageView(VkFormat format, const VkImageSubresourceRange& subresourceRange)
{
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = m_vkImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange = subresourceRange;
	if (vkCreateImageView(m_renderDevice.m_vkDevice, &viewInfo, nullptr, &m_vkImageView) != VK_SUCCESS)
	{
		print("failed to create image view\n");
		exit(EXIT_FAILURE);
	}
}

void Texture::createSampler(uint32_t mipmapCount)
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = static_cast<float>(mipmapCount);
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 8;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
	if (vkCreateSampler(m_renderDevice.m_vkDevice, &samplerInfo, nullptr, &m_vkSampler) != VK_SUCCESS)
	{
		print("failed to create sampler.\n");
		exit(EXIT_FAILURE);
	}
}

uint32_t Texture::calcNumMips(uint32_t width, uint32_t height)
{
	uint32_t maxDim = std::max(width, height);
	if (maxDim == 0)
		return 0;

	uint32_t numMips = 1;
	while (maxDim >>= 1)
		++numMips;
	return numMips;
}
