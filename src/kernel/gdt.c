#include "gdt.h"

#define GDT_ENTRIES 5

// GDT entries
static struct gdt_entry gdt[GDT_ENTRIES];
static struct gdt_ptr gp;

// Set up a GDT entry
void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;

    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);

    gdt[num].access = access;
}

void gdt_init(void) {
    // Set up GDT pointer
    gp.limit = (sizeof(struct gdt_entry) * GDT_ENTRIES) - 1;
    gp.base = (uint32_t)&gdt;

    // Null descriptor
    gdt_set_gate(0, 0, 0, 0, 0);

    // Code segment
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // 0x9A: Present, Ring 0, Code, Readable

    // Data segment
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // 0x92: Present, Ring 0, Data, Writable

    // User mode code segment
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // 0xFA: Present, Ring 3, Code, Readable

    // User mode data segment
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // 0xF2: Present, Ring 3, Data, Writable

    // Flush the GDT
    gdt_flush((uint32_t)&gp);
}