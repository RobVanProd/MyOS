#ifndef STDLIB_H
#define STDLIB_H

double atof(const char* str);
int atoi(const char* str);
long atol(const char* str);

void* malloc(size_t size);
void free(void* ptr);
void* realloc(void* ptr, size_t size);
void* calloc(size_t num, size_t size);

int abs(int n);
long labs(long n);

#endif
