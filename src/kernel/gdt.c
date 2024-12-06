#include "gdt.h"

#define GDT_ENTRIES 5

// GDT entries
static struct gdt_entry gdt[GDT_ENTRIES];
static struct gdt_ptr gp;

// External assembly function
extern void gdt_flush(uint32_t);

// Set up a GDT entry
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    // Set descriptor base address
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;

    // Set descriptor limits
    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F);

    // Set granularity bits
    gdt[num].granularity |= (gran & 0xF0);
    gdt[num].access = access;
}

void gdt_install() {
    // Setup the GDT pointer and limit
    gp.limit = (sizeof(struct gdt_entry) * GDT_ENTRIES) - 1;
    gp.base = (uint32_t)&gdt;

    // NULL descriptor
    gdt_set_gate(0, 0, 0, 0, 0);

    // Code Segment
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

    // Data Segment
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    // User Code Segment
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);

    // User Data Segment
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

    // Flush the GDT
    gdt_flush((uint32_t)&gp);
}