#ifndef DRIVER_H
#define DRIVER_H

#include "hal.h"
#include <stdint.h>
#include <stddef.h>

// Driver interface version
#define DRIVER_VERSION 0x0100

// Driver flags
#define DRIVER_FLAG_INITIALIZED  0x01
#define DRIVER_FLAG_ENABLED     0x02
#define DRIVER_FLAG_BUSY        0x04
#define DRIVER_FLAG_ERROR       0x08
#define DRIVER_FLAG_REMOVABLE   0x10

// Driver types
typedef enum {
    DRIVER_TYPE_STORAGE,
    DRIVER_TYPE_NETWORK,
    DRIVER_TYPE_DISPLAY,
    DRIVER_TYPE_INPUT,
    DRIVER_TYPE_SOUND,
    DRIVER_TYPE_SERIAL,
    DRIVER_TYPE_PARALLEL,
    DRIVER_TYPE_USB,
    DRIVER_TYPE_PCI,
    DRIVER_TYPE_ACPI,
    DRIVER_TYPE_POWER,
    DRIVER_TYPE_TIMER,
    DRIVER_TYPE_RTC,
    DRIVER_TYPE_DMA,
    DRIVER_TYPE_OTHER
} driver_type_t;

// Driver capabilities
typedef struct {
    uint32_t flags;
    uint32_t max_transfer_size;
    uint32_t buffer_alignment;
    uint32_t dma_support;
    uint32_t interrupt_support;
    uint32_t power_states;
} driver_caps_t;

// Driver statistics
typedef struct {
    uint64_t bytes_read;
    uint64_t bytes_written;
    uint32_t io_errors;
    uint32_t interrupts;
    uint32_t dma_transfers;
    uint32_t buffer_overflows;
    uint64_t uptime;
} driver_stats_t;

// Driver configuration
typedef struct {
    uint32_t io_base;
    uint32_t io_size;
    uint32_t mem_base;
    uint32_t mem_size;
    uint32_t irq;
    uint32_t dma_channel;
    uint32_t flags;
    void* private_data;
} driver_config_t;

// Base driver structure
typedef struct driver {
    char name[32];
    char description[64];
    uint16_t version;
    driver_type_t type;
    uint32_t flags;
    driver_caps_t caps;
    driver_stats_t stats;
    driver_config_t config;
    
    // Driver operations
    int (*init)(struct driver* drv);
    int (*cleanup)(struct driver* drv);
    int (*start)(struct driver* drv);
    int (*stop)(struct driver* drv);
    int (*reset)(struct driver* drv);
    
    // I/O operations
    int (*read)(struct driver* drv, void* buffer, size_t size, uint32_t offset);
    int (*write)(struct driver* drv, const void* buffer, size_t size, uint32_t offset);
    int (*ioctl)(struct driver* drv, uint32_t cmd, void* arg);
    
    // Interrupt handling
    int (*interrupt_handler)(struct driver* drv);
    
    // Power management
    int (*suspend)(struct driver* drv);
    int (*resume)(struct driver* drv);
    
    // DMA operations
    int (*dma_setup)(struct driver* drv, void* buffer, size_t size, int direction);
    int (*dma_start)(struct driver* drv);
    int (*dma_stop)(struct driver* drv);
    int (*dma_status)(struct driver* drv);
    
    // Device-specific operations
    void* device_ops;
    
    // Driver chain
    struct driver* next;
} driver_t;

// Driver registration/management functions
int driver_register(driver_t* driver);
int driver_unregister(driver_t* driver);
driver_t* driver_find(const char* name);
driver_t* driver_find_by_type(driver_type_t type);

// Driver initialization functions
void driver_init(void);
void driver_register_storage(void);
void driver_register_network(void);

// Driver utility functions
int driver_init_all(void);
int driver_cleanup_all(void);
void driver_dump_info(driver_t* driver);
const char* driver_type_string(driver_type_t type);
const char* driver_error_string(int error);

// Common IOCTL commands
#define IOCTL_GET_CAPABILITIES    0x0001
#define IOCTL_GET_STATS          0x0002
#define IOCTL_RESET_STATS        0x0003
#define IOCTL_SET_CONFIG         0x0004
#define IOCTL_GET_CONFIG         0x0005
#define IOCTL_ENABLE_IRQ         0x0006
#define IOCTL_DISABLE_IRQ        0x0007
#define IOCTL_ENABLE_DMA         0x0008
#define IOCTL_DISABLE_DMA        0x0009
#define IOCTL_SELF_TEST          0x000A
#define IOCTL_GET_STATUS         0x000B
#define IOCTL_SET_POWER          0x000C

// DMA directions
#define DMA_DIRECTION_READ       0
#define DMA_DIRECTION_WRITE      1
#define DMA_DIRECTION_BIDIRECT   2

// Driver error codes
#define DRIVER_SUCCESS           0
#define DRIVER_ERROR_INIT       -1
#define DRIVER_ERROR_BUSY       -2
#define DRIVER_ERROR_TIMEOUT    -3
#define DRIVER_ERROR_IO         -4
#define DRIVER_ERROR_INVALID    -5
#define DRIVER_ERROR_MEMORY     -6
#define DRIVER_ERROR_NOT_FOUND  -7
#define DRIVER_ERROR_EXISTS     -8
#define DRIVER_ERROR_NOT_READY  -9
#define DRIVER_ERROR_REMOVED    -10

// Helper macros
#define DRIVER_INIT(drv, n, t) do { \
    memset(drv, 0, sizeof(driver_t)); \
    strncpy(drv->name, n, sizeof(drv->name)-1); \
    drv->type = t; \
    drv->version = DRIVER_VERSION; \
} while(0)

#define DRIVER_SET_FLAG(drv, f)   ((drv)->flags |= (f))
#define DRIVER_CLEAR_FLAG(drv, f) ((drv)->flags &= ~(f))
#define DRIVER_TEST_FLAG(drv, f)  ((drv)->flags & (f))

#endif