#include "tss.h"
#include "memory.h"
#include "gdt.h"

// Global TSS structure
static tss_t tss;

// Initialize TSS
void tss_init(uint32_t gdt_entry) {
    // Get base address of TSS
    uint32_t base = (uint32_t)&tss;
    
    // Set up the TSS entry in the GDT
    gdt_set_gate(gdt_entry, base, sizeof(tss_t), 0xE9, 0x00); // Present, Ring 3, TSS
    
    // Zero out TSS
    memset(&tss, 0, sizeof(tss_t));
    
    // Set up some initial values
    tss.ss0 = 0x10;  // Kernel data segment
    tss.esp0 = 0;    // Will be set by tss_set_kernel_stack
    tss.cs = 0x0B;   // User code segment
    tss.ss = tss.ds = tss.es = tss.fs = tss.gs = 0x13; // User data segment
    tss.iomap_base = sizeof(tss_t);
}

// Set kernel stack pointer in TSS
void tss_set_kernel_stack(uint32_t stack) {
    tss.esp0 = stack;
}

// Load TSS
void tss_flush(void) {
    // Load TSS segment
    asm volatile("ltr %%ax" :: "a"(0x2B));  // 0x2B is the TSS segment selector
}
