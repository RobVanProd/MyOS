#ifndef HAL_H
#define HAL_H

#include <stdint.h>
#include <pci.h>

// Hardware Abstraction Layer Interface

// CPU Management
void hal_cpu_init(void);
void hal_cpu_enable_interrupts(void);
void hal_cpu_disable_interrupts(void);
uint32_t hal_cpu_get_vendor(void);
void hal_cpu_get_info(char* vendor, uint32_t* family, uint32_t* model);

// Memory Management Constants
#define PAGE_SIZE 4096

// Memory Management
void hal_mem_init(void);
void* hal_mem_alloc_page(void);
void hal_mem_free_page(void* page);
uint32_t hal_mem_get_total(void);
uint32_t hal_mem_get_free(void);

// IDT Management
void idt_set_gate(uint8_t vector, uint32_t handler, uint16_t selector, uint8_t flags);

// Memory Management Functions
void* kmalloc_aligned(size_t size);

// Interrupt Management
typedef void (*interrupt_handler_t)(void);
void hal_interrupt_init(void);
void hal_interrupt_register(uint8_t vector, interrupt_handler_t handler);
void hal_interrupt_unregister(uint8_t vector);
void hal_interrupt_enable(uint8_t irq);
void hal_interrupt_disable(uint8_t irq);

// DMA Management
typedef struct {
    uint32_t source;
    uint32_t destination;
    uint32_t size;
    uint8_t channel;
} dma_request_t;

void hal_dma_init(void);
int hal_dma_request(dma_request_t* request);
void hal_dma_cancel(uint8_t channel);
uint8_t hal_dma_status(uint8_t channel);

// Timer Management
typedef void (*timer_callback_t)(void* data);
void hal_timer_init(void);
uint32_t hal_timer_register(uint32_t interval_ms, timer_callback_t callback, void* data);
void hal_timer_unregister(uint32_t timer_id);
uint64_t hal_timer_get_ticks(void);

// Power Management
typedef enum {
    POWER_STATE_ACTIVE,
    POWER_STATE_STANDBY,
    POWER_STATE_SUSPEND,
    POWER_STATE_HIBERNATE
} power_state_t;

void hal_power_init(void);
int hal_power_set_state(power_state_t state);
power_state_t hal_power_get_state(void);
uint32_t hal_power_get_battery_level(void);

// PCI Management
void hal_pci_init(void);
int hal_pci_find_device(uint16_t vendor, uint16_t device, pci_device_t* dev);
void hal_pci_enable_bus_mastering(pci_device_t* dev);
uint32_t hal_pci_read_config(pci_device_t* dev, uint8_t offset);
void hal_pci_write_config(pci_device_t* dev, uint8_t offset, uint32_t value);

// ACPI Management
typedef struct {
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;
} acpi_rsdp_t;

void hal_acpi_init(void);
acpi_rsdp_t* hal_acpi_find_rsdp(void);
void* hal_acpi_find_table(const char* signature);

// Device Management Base Interface
typedef enum {
    DEVICE_TYPE_UNKNOWN,
    DEVICE_TYPE_STORAGE,
    DEVICE_TYPE_NETWORK,
    DEVICE_TYPE_DISPLAY,
    DEVICE_TYPE_INPUT,
    DEVICE_TYPE_SOUND,
    DEVICE_TYPE_SERIAL,
    DEVICE_TYPE_PARALLEL
} device_type_t;

typedef struct {
    char name[32];
    device_type_t type;
    uint16_t vendor_id;
    uint16_t device_id;
    void* driver_data;
    int (*init)(void* driver_data);
    int (*cleanup)(void* driver_data);
    int (*read)(void* driver_data, void* buffer, size_t size);
    int (*write)(void* driver_data, const void* buffer, size_t size);
    int (*ioctl)(void* driver_data, uint32_t cmd, void* arg);
} device_t;

void hal_device_init(void);
int hal_device_register(device_t* device);
int hal_device_unregister(device_t* device);
device_t* hal_device_find_by_name(const char* name);
device_t* hal_device_find_by_type(device_type_t type);

// Error Handling
typedef enum {
    HAL_SUCCESS = 0,
    HAL_ERROR_INVALID_PARAMETER = -1,
    HAL_ERROR_NOT_INITIALIZED = -2,
    HAL_ERROR_ALREADY_EXISTS = -3,
    HAL_ERROR_NOT_FOUND = -4,
    HAL_ERROR_NO_MEMORY = -5,
    HAL_ERROR_NOT_SUPPORTED = -6,
    HAL_ERROR_TIMEOUT = -7,
    HAL_ERROR_BUSY = -8,
    HAL_ERROR_IO = -9
} hal_error_t;

const char* hal_error_string(hal_error_t error);

// System Information
typedef struct {
    char cpu_vendor[16];
    uint32_t cpu_family;
    uint32_t cpu_model;
    uint32_t total_memory;
    uint32_t free_memory;
    uint32_t page_size;
    uint32_t num_cores;
    char os_version[32];
    power_state_t power_state;
    uint32_t uptime;
} system_info_t;

void hal_get_system_info(system_info_t* info);

#endif