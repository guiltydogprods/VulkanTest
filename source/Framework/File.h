#pragma once

struct File
{
	File(const char *filename);
	~File();

	uint8_t *m_buffer;
	uint32_t m_sizeInBytes;
};
