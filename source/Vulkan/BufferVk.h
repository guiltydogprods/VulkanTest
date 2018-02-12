#pragma once

#include "RenderDeviceVk.h"

struct Buffer
{
	Buffer(ScopeStack& scope, RenderDevice& renderDevice, VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags);
	~Buffer();

	void bindMemory();
	void *mapMemory(VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);
	void unmapMemory();

protected:
	Buffer(RenderDevice& renderDevice)
		: m_renderDevice(renderDevice)
		, m_buffer(nullptr)
		, m_memAllocInfo(_dummyMemAllocInfo)
		, m_allocatedSize(0)
		, m_usageFlags(0)
		, m_memoryPropertyFlags(0) {}

public:
	RenderDevice & m_renderDevice;
	VkBuffer							 m_buffer;
	std::reference_wrapper<GPUMemAllocInfo> m_memAllocInfo;
	VkDeviceSize						 m_allocatedSize;
	VkBufferUsageFlags					 m_usageFlags;
	VkMemoryPropertyFlags				 m_memoryPropertyFlags;
};

struct StagingBuffer
{
public:
	StagingBuffer(RenderDevice& renderDevice, VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags);
	~StagingBuffer();

	void bindMemory();
	void *mapMemory(VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);
	void unmapMemory();

	RenderDevice&			m_renderDevice;
	VkBuffer				m_buffer;
	VkDeviceMemory			m_memory;
	VkDeviceSize			m_allocatedSize;
	VkBufferUsageFlags		m_usageFlags;
	VkMemoryPropertyFlags	m_memoryPropertyFlags;
};
