#pragma once

struct MemoryBlock
{
	MemoryBlock(VkDeviceSize size);
	~MemoryBlock();

	VkDeviceMemory m_blockMemory;
	VkDeviceSize m_blockSize;
	uint32_t m_typeIndex;
};
