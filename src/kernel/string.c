#include <string.h>
#include <stdarg.h>

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*dest++ = *src++));
    return d;
}

char* strncpy(char* dest, const char* src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    return dest;
}

char* strchr(const char* str, int c) {
    while (*str) {
        if (*str == (char)c) {
            return (char*)str;
        }
        str++;
    }
    return NULL;
}

int memcmp(const void* s1, const void* s2, size_t n) {
    const unsigned char* p1 = s1;
    const unsigned char* p2 = s2;
    while (n--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    return 0;
}

void* memset(void* s, int c, size_t n) {
    unsigned char* p = s;
    while (n--) {
        *p++ = (unsigned char)c;
    }
    return s;
}

void* memcpy(void* dest, const void* src, size_t n) {
    unsigned char* d = dest;
    const unsigned char* s = src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

// Simple floating point to string conversion
static void ftoa(double num, char* str, int precision) {
    int whole = (int)num;
    double frac = num - whole;
    
    // Convert whole part
    int i = 0;
    if (whole == 0) {
        str[i++] = '0';
    } else {
        int rev = 0;
        while (whole > 0) {
            rev = rev * 10 + (whole % 10);
            whole /= 10;
        }
        while (rev > 0) {
            str[i++] = '0' + (rev % 10);
            rev /= 10;
        }
    }
    
    // Add decimal point and fractional part
    if (precision > 0) {
        str[i++] = '.';
        while (precision-- > 0) {
            frac *= 10;
            int digit = (int)frac;
            str[i++] = '0' + digit;
            frac -= digit;
        }
    }
    
    str[i] = '\0';
}

// Simple string to float conversion
double atof(const char* str) {
    double result = 0;
    double factor = 1;
    int decimal = 0;
    
    // Handle negative numbers
    if (*str == '-') {
        factor = -1;
        str++;
    }
    
    // Process digits before decimal point
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    
    // Process decimal point and following digits
    if (*str == '.') {
        str++;
        double power = 0.1;
        while (*str >= '0' && *str <= '9') {
            result += (*str - '0') * power;
            power *= 0.1;
            str++;
        }
    }
    
    return result * factor;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int sprintf(char* str, const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    char* s = str;
    while (*format) {
        if (*format != '%') {
            *s++ = *format++;
            continue;
        }
        
        format++;  // Skip '%'
        
        switch (*format) {
            case 'd': {
                int val = va_arg(args, int);
                if (val < 0) {
                    *s++ = '-';
                    val = -val;
                }
                
                // Convert number to string
                char tmp[32];
                int i = 0;
                do {
                    tmp[i++] = '0' + (val % 10);
                    val /= 10;
                } while (val);
                
                while (i > 0) {
                    *s++ = tmp[--i];
                }
                break;
            }
            
            case 'f': {
                double val = va_arg(args, double);
                if (val < 0) {
                    *s++ = '-';
                    val = -val;
                }
                ftoa(val, s, 6);  // 6 decimal places
                while (*s) s++;
                break;
            }
            
            case 's': {
                char* val = va_arg(args, char*);
                while (*val) {
                    *s++ = *val++;
                }
                break;
            }
            
            case 'c': {
                char val = (char)va_arg(args, int);
                *s++ = val;
                break;
            }
            
            default:
                *s++ = *format;
                break;
        }
        
        format++;
    }
    
    *s = '\0';
    va_end(args);
    return s - str;
}
