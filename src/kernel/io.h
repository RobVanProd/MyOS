#ifndef IO_H
#define IO_H

#include <stdint.h>

// Output a byte to a port
static inline void outb(uint16_t port, uint8_t value) {
    asm volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

// Input a byte from a port
static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    asm volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

// Wait for I/O operation to complete
static inline void io_wait(void) {
    outb(0x80, 0);
}

#endif 