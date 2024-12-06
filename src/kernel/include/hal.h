#ifndef HAL_H
#define HAL_H

#include <stdint.h>
#include "io.h"

// System constants
#define PAGE_SIZE 4096
#define MAX_DEVICES 32
#define MAX_TIMERS 32

// Error codes
typedef enum {
    HAL_SUCCESS = 0,
    HAL_ERROR_INVALID_PARAMETER,
    HAL_ERROR_NOT_FOUND,
    HAL_ERROR_ALREADY_EXISTS,
    HAL_ERROR_NOT_SUPPORTED,
    HAL_ERROR_RESOURCE_BUSY,
    HAL_ERROR_OUT_OF_MEMORY,
    HAL_ERROR_TIMEOUT,
    HAL_ERROR_IO_ERROR,
    HAL_ERROR_UNKNOWN
} hal_error_t;

// Power states
typedef enum {
    POWER_STATE_ACTIVE,
    POWER_STATE_STANDBY,
    POWER_STATE_SUSPEND,
    POWER_STATE_HIBERNATE,
    POWER_STATE_OFF
} power_state_t;

// Device types
typedef enum {
    DEVICE_TYPE_UNKNOWN,
    DEVICE_TYPE_BLOCK,
    DEVICE_TYPE_CHAR,
    DEVICE_TYPE_NETWORK,
    DEVICE_TYPE_DISPLAY,
    DEVICE_TYPE_INPUT,
    DEVICE_TYPE_SOUND,
    DEVICE_TYPE_TIMER,
    DEVICE_TYPE_OTHER
} device_type_t;

// Device structure
typedef struct device {
    char name[32];
    device_type_t type;
    uint32_t flags;
    void* private_data;
    int (*init)(struct device*);
    int (*cleanup)(struct device*);
    int (*read)(struct device*, void*, uint32_t);
    int (*write)(struct device*, const void*, uint32_t);
    int (*ioctl)(struct device*, uint32_t, void*);
    struct device* next;
} device_t;

// System information structure
typedef struct {
    char cpu_vendor[16];
    uint32_t cpu_family;
    uint32_t cpu_model;
    uint32_t total_memory;
    uint32_t free_memory;
    uint32_t page_size;
    uint32_t num_devices;
} system_info_t;

// Timer callback function type
typedef void (*timer_callback_t)(void* data);

// Interrupt handler function type
typedef void (*interrupt_handler_t)(void);

// CPU related functions
void hal_cpu_disable_interrupts(void);
void hal_cpu_enable_interrupts(void);
void hal_cpu_halt(void);
void hal_cpu_init(void);
void hal_cpu_get_info(char* vendor, uint32_t* family, uint32_t* model);

// I/O port functions
void hal_outb(uint16_t port, uint8_t value);
uint8_t hal_inb(uint16_t port);
void hal_outw(uint16_t port, uint16_t value);
uint16_t hal_inw(uint16_t port);
void hal_outl(uint16_t port, uint32_t value);
uint32_t hal_inl(uint16_t port);

// Memory management functions
void hal_mem_init(void);
void* hal_mem_alloc_page(void);
void hal_mem_free_page(void* page);
uint32_t hal_mem_get_total(void);
uint32_t hal_mem_get_free(void);
void hal_enable_paging(void);
void hal_disable_paging(void);
void hal_load_page_directory(uint32_t* page_directory);
void hal_invalidate_page(uint32_t virtual_address);

// Timer functions
void hal_timer_init(uint32_t frequency);
void hal_timer_wait(uint32_t ticks);
uint32_t hal_get_tick_count(void);
uint32_t hal_timer_register(uint32_t interval_ms, timer_callback_t callback, void* data);
void hal_timer_unregister(uint32_t timer_id);

// DMA functions
void hal_dma_init(void);
void hal_dma_set_channel(uint8_t channel, uint32_t address, uint32_t count, uint8_t mode);
void hal_dma_start_channel(uint8_t channel);
void hal_dma_stop_channel(uint8_t channel);

// Interrupt controller functions
void hal_pic_remap(uint32_t master_offset, uint32_t slave_offset);
void hal_pic_eoi(uint8_t irq);
void hal_pic_mask_irq(uint8_t irq);
void hal_pic_unmask_irq(uint8_t irq);
void hal_interrupt_init(void);
void hal_interrupt_register(uint8_t vector, interrupt_handler_t handler);
void hal_interrupt_unregister(uint8_t vector);

// Power management
int hal_power_set_state(power_state_t state);
power_state_t hal_power_get_state(void);

// Device management
int hal_device_register(device_t* device);
int hal_device_unregister(device_t* device);
device_t* hal_device_find_by_name(const char* name);
device_t* hal_device_find_by_type(device_type_t type);

// System control
void hal_reboot(void);
void hal_shutdown(void);
void hal_enter_usermode(void);

// Error handling
const char* hal_error_string(hal_error_t error);

// System information
void hal_get_system_info(system_info_t* info);

#endif // HAL_H
