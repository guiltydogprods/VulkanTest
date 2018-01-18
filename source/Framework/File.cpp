#include "stdafx.h"
#include "File.h"

File::File(const char *filename, const char *folder)
	: m_buffer(nullptr)
	, m_sizeInBytes(0)
{
	char pathname[4096];
	strcpy_s(pathname, sizeof(pathname), folder);
	strcat_s(pathname, sizeof(pathname), filename);

	FILE *fptr = nullptr;
	errno_t err = fopen_s(&fptr, pathname, "rb");
	fseek(fptr, 0L, SEEK_END);
	m_sizeInBytes = (uint32_t)ftell(fptr);
	fseek(fptr, 0L, SEEK_SET);

	m_buffer = static_cast<uint8_t *>(malloc(m_sizeInBytes));
	fread(m_buffer, m_sizeInBytes, 1, fptr);
	fclose(fptr);
}

File::~File()
{
	free(m_buffer);
}