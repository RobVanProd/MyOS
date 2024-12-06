#ifndef ISR_H
#define ISR_H

#include <stdint.h>

// Structure to hold register values when an ISR is run
typedef struct {
    uint32_t ds;                                     // Data segment
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha
    uint32_t int_no, err_code;                       // Interrupt number and error code
    uint32_t eip, cs, eflags, useresp, ss;          // Pushed by the processor automatically
} registers_t;

// ISR handler function type
typedef void (*isr_handler_t)(registers_t*);

// Function to register an interrupt handler
void register_interrupt_handler(uint8_t n, isr_handler_t handler);

#endif
