#ifndef GDT_H
#define GDT_H

#include <stdint.h>

// GDT entry structure
struct gdt_entry {
    uint16_t limit_low;           // The lower 16 bits of the limit
    uint16_t base_low;            // The lower 16 bits of the base
    uint8_t base_middle;          // The next 8 bits of the base
    uint8_t access;               // Access flags, determine what ring this segment can be used in
    uint8_t granularity;          // Granularity flags
    uint8_t base_high;            // The last 8 bits of the base
} __attribute__((packed));

// GDT pointer structure
struct gdt_ptr {
    uint16_t limit;               // The upper 16 bits of all selector limits
    uint32_t base;                // The address of the first gdt_entry
} __attribute__((packed));

// GDT segment selectors
#define GDT_CODE_SEGMENT 0x08
#define GDT_DATA_SEGMENT 0x10

// GDT access flags
#define GDT_PRESENT        0x80
#define GDT_RING0         0x00
#define GDT_RING3         0x60
#define GDT_SYSTEM        0x10
#define GDT_EXECUTABLE    0x08
#define GDT_CONFORMING    0x04
#define GDT_RW           0x02
#define GDT_ACCESSED      0x01

// GDT granularity flags
#define GDT_GRANULARITY   0x80
#define GDT_32BIT        0x40

// Function declarations
void gdt_init(void);
void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);
extern void gdt_flush(uint32_t);

#endif // GDT_H
