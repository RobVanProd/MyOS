#ifndef IO_H
#define IO_H

#include <stdint.h>

// IO port operations for byte (8-bit)
static inline void outb(uint16_t port, uint8_t value) {
    asm volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    asm volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

// IO port operations for word (16-bit)
static inline void outw(uint16_t port, uint16_t value) {
    asm volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t value;
    asm volatile ("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

// IO port operations for long (32-bit)
static inline void outl(uint16_t port, uint32_t value) {
    asm volatile ("outl %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t value;
    asm volatile ("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

// IO port operations for string
static inline void outsb(uint16_t port, const uint8_t* data, uint32_t count) {
    asm volatile ("rep outsb" : "+S"(data), "+c"(count) : "d"(port));
}

static inline void insb(uint16_t port, uint8_t* data, uint32_t count) {
    asm volatile ("rep insb" : "+D"(data), "+c"(count) : "d"(port));
}

static inline void outsw(uint16_t port, const uint16_t* data, uint32_t count) {
    asm volatile ("rep outsw" : "+S"(data), "+c"(count) : "d"(port));
}

static inline void insw(uint16_t port, uint16_t* data, uint32_t count) {
    asm volatile ("rep insw" : "+D"(data), "+c"(count) : "d"(port));
}

static inline void outsl(uint16_t port, const uint32_t* data, uint32_t count) {
    asm volatile ("rep outsl" : "+S"(data), "+c"(count) : "d"(port));
}

static inline void insl(uint16_t port, uint32_t* data, uint32_t count) {
    asm volatile ("rep insl" : "+D"(data), "+c"(count) : "d"(port));
}

// IO wait
static inline void io_wait(void) {
    outb(0x80, 0);
}

#endif /* IO_H */
