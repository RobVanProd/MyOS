#include "include/string.h"
#include "include/heap.h"

// Forward declarations
static size_t strspn(const char* str, const char* accept);
static size_t strcspn(const char* str, const char* reject);

// Memory operations declarations
void* memset(void* dest, int val, size_t len) {
    unsigned char* ptr = (unsigned char*)dest;
    while (len-- > 0)
        *ptr++ = val;
    return dest;
}

void* memcpy(void* dest, const void* src, size_t len) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    while (len-- > 0)
        *d++ = *s++;
    return dest;
}

void* memmove(void* dest, const void* src, size_t len) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    if (d < s) {
        while (len-- > 0)
            *d++ = *s++;
    } else {
        d += len;
        s += len;
        while (len-- > 0)
            *--d = *--s;
    }
    return dest;
}

int memcmp(const void* s1, const void* s2, size_t len) {
    const unsigned char* p1 = (const unsigned char*)s1;
    const unsigned char* p2 = (const unsigned char*)s2;
    while (len-- > 0) {
        if (*p1 != *p2)
            return *p1 - *p2;
        p1++;
        p2++;
    }
    return 0;
}

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

size_t strnlen(const char* str, size_t maxlen) {
    size_t len = 0;
    while (len < maxlen && str[len]) len++;
    return len;
}

char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*d++ = *src++));
    return dest;
}

char* strncpy(char* dest, const char* src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i]; i++)
        dest[i] = src[i];
    for (; i < n; i++)
        dest[i] = '\0';
    return dest;
}

char* strcat(char* dest, const char* src) {
    char* d = dest + strlen(dest);
    while ((*d++ = *src++));
    return dest;
}

char* strncat(char* dest, const char* src, size_t n) {
    size_t dest_len = strlen(dest);
    size_t i;
    for (i = 0; i < n && src[i]; i++)
        dest[dest_len + i] = src[i];
    dest[dest_len + i] = '\0';
    return dest;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    if (n == 0) return 0;
    while (n-- > 0 && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

char* strchr(const char* s, int c) {
    while (*s != (char)c) {
        if (!*s++) return NULL;
    }
    return (char*)s;
}

char* strrchr(const char* s, int c) {
    const char* last = NULL;
    do {
        if (*s == (char)c) last = s;
    } while (*s++);
    return (char*)last;
}

char* strstr(const char* haystack, const char* needle) {
    size_t needle_len = strlen(needle);
    if (!needle_len) return (char*)haystack;
    while (*haystack) {
        if (!strncmp(haystack, needle, needle_len))
            return (char*)haystack;
        haystack++;
    }
    return NULL;
}

void strrev(char* str) {
    int i, j;
    char temp;
    for (i = 0, j = strlen(str) - 1; i < j; i++, j--) {
        temp = str[i];
        str[i] = str[j];
        str[j] = temp;
    }
}

void int_to_string(int value, char* buffer) {
    int i = 0;
    int is_negative = 0;

    if (value == 0) {
        buffer[i++] = '0';
        buffer[i] = '\0';
        return;
    }

    if (value < 0) {
        is_negative = 1;
        value = -value;
    }

    while (value != 0) {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }

    if (is_negative)
        buffer[i++] = '-';

    buffer[i] = '\0';
    strrev(buffer);
}

void uint_to_string(unsigned int value, char* buffer) {
    int i = 0;

    if (value == 0) {
        buffer[i++] = '0';
        buffer[i] = '\0';
        return;
    }

    while (value != 0) {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }

    buffer[i] = '\0';
    strrev(buffer);
}

void int_to_hex_string(uint32_t value, char* buffer) {
    const char hex_digits[] = "0123456789ABCDEF";
    int i = 0;

    if (value == 0) {
        buffer[i++] = '0';
        buffer[i] = '\0';
        return;
    }

    while (value != 0) {
        buffer[i++] = hex_digits[value & 0xF];
        value >>= 4;
    }

    buffer[i] = '\0';
    strrev(buffer);
}

char* strtok(char* str, const char* delim) {
    static char* last = NULL;
    return strtok_r(str, delim, &last);
}

char* strtok_r(char* str, const char* delim, char** saveptr) {
    char* token_start;
    char* token_end;

    if (!str && !*saveptr) return NULL;

    token_start = str ? str : *saveptr;
    
    // Skip leading delimiters
    token_start += strspn(token_start, delim);
    if (!*token_start) {
        *saveptr = token_start;
        return NULL;
    }

    // Find end of token
    token_end = token_start + strcspn(token_start, delim);
    if (*token_end) {
        *token_end = '\0';
        *saveptr = token_end + 1;
    } else {
        *saveptr = token_end;
    }

    return token_start;
}

// Helper function for strtok_r
static size_t strspn(const char* str, const char* accept) {
    const char* s;
    size_t count = 0;

    while (*str) {
        for (s = accept; *s; s++) {
            if (*str == *s) break;
        }
        if (!*s) return count;
        str++;
        count++;
    }
    return count;
}

// Helper function for strtok_r
static size_t strcspn(const char* str, const char* reject) {
    const char* s;
    size_t count = 0;

    while (*str) {
        for (s = reject; *s; s++) {
            if (*str == *s) return count;
        }
        str++;
        count++;
    }
    return count;
}

void long_to_string(long value, char* buffer) {
    int i = 0;
    int is_negative = 0;

    if (value == 0) {
        buffer[i++] = '0';
        buffer[i] = '\0';
        return;
    }

    if (value < 0) {
        is_negative = 1;
        value = -value;
    }

    while (value != 0) {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }

    if (is_negative)
        buffer[i++] = '-';

    buffer[i] = '\0';
    strrev(buffer);
}

void ulong_to_string(unsigned long value, char* buffer) {
    int i = 0;

    if (value == 0) {
        buffer[i++] = '0';
        buffer[i] = '\0';
        return;
    }

    while (value != 0) {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }

    buffer[i] = '\0';
    strrev(buffer);
}

int atoi(const char* str) {
    int result = 0;
    int sign = 1;
    
    // Skip whitespace
    while (*str == ' ' || *str == '\t') str++;
    
    // Handle sign
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }
    
    // Convert digits
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    
    return sign * result;
}

long atol(const char* str) {
    long result = 0;
    int sign = 1;
    
    // Skip whitespace
    while (*str == ' ' || *str == '\t') str++;
    
    // Handle sign
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }
    
    // Convert digits
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    
    return sign * result;
}

char* strdup(const char* s) {
    size_t len = strlen(s) + 1;
    char* new_str = heap_alloc(len);
    if (new_str) {
        memcpy(new_str, s, len);
    }
    return new_str;
}

char* strndup(const char* s, size_t n) {
    size_t len = strnlen(s, n);
    char* new_str = heap_alloc(len + 1);
    if (new_str) {
        memcpy(new_str, s, len);
        new_str[len] = '\0';
    }
    return new_str;
}

char* strlwr(char* str) {
    char* p = str;
    while (*p) {
        if (*p >= 'A' && *p <= 'Z')
            *p = *p + ('a' - 'A');
        p++;
    }
    return str;
}

char* strupr(char* str) {
    char* p = str;
    while (*p) {
        if (*p >= 'a' && *p <= 'z')
            *p = *p - ('a' - 'A');
        p++;
    }
    return str;
}

char* strstrip(char* str) {
    char* end;
    
    // Trim leading space
    while (*str == ' ' || *str == '\t' || *str == '\n')
        str++;
    
    if (*str == 0) // All spaces?
        return str;
    
    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n'))
        end--;
    
    // Write new null terminator
    *(end + 1) = '\0';
    
    return str;
}

int abs(int x) {
    return x < 0 ? -x : x;
}
