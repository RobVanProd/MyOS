#ifndef STRING_H
#define STRING_H

#include <stddef.h>
#include <stdint.h>

// String length functions
size_t strlen(const char* str);
size_t strnlen(const char* str, size_t maxlen);

// String copy functions
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);
char* strcat(char* dest, const char* src);
char* strncat(char* dest, const char* src, size_t n);

// String comparison functions
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, size_t n);

// String search functions
char* strchr(const char* s, int c);
char* strrchr(const char* s, int c);
char* strstr(const char* haystack, const char* needle);

// String tokenization
char* strtok(char* str, const char* delim);
char* strtok_r(char* str, const char* delim, char** saveptr);

// String conversion functions
void int_to_string(int value, char* buffer);
void uint_to_string(unsigned int value, char* buffer);
void int_to_hex_string(uint32_t value, char* buffer);
void long_to_string(long value, char* buffer);
void ulong_to_string(unsigned long value, char* buffer);
int atoi(const char* str);
long atol(const char* str);

// String utility functions
char* strdup(const char* s);
char* strndup(const char* s, size_t n);
void strrev(char* str);
char* strlwr(char* str);
char* strupr(char* str);
char* strstrip(char* str);

#endif /* STRING_H */
