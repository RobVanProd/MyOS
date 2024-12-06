#ifndef STRING_H
#define STRING_H

#include <stddef.h>
#include <stdint.h>

// String manipulation functions
void* memcpy(void* dest, const void* src, size_t n);
void* memmove(void* dest, const void* src, size_t n);
void* memset(void* s, int c, size_t n);
int memcmp(const void* s1, const void* s2, size_t n);
void* memchr(const void* s, int c, size_t n);

size_t strlen(const char* s);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);
char* strcat(char* dest, const char* src);
char* strncat(char* dest, const char* src, size_t n);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, size_t n);
char* strchr(const char* s, int c);
char* strrchr(const char* s, int c);
char* strstr(const char* haystack, const char* needle);
char* strdup(const char* s);

// Additional utility functions
void reverse(char* s);
char* itoa(int n, char* s);
char* utoa(unsigned int n, char* s);
char* ltoa(long n, char* s);
char* ultoa(unsigned long n, char* s);
int atoi(const char* s);
long atol(const char* s);

// Memory block operations
void* memccpy(void* dest, const void* src, int c, size_t n);
void bzero(void* s, size_t n);
void bcopy(const void* src, void* dest, size_t n);

// String tokenization
char* strtok(char* str, const char* delim);
char* strtok_r(char* str, const char* delim, char** saveptr);

// Case conversion
int toupper(int c);
int tolower(int c);
char* strupr(char* s);
char* strlwr(char* s);

// String trimming
char* trim(char* s);
char* ltrim(char* s);
char* rtrim(char* s);

// Safe string functions
size_t strlcpy(char* dest, const char* src, size_t size);
size_t strlcat(char* dest, const char* src, size_t size);

#endif // STRING_H
