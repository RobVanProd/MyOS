#include "pic.h"
#include "io.h"

// Initialization Control Words (ICW) for PIC initialization
#define ICW1    (ICW1_INIT | ICW1_ICW4)  // ICW1: Initialize + ICW4 needed
#define ICW2_MASTER    0x20               // ICW2: IRQ 0-7 mapped to interrupts 0x20-0x27
#define ICW2_SLAVE     0x28               // ICW2: IRQ 8-15 mapped to interrupts 0x28-0x2F
#define ICW3_MASTER    0x04               // ICW3: Slave PIC at IRQ2
#define ICW3_SLAVE     0x02               // ICW3: Cascade identity
#define ICW4           ICW4_8086          // ICW4: 8086 mode, normal EOI

void pic_init(void) {
    uint8_t master_mask, slave_mask;

    // Save masks
    master_mask = inb(PIC1_DATA);
    slave_mask = inb(PIC2_DATA);

    // Start initialization sequence
    outb(PIC1_COMMAND, ICW1);
    io_wait();
    outb(PIC2_COMMAND, ICW1);
    io_wait();

    // Set vector offsets
    outb(PIC1_DATA, ICW2_MASTER);
    io_wait();
    outb(PIC2_DATA, ICW2_SLAVE);
    io_wait();

    // Set up cascading
    outb(PIC1_DATA, ICW3_MASTER);
    io_wait();
    outb(PIC2_DATA, ICW3_SLAVE);
    io_wait();

    // Set operation mode
    outb(PIC1_DATA, ICW4);
    io_wait();
    outb(PIC2_DATA, ICW4);
    io_wait();

    // Restore saved masks
    outb(PIC1_DATA, master_mask);
    outb(PIC2_DATA, slave_mask);
}

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8)
        outb(PIC2_COMMAND, PIC_EOI);
    outb(PIC1_COMMAND, PIC_EOI);
}

void pic_disable(void) {
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

void pic_enable_irq(uint8_t irq) {
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

void pic_disable_irq(uint8_t irq) {
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