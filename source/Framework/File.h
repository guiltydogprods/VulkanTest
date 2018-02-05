#pragma once

struct File
{
	File(const char *filename, const char *folder = "assets/");
	~File();

	size_t load();
	size_t readBytes(size_t bytesToRead, void* buffer);

	FILE *m_fptr;
	uint8_t *m_buffer;
	uint32_t m_sizeInBytes;
};
