#pragma once

struct File
{
	File(const char *filename, const char *folder = "assets/");
	~File();

	size_t File::load();

	FILE *m_fptr;
	uint8_t *m_buffer;
	uint32_t m_sizeInBytes;
};
