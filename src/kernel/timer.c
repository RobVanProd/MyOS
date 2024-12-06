#include "timer.h"
#include "io.h"
#include "isr.h"
#include "process.h"

// Timer variables
static uint32_t tick = 0;
static uint32_t frequency = 0;

// Timer callback
static void timer_callback(registers_t* regs) {
    tick++;
    
    // Acknowledge interrupt
    hal_pic_eoi(0);
    
    // Schedule next process if needed
    process_schedule();
}

// Initialize timer
void timer_init(uint32_t freq) {
    frequency = freq;
    
    // Calculate divisor
    uint32_t divisor = 1193180 / freq;
    
    // Send command byte
    outb(0x43, 0x36);
    
    // Send frequency divisor
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
    
    // Register timer callback
    register_interrupt_handler(IRQ0, timer_callback);
}

// Wait for specified number of ticks
void timer_wait(uint32_t ticks) {
    uint32_t end_tick = tick + ticks;
    while (tick < end_tick) {
        asm volatile("hlt");
    }
}

// Get current tick count
uint32_t get_timer_ticks(void) {
    return tick;
}

// Sleep for specified number of milliseconds
void sleep(uint32_t ms) {
    if (frequency > 0) {
        timer_wait((ms * frequency) / 1000);
    }
}
