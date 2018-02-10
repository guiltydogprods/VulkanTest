#pragma once

struct File
{
	File(const char *filename, const char *folder = "assets/");
	~File();

	size_t load(ScopeStack& scope);
	size_t readBytes(size_t bytesToRead, void* buffer);
	int32_t rewindBytes(int32_t bytesToSkip);
	int32_t skipBytes(int32_t bytesToSkip);

	FILE *m_fptr;
	uint8_t *m_buffer;
	uint32_t m_sizeInBytes;
};
