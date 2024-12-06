#ifndef _INTERRUPT_H
#define _INTERRUPT_H

#include <stdint.h>

// Registers structure
typedef struct {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
} registers_t;

// Interrupt handler function type
typedef void (*interrupt_handler_t)(registers_t);

// Function declarations
void interrupt_init(void);
void register_interrupt_handler(uint8_t n, interrupt_handler_t handler);
void isr_handler(registers_t regs);
void irq_handler(registers_t regs);

#endif
