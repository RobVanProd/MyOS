 #include <mouse.h>
#include <io.h>
#include <pic.h>
#include <graphics.h>
#include <isr.h>

// Define mouse state structure
typedef struct {
    int x;
    int y;
    int dx;
    int dy;
    uint8_t buttons;
} mouse_event_t;

static mouse_event_t mouse_state = {0, 0, 0, 0, 0};
static mouse_callback_t mouse_callback = NULL;
static uint8_t mouse_cycle = 0;
static uint8_t mouse_byte[3];

static void mouse_wait(uint8_t type) {
    uint32_t timeout = 100000;
    if (type == 0) {
        while (timeout--) {
            if ((inb(0x64) & 1) == 1) return;
        }
        return;
    } else {
        while (timeout--) {
            if ((inb(0x64) & 2) == 0) return;
        }
        return;
    }
}

static void mouse_write(uint8_t data) {
    mouse_wait(1);
    outb(0x64, 0xD4);
    mouse_wait(1);
    outb(0x60, data);
}

static uint8_t mouse_read(void) {
    mouse_wait(0);
    return inb(0x60);
}

void mouse_handle_interrupt(registers_t* regs) {
    (void)regs;  // Unused parameter
    
    switch (mouse_cycle) {
        case 0:
            mouse_byte[0] = mouse_read();
            mouse_cycle++;
            break;
        case 1:
            mouse_byte[1] = mouse_read();
            mouse_cycle++;
            break;
        case 2:
            mouse_byte[2] = mouse_read();
            
            // Update mouse state
            mouse_state.dx = mouse_byte[1];
            mouse_state.dy = -mouse_byte[2];
            mouse_state.x += mouse_state.dx;
            mouse_state.y += mouse_state.dy;
            mouse_state.buttons = mouse_byte[0] & 0x07;
            
            // Clamp mouse position
            if (mouse_state.x < 0) mouse_state.x = 0;
            if (mouse_state.y < 0) mouse_state.y = 0;
            if (mouse_state.x >= SCREEN_WIDTH) mouse_state.x = SCREEN_WIDTH - 1;
            if (mouse_state.y >= SCREEN_HEIGHT) mouse_state.y = SCREEN_HEIGHT - 1;
            
            // Call callback if registered
            if (mouse_callback) {
                mouse_callback(&mouse_state);
            }
            
            mouse_cycle = 0;
            break;
    }
    
    pic_send_eoi(12);  // Send End of Interrupt
}

void mouse_init(void) {
    // Enable PS/2 mouse
    mouse_wait(1);
    outb(0x64, 0xA8);
    
    // Enable interrupts
    mouse_wait(1);
    outb(0x64, 0x20);
    mouse_wait(0);
    uint8_t status = inb(0x60) | 2;
    mouse_wait(1);
    outb(0x64, 0x60);
    mouse_wait(1);
    outb(0x60, status);
    
    // Use default settings
    mouse_write(0xF6);
    mouse_read();  // Acknowledge
    
    // Enable mouse
    mouse_write(0xF4);
    mouse_read();  // Acknowledge
    
    // Register interrupt handler
    register_interrupt_handler(44, mouse_handle_interrupt);
    pic_enable_irq(12);
}

void mouse_set_callback(mouse_callback_t callback) {
    mouse_callback = callback;
}