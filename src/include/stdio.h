#ifndef STDIO_H
#define STDIO_H

#include <stdarg.h>

int sprintf(char* str, const char* format, ...);
int vsprintf(char* str, const char* format, va_list args);
int printf(const char* format, ...);
int vprintf(const char* format, va_list args);

#endif
