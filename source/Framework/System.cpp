#include "stdafx.h"

#pragma once

void AssertMsgFunc(const char *func, const char *file, int line, const char *fmt, ...)
{
	char debugString[16384];

	va_list args;
	va_start(args, fmt);
	vsprintf_s(debugString, sizeof(debugString), fmt, args);
	va_end(args);

	print("Assert: %s\n  function %s, file %s, line %d.\n", debugString, func, file, line);
#if defined(WIN32)
	DebugBreak();
#endif
}

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
