#pragma once
#include "System.h"

class LinearAllocator
{
public:
	LinearAllocator(uint8_t *ptr, size_t size)
		: m_ptr(ptr)
		, m_end(m_ptr + size)
	{
	}

	static size_t alignedSize(size_t size, size_t align = 16)
	{
		return (size + (align - 1)) & ~(align - 1);
	}

	void* allocate(size_t size, size_t align = 16)
	{
		size = alignedSize(size, align);
		uint8_t *result = m_ptr;
		m_ptr += size;
		AssertMsg((result + size <= m_end), "Out of memory.");
#ifdef MEM_DEBUG
//		print("Allocated %d bytes from 0x%016lx\n", size, (size_t)result);
#endif
		return result;
	}

	void *ptr() { return m_ptr; }

	void rewind(void *ptr, bool bLog = true)
	{
		void *oldPtr = m_ptr;
		m_ptr = (uint8_t*)ptr;
#ifdef MEM_DEBUG
		if (bLog)
			print("Rewinding 0x%016lx -> 0x%016lx\n", (size_t)oldPtr, (size_t)ptr);
#else 
		(void)oldPtr;
#endif
	}

private:
	uint8_t *m_ptr;
	uint8_t *m_end;
};
