#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "interrupt.h"
#include "io.h"
#include "terminal.h"
#include "pic.h"
#include "signal.h"

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

// Declare IDT array
static struct idt_entry idt[256];
static struct idt_ptr idtp;

// External assembly functions
extern void idt_load(struct idt_ptr* ptr);

// ISR handlers from assembly
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

// Interrupt nesting level
static volatile int interrupt_depth = 0;
static volatile int in_critical_section = 0;

// Interrupt handlers
static interrupt_handler_t interrupt_handlers[256];

// Error messages for exceptions
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

// Initialize IDT entry
static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].selector = sel;
    idt[num].zero = 0;
    idt[num].flags = flags;
}

// Initialize interrupt handling
void interrupt_init(void) {
    // Set up IDT pointer
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base = (uint32_t)&idt;

    // Clear IDT and handlers
    memset(&idt, 0, sizeof(struct idt_entry) * 256);
    memset(&interrupt_handlers, 0, sizeof(interrupt_handlers));

    // Install ISR handlers
    idt_set_gate(0, (uint32_t)isr0, 0x08, 0x8E);
    idt_set_gate(1, (uint32_t)isr1, 0x08, 0x8E);
    idt_set_gate(2, (uint32_t)isr2, 0x08, 0x8E);
    idt_set_gate(3, (uint32_t)isr3, 0x08, 0x8E);
    idt_set_gate(4, (uint32_t)isr4, 0x08, 0x8E);
    idt_set_gate(5, (uint32_t)isr5, 0x08, 0x8E);
    idt_set_gate(6, (uint32_t)isr6, 0x08, 0x8E);
    idt_set_gate(7, (uint32_t)isr7, 0x08, 0x8E);
    idt_set_gate(8, (uint32_t)isr8, 0x08, 0x8E);
    idt_set_gate(9, (uint32_t)isr9, 0x08, 0x8E);
    idt_set_gate(10, (uint32_t)isr10, 0x08, 0x8E);
    idt_set_gate(11, (uint32_t)isr11, 0x08, 0x8E);
    idt_set_gate(12, (uint32_t)isr12, 0x08, 0x8E);
    idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E);
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);
    idt_set_gate(15, (uint32_t)isr15, 0x08, 0x8E);
    idt_set_gate(16, (uint32_t)isr16, 0x08, 0x8E);
    idt_set_gate(17, (uint32_t)isr17, 0x08, 0x8E);
    idt_set_gate(18, (uint32_t)isr18, 0x08, 0x8E);
    idt_set_gate(19, (uint32_t)isr19, 0x08, 0x8E);
    idt_set_gate(20, (uint32_t)isr20, 0x08, 0x8E);
    idt_set_gate(21, (uint32_t)isr21, 0x08, 0x8E);
    idt_set_gate(22, (uint32_t)isr22, 0x08, 0x8E);
    idt_set_gate(23, (uint32_t)isr23, 0x08, 0x8E);
    idt_set_gate(24, (uint32_t)isr24, 0x08, 0x8E);
    idt_set_gate(25, (uint32_t)isr25, 0x08, 0x8E);
    idt_set_gate(26, (uint32_t)isr26, 0x08, 0x8E);
    idt_set_gate(27, (uint32_t)isr27, 0x08, 0x8E);
    idt_set_gate(28, (uint32_t)isr28, 0x08, 0x8E);
    idt_set_gate(29, (uint32_t)isr29, 0x08, 0x8E);
    idt_set_gate(30, (uint32_t)isr30, 0x08, 0x8E);
    idt_set_gate(31, (uint32_t)isr31, 0x08, 0x8E);

    // Remap PIC
    pic_remap(0x20, 0x28);

    // Load IDT
    idt_load(&idtp);

    // Enable interrupts
    __asm__ volatile("sti");
}

// Register an interrupt handler
void register_interrupt_handler(uint8_t n, interrupt_handler_t handler) {
    if (handler) {
        interrupt_handlers[n] = handler;
    }
}

// Enter critical section
void enter_critical_section(void) {
    __asm__ volatile("cli");
    in_critical_section++;
}

// Exit critical section
void exit_critical_section(void) {
    if (--in_critical_section == 0) {
        __asm__ volatile("sti");
    }
}

// Common interrupt handler
void isr_handler(registers_t regs) {
    interrupt_depth++;

    // Handle CPU exceptions (interrupts 0-31)
    if (regs.int_no < 32) {
        if (regs.int_no < sizeof(exception_messages) / sizeof(char*)) {
            kprintf("Exception: %s\n", exception_messages[regs.int_no]);
        } else {
            kprintf("Unknown exception %d\n", regs.int_no);
        }

        kprintf("Error code: %d\n", regs.err_code);
        kprintf("EIP: 0x%x\n", regs.eip);
        kprintf("CS: 0x%x\n", regs.cs);
        kprintf("EFLAGS: 0x%x\n", regs.eflags);

        // If we're in user mode, also print SS and ESP
        if ((regs.cs & 0x3) == 3) {
            kprintf("ESP: 0x%x\n", regs.useresp);
            kprintf("SS: 0x%x\n", regs.ss);
        }

        // Handle fatal exceptions
        if (regs.int_no == 8 || regs.int_no == 13 || regs.int_no == 14) {
            kprintf("Fatal exception. System halted.\n");
            for(;;);
        }
    }

    // Call registered handler if available
    if (interrupt_handlers[regs.int_no]) {
        interrupt_handlers[regs.int_no](regs);
    } else if (regs.int_no >= 32) {
        kprintf("Unhandled interrupt: %d\n", regs.int_no);
    }

    interrupt_depth--;

    // If we're returning to user mode and there are pending signals,
    // handle them now
    if (interrupt_depth == 0 && (regs.cs & 0x3) == 3) {
        check_pending_signals();
    }
}

// IRQ handler
void irq_handler(registers_t regs) {
    interrupt_depth++;

    // Send an EOI (end of interrupt) signal to the PICs
    if (regs.int_no >= 40) {
        outb(0xA0, 0x20); // Send reset signal to slave
    }
    outb(0x20, 0x20); // Send reset signal to master

    // Call the registered handler if available
    if (interrupt_handlers[regs.int_no]) {
        interrupt_handlers[regs.int_no](regs);
    }

    interrupt_depth--;

    // If we're returning to user mode and there are pending signals,
    // handle them now
    if (interrupt_depth == 0 && (regs.cs & 0x3) == 3) {
        check_pending_signals();
    }
}

// Get current interrupt depth
int get_interrupt_depth(void) {
    return interrupt_depth;
}

// Check if we're in an interrupt context
bool is_interrupt_context(void) {
    return interrupt_depth > 0;
}

// Disable interrupts
void disable_interrupts(void) {
    __asm__ volatile("cli");
}

// Enable interrupts
void enable_interrupts(void) {
    __asm__ volatile("sti");
}
