#include "stdafx.h"
#include "BufferVk.h"
#include "RenderDeviceVk.h"

Buffer::Buffer(ScopeStack& scope, RenderDevice& renderDevice, VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags)
	: m_renderDevice(renderDevice)
	, m_buffer(nullptr)
	, m_memAllocInfo{ nullptr, 0 }
	, m_allocatedSize(0)
	, m_usageFlags(usageFlags)
	, m_memoryPropertyFlags(memoryPropertyFlags)
{

	VkBufferCreateInfo vertexBufferInfo = {};
	vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vertexBufferInfo.size = size;
	vertexBufferInfo.usage = usageFlags;
	vertexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	vkCreateBuffer(renderDevice.m_vkDevice, &vertexBufferInfo, nullptr, &m_buffer);

	VkMemoryRequirements memReqs;
	vkGetBufferMemoryRequirements(renderDevice.m_vkDevice, m_buffer, &memReqs);

	m_allocatedSize = memReqs.size;

	uint32_t memoryTypeIndex = renderDevice.getMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);	// , memAlloc.memoryTypeIndex);
	m_memAllocInfo = RenderDevice::GPUMemoryManager::Instance().allocate(scope, memReqs.size, memReqs.alignment, memoryTypeIndex);

}

Buffer::~Buffer()
{
	vkDestroyBuffer(m_renderDevice.m_vkDevice, m_buffer, nullptr);
}

void Buffer::bindMemory()
{
	VkResult res = vkBindBufferMemory(m_renderDevice.m_vkDevice, m_buffer, m_memAllocInfo.get().memoryBlock.m_memory, m_memAllocInfo.get().offset);
	AssertMsg(res == VK_SUCCESS, "vkBindBuffer failed (res = %d).", static_cast<int32_t>(res));
}

void *Buffer::mapMemory(VkDeviceSize offset, VkDeviceSize size)
{
	AssertMsg((size == VK_WHOLE_SIZE || size <= m_allocatedSize), "Error: requested size too large.\n");
	AssertMsg((m_memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT), "Error: VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT not set.\n");
	void *data = nullptr;
	VkResult res = vkMapMemory(m_renderDevice.m_vkDevice, m_memAllocInfo.get().memoryBlock.m_memory, m_memAllocInfo.get().offset + offset, size, 0, &data);
	AssertMsg(res == VK_SUCCESS, "vkMapMemory failed (res = %d).", static_cast<int32_t>(res));
	return data;
}

void Buffer::unmapMemory()
{
	AssertMsg((m_memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0, "Error: VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT not set.\n");
	vkUnmapMemory(m_renderDevice.m_vkDevice, m_memAllocInfo.get().memoryBlock.m_memory);
}


StagingBuffer::StagingBuffer(RenderDevice& renderDevice, VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags)
	: m_renderDevice(renderDevice)
	, m_buffer(nullptr)
	, m_memory(nullptr)
	, m_allocatedSize(0)
	, m_usageFlags(usageFlags)
	, m_memoryPropertyFlags(memoryPropertyFlags)
{
	AssertMsg((size > 0), "StagingBuffer size is 0 bytes.\n");
	AssertMsg((usageFlags & (VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT)) != 0, "StagingBuffer requires either VK_BUFFER_USAGE_TRANSFER_SRC_BIT or VK_BUFFER_USAGE_TRANSFRER_DST_BIT to be set.\n");
	AssertMsg((memoryPropertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == 0, "StagingBuffer can not be created have VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT set.\n");
	memoryPropertyFlags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT; // Must be set for staging buffer so set it here just in case.

	m_usageFlags = usageFlags;
	m_memoryPropertyFlags = memoryPropertyFlags;

	VkBufferCreateInfo vertexBufferInfo = {};
	vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vertexBufferInfo.size = size;
	vertexBufferInfo.usage = usageFlags;
	vertexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	vkCreateBuffer(renderDevice.m_vkDevice, &vertexBufferInfo, nullptr, &m_buffer);

	VkMemoryRequirements memReqs;
	vkGetBufferMemoryRequirements(renderDevice.m_vkDevice, m_buffer, &memReqs);

	m_allocatedSize = memReqs.size;

	uint32_t memoryTypeIndex = renderDevice.getMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);	// , memAlloc.memoryTypeIndex);
	m_memory = renderDevice.allocateGpuMemory(memReqs.size, memReqs.alignment, memoryTypeIndex);
}

StagingBuffer::~StagingBuffer()
{
	vkDestroyBuffer(m_renderDevice.m_vkDevice, m_buffer, nullptr);
	vkFreeMemory(m_renderDevice.m_vkDevice, m_memory, nullptr);
}

void StagingBuffer::bindMemory()
{
	VkResult res = vkBindBufferMemory(m_renderDevice.m_vkDevice, m_buffer, m_memory, 0);
	AssertMsg(res == VK_SUCCESS, "vkBindBuffer failed (res = %d).", static_cast<int32_t>(res));
}

void *StagingBuffer::mapMemory(VkDeviceSize offset, VkDeviceSize size)
{
	AssertMsg((size == VK_WHOLE_SIZE || size <= m_allocatedSize), "Error: requested size too large.\n");
	AssertMsg((m_memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT), "Error: VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT not set.\n");
	void *data = nullptr;
	VkResult res = vkMapMemory(m_renderDevice.m_vkDevice, m_memory, 0, size, 0, &data);
	AssertMsg(res == VK_SUCCESS, "vkMapMemory failed (res = %d).", static_cast<int32_t>(res));
	return data;
}

void StagingBuffer::unmapMemory()
{
	AssertMsg((m_memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0, "Error: VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT not set.\n");
	vkUnmapMemory(m_renderDevice.m_vkDevice, m_memory);
}
