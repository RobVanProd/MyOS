#ifndef _STRING_H
#define _STRING_H

#include <stdint.h>

size_t strlen(const char* str);
void* memset(void* dest, int val, size_t len);
void* memcpy(void* dest, const void* src, size_t len);
int memcmp(const void* ptr1, const void* ptr2, size_t len);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, size_t n);
char* strchr(const char* str, int ch);
char* strrchr(const char* str, int ch);

#endif
