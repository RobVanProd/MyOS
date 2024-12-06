#ifndef DRIVER_H
#define DRIVER_H

#include <stdint.h>
#include <stddef.h>

// Driver error codes
#define DRIVER_SUCCESS          0
#define DRIVER_ERROR           -1
#define DRIVER_ERROR_INIT      -2
#define DRIVER_ERROR_BUSY      -3
#define DRIVER_ERROR_TIMEOUT   -4
#define DRIVER_ERROR_IO        -5
#define DRIVER_ERROR_INVALID   -6
#define DRIVER_ERROR_MEMORY    -7
#define DRIVER_ERROR_NOT_FOUND -8
#define DRIVER_ERROR_EXISTS    -9
#define DRIVER_ERROR_NOT_READY -10
#define DRIVER_ERROR_REMOVED   -11
#define DRIVER_NOT_FOUND       -12
#define DRIVER_ALREADY_EXISTS  -13
#define DRIVER_INVALID_PARAM   -14
#define DRIVER_NOT_SUPPORTED   -15
#define DRIVER_IO_ERROR        -16
#define DRIVER_NO_MEMORY       -17
#define DRIVER_BUSY            -18
#define DRIVER_TIMEOUT         -19

// Driver flags
#define DRIVER_FLAG_INITIALIZED 0x00000001
#define DRIVER_FLAG_ERROR      0x00000002
#define DRIVER_FLAG_BUSY       0x00000004
#define DRIVER_FLAG_SUSPENDED  0x00000008
#define DRIVER_FLAG_REMOVED    0x00000010

// Driver version
#define DRIVER_VERSION_MAJOR   1
#define DRIVER_VERSION_MINOR   0
#define DRIVER_VERSION_PATCH   0
#define DRIVER_VERSION         ((DRIVER_VERSION_MAJOR << 16) | (DRIVER_VERSION_MINOR << 8) | DRIVER_VERSION_PATCH)

// Driver types
typedef enum {
    DRIVER_TYPE_UNKNOWN = 0,
    DRIVER_TYPE_CHAR,
    DRIVER_TYPE_BLOCK,
    DRIVER_TYPE_NETWORK,
    DRIVER_TYPE_KEYBOARD,
    DRIVER_TYPE_MOUSE,
    DRIVER_TYPE_DISPLAY,
    DRIVER_TYPE_SOUND,
    DRIVER_TYPE_STORAGE,
    DRIVER_TYPE_USB,
    DRIVER_TYPE_PCI,
    DRIVER_TYPE_ACPI,
    DRIVER_TYPE_RTC,
    DRIVER_TYPE_TIMER,
    DRIVER_TYPE_DMA,
    DRIVER_TYPE_MAX
} driver_type_t;

// Driver capabilities
typedef struct {
    uint32_t flags;
    uint32_t max_transfer_size;
    uint32_t buffer_alignment;
    uint32_t dma_support;
    uint32_t interrupt_support;
    uint32_t features;
} driver_capabilities_t;

// Driver statistics
typedef struct {
    uint64_t bytes_read;
    uint64_t bytes_written;
    uint64_t io_errors;
    uint64_t interrupts;
    uint64_t dma_transfers;
    uint64_t uptime;
} driver_statistics_t;

// Driver configuration
typedef struct {
    uint32_t flags;
    uint32_t timeout;
    uint32_t buffer_size;
    uint32_t dma_channel;
    uint32_t irq;
    uint32_t io_base;
    uint32_t io_size;  // Add io_size
    uint32_t mem_base;
    uint32_t mem_size; // Add mem_size
} driver_config_t;

// Forward declaration of driver structure
typedef struct driver driver_t;

// Driver function types
typedef int (*driver_init_fn)(driver_t* driver);
typedef int (*driver_cleanup_fn)(driver_t* driver);
typedef ssize_t (*driver_read_fn)(driver_t* driver, void* buffer, size_t size);
typedef ssize_t (*driver_write_fn)(driver_t* driver, const void* buffer, size_t size);
typedef int (*driver_ioctl_fn)(driver_t* driver, int request, void* arg);
typedef int (*driver_flush_fn)(driver_t* driver);
typedef int (*driver_reset_fn)(driver_t* driver);

// Driver structure
struct driver {
    char name[32];
    char description[128];
    uint32_t version;
    uint32_t flags;
    driver_type_t type;
    driver_capabilities_t caps;
    driver_statistics_t stats;
    driver_config_t config;
    driver_init_fn init;
    driver_cleanup_fn cleanup;
    driver_read_fn read;
    driver_write_fn write;
    driver_ioctl_fn ioctl;
    driver_flush_fn flush;
    driver_reset_fn reset;
    struct driver* next;
};

// Flag test macros
#define DRIVER_TEST_FLAG(d, f)    ((d)->flags & (f))
#define DRIVER_SET_FLAG(d, f)     ((d)->flags |= (f))
#define DRIVER_CLEAR_FLAG(d, f)   ((d)->flags &= ~(f))

// Function declarations
int driver_register(driver_t* driver);
int driver_unregister(driver_t* driver);
driver_t* driver_find(const char* name);
driver_t* driver_find_by_type(driver_type_t type);
int driver_init_all(void);  // Changed return type to int
int driver_cleanup_all(void);  // Changed return type to int
void driver_dump_info(driver_t* driver);
const char* driver_error_string(int error);

// Global variables
extern driver_t* driver_list;

#endif // DRIVER_H
