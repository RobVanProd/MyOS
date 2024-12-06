#ifndef IDT_H
#define IDT_H

#include <stdint.h>

// Interrupt handler function type
typedef uint32_t isr_t;

// IDT entry structure
struct idt_entry {
    uint16_t base_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t flags;
    uint16_t base_high;
} __attribute__((packed));

// IDT pointer structure
struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

// Initialize the IDT
void idt_init(void);

// Install an interrupt handler
void idt_set_gate(uint8_t num, uint32_t handler, uint16_t selector, uint8_t flags);

// Load the IDT
extern void idt_load(struct idt_ptr* idt_ptr_addr);

#endif /* IDT_H */