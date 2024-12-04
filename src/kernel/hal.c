#include "hal.h"
#include "io.h"
#include "memory.h"
#include "pic.h"
#include <string.h>

// Static variables for HAL state
static system_info_t system_info;
static power_state_t current_power_state = POWER_STATE_ACTIVE;
static uint64_t system_ticks = 0;
static timer_callback_t timer_callbacks[32] = {0};
static void* timer_callback_data[32] = {0};
static uint32_t next_timer_id = 0;

// CPU Management Implementation
void hal_cpu_init(void) {
    // Read CPU vendor and info
    char vendor[13] = {0};
    uint32_t eax, ebx, ecx, edx;
    
    // CPUID instruction to get vendor
    asm volatile("cpuid"
                : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                : "a"(0));
                
    // Store vendor string
    *((uint32_t*)vendor) = ebx;
    *((uint32_t*)(vendor + 4)) = edx;
    *((uint32_t*)(vendor + 8)) = ecx;
    
    strncpy(system_info.cpu_vendor, vendor, 12);
    
    // Get CPU family/model info
    asm volatile("cpuid"
                : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                : "a"(1));
                
    system_info.cpu_family = (eax >> 8) & 0xF;
    system_info.cpu_model = (eax >> 4) & 0xF;
}

void hal_cpu_enable_interrupts(void) {
    asm volatile("sti");
}

void hal_cpu_disable_interrupts(void) {
    asm volatile("cli");
}

uint32_t hal_cpu_get_vendor(void) {
    uint32_t vendor;
    asm volatile("cpuid"
                : "=b"(vendor)
                : "a"(0)
                : "ecx", "edx");
    return vendor;
}

void hal_cpu_get_info(char* vendor, uint32_t* family, uint32_t* model) {
    if (vendor) strncpy(vendor, system_info.cpu_vendor, 16);
    if (family) *family = system_info.cpu_family;
    if (model) *model = system_info.cpu_model;
}

// Memory Management Implementation
void hal_mem_init(void) {
    // Initialize memory management
    memory_init();
    
    // Get memory information
    system_info.total_memory = get_total_memory();
    system_info.free_memory = get_free_memory();
    system_info.page_size = PAGE_SIZE;
}

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
    return get_free_memory();
}

// Interrupt Management Implementation
void hal_interrupt_init(void) {
    pic_init();
}

void hal_interrupt_register(uint8_t vector, interrupt_handler_t handler) {
    // Register interrupt handler
    idt_set_gate(vector, (uint32_t)handler, 0x08, 0x8E);
}

void hal_interrupt_unregister(uint8_t vector) {
    // Clear interrupt handler
    idt_set_gate(vector, 0, 0, 0);
}

void hal_interrupt_enable(uint8_t irq) {
    pic_enable_irq(irq);
}

void hal_interrupt_disable(uint8_t irq) {
    pic_disable_irq(irq);
}

// Timer Management Implementation
void hal_timer_init(void) {
    // Initialize system timer (PIT)
    uint32_t divisor = 1193180 / 1000; // 1ms intervals
    
    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);
    
    // Enable timer interrupt
    hal_interrupt_enable(0);
}

uint32_t hal_timer_register(uint32_t interval_ms, timer_callback_t callback, void* data) {
    if (!callback) return 0;
    
    // Find free timer slot
    for (int i = 0; i < 32; i++) {
        if (!timer_callbacks[i]) {
            timer_callbacks[i] = callback;
            timer_callback_data[i] = data;
            return ++next_timer_id;
        }
    }
    return 0;
}

void hal_timer_unregister(uint32_t timer_id) {
    for (int i = 0; i < 32; i++) {
        if (timer_callbacks[i]) {
            timer_callbacks[i] = NULL;
            timer_callback_data[i] = NULL;
            break;
        }
    }
}

uint64_t hal_timer_get_ticks(void) {
    return system_ticks;
}

// Timer interrupt handler
void timer_interrupt_handler(void) {
    system_ticks++;
    
    // Call registered callbacks
    for (int i = 0; i < 32; i++) {
        if (timer_callbacks[i]) {
            timer_callbacks[i](timer_callback_data[i]);
        }
    }
    
    pic_send_eoi(0);
}

// Power Management Implementation
void hal_power_init(void) {
    current_power_state = POWER_STATE_ACTIVE;
}

int hal_power_set_state(power_state_t state) {
    // Basic power state switching
    switch (state) {
        case POWER_STATE_ACTIVE:
            // Wake up system
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
            
        default:
            return HAL_ERROR_INVALID_PARAMETER;
    }
}

power_state_t hal_power_get_state(void) {
    return current_power_state;
}

uint32_t hal_power_get_battery_level(void) {
    // Placeholder - implement actual battery check
    return 100;
}

// Device Management Implementation
void hal_device_init(void) {
    // Initialize device management system
}

int hal_device_register(device_t* device) {
    if (!device || !device->name[0]) {
        return HAL_ERROR_INVALID_PARAMETER;
    }
    
    // Register device in device list
    // (Implement device list management)
    
    return HAL_SUCCESS;
}

int hal_device_unregister(device_t* device) {
    if (!device) {
        return HAL_ERROR_INVALID_PARAMETER;
    }
    
    // Remove device from device list
    // (Implement device list management)
    
    return HAL_SUCCESS;
}

device_t* hal_device_find_by_name(const char* name) {
    if (!name) return NULL;
    
    // Search device list by name
    // (Implement device list search)
    
    return NULL;
}

device_t* hal_device_find_by_type(device_type_t type) {
    // Search device list by type
    // (Implement device list search)
    
    return NULL;
}

// Error Handling Implementation
const char* hal_error_string(hal_error_t error) {
    switch (error) {
        case HAL_SUCCESS:
            return "Success";
        case HAL_ERROR_INVALID_PARAMETER:
            return "Invalid parameter";
        case HAL_ERROR_NOT_INITIALIZED:
            return "Not initialized";
        case HAL_ERROR_ALREADY_EXISTS:
            return "Already exists";
        case HAL_ERROR_NOT_FOUND:
            return "Not found";
        case HAL_ERROR_NO_MEMORY:
            return "No memory";
        case HAL_ERROR_NOT_SUPPORTED:
            return "Not supported";
        case HAL_ERROR_TIMEOUT:
            return "Timeout";
        case HAL_ERROR_BUSY:
            return "Busy";
        case HAL_ERROR_IO:
            return "I/O error";
        default:
            return "Unknown error";
    }
}

// System Information Implementation
void hal_get_system_info(system_info_t* info) {
    if (!info) return;
    
    // Copy current system info
    memcpy(info, &system_info, sizeof(system_info_t));
    
    // Update dynamic information
    info->free_memory = get_free_memory();
    info->power_state = current_power_state;
    info->uptime = system_ticks / 1000; // Convert to seconds
} 