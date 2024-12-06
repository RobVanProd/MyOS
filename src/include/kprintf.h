#ifndef KPRINTF_H
#define KPRINTF_H

#include <stdarg.h>

// Kernel printf function
void kprintf(const char* format, ...);

// Kernel vprintf function
void kvprintf(const char* format, va_list args);

#endif
