#include "pic.h"
#include "io.h"

// PIC ports
#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

// PIC commands
#define PIC_EOI     0x20
#define ICW1_ICW4   0x01
#define ICW1_SINGLE 0x02
#define ICW1_INTERVAL4  0x04
#define ICW1_LEVEL  0x08
#define ICW1_INIT   0x10
#define ICW4_8086   0x01

void pic_init(void) {
    // Save masks
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);

    // Start initialization sequence in cascade mode
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();

    // Set vector offsets
    outb(PIC1_DATA, 0x20);  // IRQ 0-7: interrupts 0x20-0x27
    io_wait();
    outb(PIC2_DATA, 0x28);  // IRQ 8-15: interrupts 0x28-0x2F
    io_wait();

    // Tell Master PIC there is a slave PIC at IRQ2
    outb(PIC1_DATA, 4);
    io_wait();
    // Tell Slave PIC its cascade identity
    outb(PIC2_DATA, 2);
    io_wait();

    // Set both PICs to 8086 mode
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    // Restore saved masks
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) {
        // Send EOI to slave PIC
        outb(PIC2_COMMAND, PIC_EOI);
    }
    // Send EOI to master PIC
    outb(PIC1_COMMAND, PIC_EOI);
}

void pic_set_mask(uint8_t irq) {
    uint16_t port;
    uint8_t value;
 
    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    value = inb(port) | (1 << irq);
    outb(port, value);        
}

void pic_clear_mask(uint8_t irq) {
    uint16_t port;
    uint8_t value;
 
    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    value = inb(port) & ~(1 << irq);
    outb(port, value);
}

// Read the IRR (Interrupt Request Register)
uint16_t pic_get_irr(void) {
    outb(PIC1_COMMAND, 0x0A);  // OCW3: Read IRR
    outb(PIC2_COMMAND, 0x0A);
    return (inb(PIC2_COMMAND) << 8) | inb(PIC1_COMMAND);
}

// Read the ISR (In-Service Register)
uint16_t pic_get_isr(void) {
    outb(PIC1_COMMAND, 0x0B);  // OCW3: Read ISR
    outb(PIC2_COMMAND, 0x0B);
    return (inb(PIC2_COMMAND) << 8) | inb(PIC1_COMMAND);
}

// Disable the PIC (useful when switching to APIC)
void pic_disable(void) {
    // Mask all interrupts
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

// Enable an IRQ
void pic_enable_irq(uint8_t irq) {
    pic_clear_mask(irq);
}

// Disable an IRQ
void pic_disable_irq(uint8_t irq) {
    pic_set_mask(irq);
}

// Remap PIC vectors
void pic_remap(uint8_t offset1, uint8_t offset2) {
    uint8_t mask1, mask2;

    // Save masks
    mask1 = inb(PIC1_DATA);
    mask2 = inb(PIC2_DATA);

    // Start initialization sequence
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();

    // Set vector offsets
    outb(PIC1_DATA, offset1);
    io_wait();
    outb(PIC2_DATA, offset2);
    io_wait();

    // Set up cascading
    outb(PIC1_DATA, 4);
    io_wait();
    outb(PIC2_DATA, 2);
    io_wait();

    // Set 8086 mode
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    // Restore masks
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}