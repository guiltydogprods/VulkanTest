#include "stdafx.h"

#pragma once

void print(const char *fmt, ...)
{
	char debugString[16384];				//CLR - Should really calculate the size of the output string instead.

	va_list args;
	va_start(args, fmt);
	vsprintf_s(debugString, sizeof(debugString), fmt, args);
	va_end(args);

	if (IsDebuggerPresent())
		OutputDebugStringA(debugString);
	fputs(debugString, stdout);
}
