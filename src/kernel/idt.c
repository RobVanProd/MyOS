#include "idt.h"
#include "terminal.h"

#define IDT_ENTRIES 256

static struct idt_entry idt[IDT_ENTRIES];
static struct idt_ptr idt_pointer;

// External assembly functions
extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);

void idt_set_gate(uint8_t num, isr_t handler, uint16_t selector, uint8_t flags) {
    idt[num].base_low = (uint32_t)handler & 0xFFFF;
    idt[num].base_high = ((uint32_t)handler >> 16) & 0xFFFF;
    idt[num].selector = selector;
    idt[num].zero = 0;
    idt[num].flags = flags;
}

void idt_init(void) {
    // Set up IDT pointer
    idt_pointer.limit = sizeof(struct idt_entry) * IDT_ENTRIES - 1;
    idt_pointer.base = (uint32_t)&idt;

    // Clear IDT
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate(i, 0, 0, 0);
    }

    // Install CPU exception handlers
    idt_set_gate(0, isr0, 0x08, 0x8E);
    idt_set_gate(1, isr1, 0x08, 0x8E);
    idt_set_gate(2, isr2, 0x08, 0x8E);
    idt_set_gate(3, isr3, 0x08, 0x8E);
    idt_set_gate(4, isr4, 0x08, 0x8E);
    idt_set_gate(5, isr5, 0x08, 0x8E);
    idt_set_gate(6, isr6, 0x08, 0x8E);
    idt_set_gate(7, isr7, 0x08, 0x8E);
    idt_set_gate(8, isr8, 0x08, 0x8E);
    idt_set_gate(9, isr9, 0x08, 0x8E);
    idt_set_gate(10, isr10, 0x08, 0x8E);
    idt_set_gate(11, isr11, 0x08, 0x8E);
    idt_set_gate(12, isr12, 0x08, 0x8E);
    idt_set_gate(13, isr13, 0x08, 0x8E);
    idt_set_gate(14, isr14, 0x08, 0x8E);
    idt_set_gate(15, isr15, 0x08, 0x8E);
    idt_set_gate(16, isr16, 0x08, 0x8E);
    idt_set_gate(17, isr17, 0x08, 0x8E);
    idt_set_gate(18, isr18, 0x08, 0x8E);
    idt_set_gate(19, isr19, 0x08, 0x8E);
    idt_set_gate(20, isr20, 0x08, 0x8E);
    idt_set_gate(21, isr21, 0x08, 0x8E);
    idt_set_gate(22, isr22, 0x08, 0x8E);
    idt_set_gate(23, isr23, 0x08, 0x8E);
    idt_set_gate(24, isr24, 0x08, 0x8E);
    idt_set_gate(25, isr25, 0x08, 0x8E);
    idt_set_gate(26, isr26, 0x08, 0x8E);
    idt_set_gate(27, isr27, 0x08, 0x8E);
    idt_set_gate(28, isr28, 0x08, 0x8E);
    idt_set_gate(29, isr29, 0x08, 0x8E);
    idt_set_gate(30, isr30, 0x08, 0x8E);
    idt_set_gate(31, isr31, 0x08, 0x8E);

    // Load IDT
    idt_load(&idt_pointer);
}

// Exception messages for debugging
static const char* exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

// Interrupt handler
void isr_handler(void) {
    terminal_writestring("\nReceived interrupt: ");
    terminal_writestring(exception_messages[0]); // We'll improve this later
    terminal_writestring("\n");
} 