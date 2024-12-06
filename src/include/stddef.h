#ifndef _STDDEF_H
#define _STDDEF_H

#include <stdint.h>

typedef uint32_t size_t;
typedef int32_t ptrdiff_t;

#define NULL ((void*)0)

#define offsetof(type, member) __builtin_offsetof(type, member)

#endif
