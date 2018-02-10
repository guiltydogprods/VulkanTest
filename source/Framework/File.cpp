#include "stdafx.h"
#include "File.h"

File::File(const char *filename, const char *folder)
	: m_fptr(nullptr)
	, m_buffer(nullptr)
	, m_sizeInBytes(0)
{
	char pathname[512];
	strcpy_s(pathname, sizeof(pathname), folder);
	strcat_s(pathname, sizeof(pathname), filename);

	errno_t err = fopen_s(&m_fptr, pathname, "rb");
	fseek(m_fptr, 0L, SEEK_END);
	m_sizeInBytes = (uint32_t)ftell(m_fptr);
	fseek(m_fptr, 0L, SEEK_SET);
}

size_t File::load(ScopeStack& scope)
{
	m_buffer = static_cast<uint8_t *>(scope.allocate(m_sizeInBytes));
	size_t bytesRead = fread(m_buffer, 1, m_sizeInBytes, m_fptr);
	AssertMsg(bytesRead == m_sizeInBytes, "Warning: File::load failed.\n");

	return bytesRead;
}

size_t File::readBytes(size_t bytesToRead, void *buffer)
{
	size_t bytesRead = fread(buffer, 1, bytesToRead, m_fptr);
	AssertMsg(bytesRead == bytesToRead, "Warning: File::readBytes failed.\n");

	return bytesRead;
}

int32_t File::rewindBytes(int32_t bytesToSkip)
{
	fseek(m_fptr, -bytesToSkip, SEEK_CUR);
	return(ftell(m_fptr));
}

int32_t File::skipBytes(int32_t bytesToSkip)
{
	fseek(m_fptr, bytesToSkip, SEEK_CUR);
	return(ftell(m_fptr));
}

File::~File()
{
	fclose(m_fptr);
}