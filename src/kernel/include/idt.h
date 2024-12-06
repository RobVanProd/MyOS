#ifndef IDT_H
#define IDT_H

#include <stdint.h>

// IDT Gate Types
#define IDT_GATE_TASK      0x5
#define IDT_GATE_INT16     0x6
#define IDT_GATE_TRAP16    0x7
#define IDT_GATE_INT32     0xE
#define IDT_GATE_TRAP32    0xF

// IDT Gate Attributes
#define IDT_PRESENT        0x80
#define IDT_DPL_0          0x00
#define IDT_DPL_1          0x20
#define IDT_DPL_2          0x40
#define IDT_DPL_3          0x60

// IDT Entry Structure
typedef struct {
    uint16_t base_low;      // Lower 16 bits of handler function address
    uint16_t selector;      // Kernel segment selector
    uint8_t  zero;         // Reserved, must be 0
    uint8_t  type_attr;    // Type and attributes
    uint16_t base_high;     // Upper 16 bits of handler function address
} __attribute__((packed)) idt_entry_t;

// IDTR Structure
typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idtr_t;

// Function declarations
void idt_init(void);
void idt_set_gate(uint8_t num, uint32_t base, uint16_t selector, uint8_t flags);
void idt_load(void);

#endif /* IDT_H */
