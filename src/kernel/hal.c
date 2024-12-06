#include "hal.h"
#include "memory.h"
#include "string.h"
#include "kheap.h"
#include "isr.h"
#include "idt.h"

// Forward declaration of timer interrupt handler
static void timer_interrupt_handler(registers_t* regs);

// Global variables
static system_info_t system_info;
static power_state_t current_power_state = POWER_STATE_ACTIVE;
static device_t* device_list = NULL;
static timer_callback_t timer_callbacks[MAX_TIMERS] = {0};
static void* timer_callback_data[MAX_TIMERS] = {0};
static uint32_t next_timer_id = 0;

// CPU initialization
void hal_cpu_init(void) {
    // Get CPU vendor string
    uint32_t eax, ebx, ecx, edx;
    char vendor[13] = {0};
    
    __asm__ volatile("cpuid"
                     : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                     : "a"(0));
                     
    // Copy vendor string safely
    memcpy(vendor, &ebx, 4);
    memcpy(vendor + 4, &edx, 4);
    memcpy(vendor + 8, &ecx, 4);
    vendor[12] = '\0';

    // Store CPU info
    strncpy(system_info.cpu_vendor, vendor, sizeof(system_info.cpu_vendor) - 1);
    system_info.cpu_vendor[sizeof(system_info.cpu_vendor) - 1] = '\0';
    
    // Get CPU family and model
    __asm__ volatile("cpuid"
                     : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                     : "a"(1));
                     
    system_info.cpu_family = (eax >> 8) & 0xF;
    system_info.cpu_model = (eax >> 4) & 0xF;
}

// Get CPU information
void hal_cpu_get_info(char* vendor, uint32_t* family, uint32_t* model) {
    if (vendor) strncpy(vendor, system_info.cpu_vendor, 16);
    if (family) *family = system_info.cpu_family;
    if (model) *model = system_info.cpu_model;
}

// Memory initialization
void hal_mem_init(void) {
    system_info.total_memory = get_total_memory();
    system_info.free_memory = get_free_memory();
    system_info.page_size = PAGE_SIZE;
}

// Memory allocation
void* hal_mem_alloc_page(void) {
    return kmalloc_aligned(PAGE_SIZE);
}

void hal_mem_free_page(void* page) {
    kfree(page);
}

uint32_t hal_mem_get_total(void) {
    return system_info.total_memory;
}

uint32_t hal_mem_get_free(void) {
    return system_info.free_memory;
}

// Interrupt management
void hal_interrupt_init(void) {
    // Initialize PIC
    pic_init();
    
    // Initialize IDT
    idt_init();
    
    // Initialize timer
    hal_timer_init(100);  // 100 Hz timer
    
    // Enable interrupts
    asm volatile("sti");
}

void hal_interrupt_register(uint8_t vector, interrupt_handler_t handler) {
    if (!handler) return;
    idt_set_gate(vector, (uint32_t)handler, 0x08, 0x8E);
}

void hal_interrupt_unregister(uint8_t vector) {
    idt_set_gate(vector, 0, 0, 0);
}

void hal_pic_eoi(uint8_t irq) {
    if (irq >= 8) {
        outb(0xA0, 0x20);  // Send EOI to slave PIC
    }
    outb(0x20, 0x20);  // Send EOI to master PIC
}

// Timer management
void hal_timer_init(uint32_t frequency) {
    uint32_t divisor = 1193180 / frequency;
    
    // Send command byte
    outb(0x43, 0x36);
    
    // Send frequency divisor
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
    
    // Register our timer callback
    register_interrupt_handler(32, timer_interrupt_handler);
}

uint32_t hal_timer_register(uint32_t interval_ms, timer_callback_t callback, void* data) {
    (void)interval_ms; // Mark as intentionally unused
    
    if (!callback || next_timer_id >= MAX_TIMERS) {
        return 0;
    }
    
    uint32_t id = next_timer_id++;
    timer_callbacks[id] = callback;
    timer_callback_data[id] = data;
    
    return id;
}

void hal_timer_unregister(uint32_t timer_id) {
    if (timer_id > 0 && timer_id <= MAX_TIMERS) {
        uint32_t index = timer_id - 1;
        timer_callbacks[index] = NULL;
        timer_callback_data[index] = NULL;
    }
}

static void timer_interrupt_handler(registers_t* regs) {
    (void)regs; // Unused parameter
    
    static uint32_t ticks = 0;
    ticks++;
    
    // Call registered callbacks
    for (uint32_t i = 0; i < next_timer_id; i++) {
        if (timer_callbacks[i]) {
            timer_callbacks[i](timer_callback_data[i]);
        }
    }
    
    hal_pic_eoi(0);
}

// Power management
int hal_power_set_state(power_state_t state) {
    switch (state) {
        case POWER_STATE_ACTIVE:
            // Resume normal operation
            current_power_state = POWER_STATE_ACTIVE;
            return 0;
            
        case POWER_STATE_STANDBY:
            // Enter standby mode
            current_power_state = POWER_STATE_STANDBY;
            return 0;
            
        case POWER_STATE_SUSPEND:
            // Enter suspend mode
            current_power_state = POWER_STATE_SUSPEND;
            return 0;
            
        case POWER_STATE_HIBERNATE:
            // Enter hibernate mode
            current_power_state = POWER_STATE_HIBERNATE;
            return 0;
            
        case POWER_STATE_OFF:
            // Shutdown system
            current_power_state = POWER_STATE_OFF;
            hal_shutdown();
            return 0;
            
        default:
            return -1;
    }
}

power_state_t hal_power_get_state(void) {
    return current_power_state;
}

// System shutdown
void hal_shutdown(void) {
    // Disable interrupts
    asm volatile("cli");
    
    // Try ACPI shutdown first
    if (acpi_shutdown() == HAL_SUCCESS) {
        return;
    }
    
    // If ACPI shutdown fails, try APM
    outw(0xB004, 0x0 | (2 << 10));
    
    // If APM fails, try keyboard controller
    outb(0x64, 0xFE);
    
    // If all else fails, halt the CPU
    for (;;) {
        asm volatile("hlt");
    }
}

// Device management
int hal_device_register(device_t* device) {
    if (!device) return -1;
    
    // Initialize device
    if (device->init && device->init(device) != 0) {
        return -1;
    }
    
    // Add to device list
    device->next = device_list;
    device_list = device;
    system_info.num_devices++;
    
    return 0;
}

int hal_device_unregister(device_t* device) {
    if (!device) return -1;
    
    // Remove from device list
    device_t** pp = &device_list;
    while (*pp) {
        if (*pp == device) {
            *pp = device->next;
            system_info.num_devices--;
            
            // Cleanup device
            if (device->cleanup) {
                device->cleanup(device);
            }
            
            return 0;
        }
        pp = &(*pp)->next;
    }
    
    return -1;
}

device_t* hal_device_find_by_name(const char* name) {
    if (!name) return NULL;
    
    device_t* dev = device_list;
    while (dev) {
        if (strcmp(dev->name, name) == 0) {
            return dev;
        }
        dev = dev->next;
    }
    
    return NULL;
}

device_t* hal_device_find_by_type(device_type_t type) {
    device_t* dev = device_list;
    while (dev) {
        if (dev->type == type) {
            return dev;
        }
        dev = dev->next;
    }
    
    return NULL;
}

// Error handling
const char* hal_error_string(hal_error_t error) {
    switch (error) {
        case HAL_SUCCESS:
            return "Success";
        case HAL_ERROR_INVALID_PARAMETER:
            return "Invalid parameter";
        case HAL_ERROR_NOT_FOUND:
            return "Not found";
        case HAL_ERROR_ALREADY_EXISTS:
            return "Already exists";
        case HAL_ERROR_NOT_SUPPORTED:
            return "Not supported";
        case HAL_ERROR_RESOURCE_BUSY:
            return "Resource busy";
        case HAL_ERROR_OUT_OF_MEMORY:
            return "Out of memory";
        case HAL_ERROR_TIMEOUT:
            return "Timeout";
        case HAL_ERROR_IO_ERROR:
            return "I/O error";
        default:
            return "Unknown error";
    }
}

// System information
void hal_get_system_info(system_info_t* info) {
    if (info) {
        memcpy(info, &system_info, sizeof(system_info_t));
    }
}