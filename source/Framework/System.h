#pragma once

void print(const char *fmt, ...);
void AssertMsgFunc(const char *func, const char *file, int line, const char *fmt, ...);

#define AssertMsg(e, fmt, ...)	(((!e)) ? AssertMsgFunc(__FUNCTION__, __FILE__, __LINE__, fmt, ##__VA_ARGS__) : (void)0)
