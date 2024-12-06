#include <string.h>
#include <stdarg.h>

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

void* memset(void* dest, int val, size_t len) {
    unsigned char* ptr = dest;
    while (len-- > 0) {
        *ptr++ = val;
    }
    return dest;
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
double atof(const char* s) {
    double result = 0.0;
    double power = 1.0;
    int sign = 1;
    int i = 0;

    // Skip whitespace
    while (s[i] == ' ' || s[i] == '\t') i++;

    // Handle sign
    if (s[i] == '-') {
        sign = -1;
        i++;
    } else if (s[i] == '+') {
        i++;
    }

    // Process integer part
    while (s[i] >= '0' && s[i] <= '9') {
        result = result * 10.0 + (s[i] - '0');
        i++;
    }

    // Process decimal part
    if (s[i] == '.') {
        i++;
        while (s[i] >= '0' && s[i] <= '9') {
            result = result * 10.0 + (s[i] - '0');
            power *= 10.0;
            i++;
        }
    }

    return sign * result / power;
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
