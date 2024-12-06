#ifndef DRIVER_H
#define DRIVER_H

#include <stdint.h>
#include <stddef.h>

// Driver status codes
#define DRIVER_SUCCESS          0
#define DRIVER_ERROR          -1
#define DRIVER_ERROR_INIT     -2
#define DRIVER_ERROR_BUSY     -3
#define DRIVER_ERROR_TIMEOUT  -4
#define DRIVER_ERROR_IO       -5
#define DRIVER_ERROR_INVALID  -6
#define DRIVER_ERROR_MEMORY   -7
#define DRIVER_ERROR_NOT_FOUND -8
#define DRIVER_ERROR_EXISTS   -9
#define DRIVER_ERROR_NOT_READY -10
#define DRIVER_ERROR_REMOVED  -11

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

// Driver flags
#define DRIVER_FLAG_INITIALIZED 0x01
#define DRIVER_FLAG_ENABLED    0x02
#define DRIVER_FLAG_BUSY       0x04
#define DRIVER_FLAG_ERROR      0x08

// Driver capabilities
typedef struct {
    uint32_t max_transfer_size;
    uint32_t buffer_alignment;
    uint8_t dma_support;
    uint8_t interrupt_support;
} driver_caps_t;

// Driver statistics
typedef struct {
    uint64_t bytes_read;
    uint64_t bytes_written;
    uint32_t io_errors;
    uint32_t interrupts;
    uint32_t dma_transfers;
    uint32_t uptime;
} driver_stats_t;

// Driver configuration
typedef struct {
    uint32_t io_base;
    uint32_t io_size;
    uint32_t mem_base;
    uint32_t mem_size;
    uint8_t irq;
    uint8_t dma_channel;
} driver_config_t;

// Driver structure
typedef struct driver {
    char name[32];              // Driver name
    char description[64];       // Driver description
    uint32_t version;          // Driver version
    driver_type_t type;        // Driver type
    uint32_t flags;            // Driver flags
    driver_caps_t caps;        // Driver capabilities
    driver_stats_t stats;      // Driver statistics
    driver_config_t config;    // Driver configuration
    
    // Driver operations
    int (*init)(struct driver*);         // Initialize the driver
    int (*cleanup)(struct driver*);      // Cleanup the driver
    int (*reset)(struct driver*);        // Reset the driver
    int (*suspend)(struct driver*);      // Suspend the driver
    int (*resume)(struct driver*);       // Resume the driver
    
    // I/O operations
    size_t (*read)(struct driver*, void* buffer, size_t size);
    size_t (*write)(struct driver*, const void* buffer, size_t size);
    int (*ioctl)(struct driver*, uint32_t cmd, void* arg);
    
    // Device-specific data
    void* device_data;
    
    // Linked list
    struct driver* next;
} driver_t;

// Function declarations
int driver_register(driver_t* driver);
int driver_unregister(driver_t* driver);
driver_t* driver_find(const char* name);
driver_t* driver_find_by_type(driver_type_t type);
int driver_init_all(void);
int driver_cleanup_all(void);
void driver_dump_info(driver_t* driver);
const char* driver_type_string(driver_type_t type);
const char* driver_error_string(int error);

// Driver list
driver_t* driver_list;

// Helper macros
#define DRIVER_TEST_FLAG(d, f)  ((d)->flags & (f))
#define DRIVER_SET_FLAG(d, f)   ((d)->flags |= (f))
#define DRIVER_CLEAR_FLAG(d, f) ((d)->flags &= ~(f))

#endif /* DRIVER_H */
