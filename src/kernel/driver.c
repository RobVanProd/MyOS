#include "driver.h"
#include "memory.h"
#include <string.h>

// Global driver list
static driver_t* driver_list = NULL;

// Driver type strings
static const char* driver_type_strings[] = {
    "Storage",
    "Network",
    "Display",
    "Input",
    "Sound",
    "Serial",
    "Parallel",
    "USB",
    "PCI",
    "ACPI",
    "Power",
    "Timer",
    "RTC",
    "DMA",
    "Other"
};

// Driver registration
int driver_register(driver_t* driver) {
    if (!driver || !driver->name[0]) {
        return DRIVER_ERROR_INVALID;
    }
    
    // Check if driver already exists
    if (driver_find(driver->name)) {
        return DRIVER_ERROR_EXISTS;
    }
    
    // Initialize driver if not already initialized
    if (!DRIVER_TEST_FLAG(driver, DRIVER_FLAG_INITIALIZED)) {
        if (driver->init) {
            int result = driver->init(driver);
            if (result != DRIVER_SUCCESS) {
                return result;
            }
        }
        DRIVER_SET_FLAG(driver, DRIVER_FLAG_INITIALIZED);
    }
    
    // Add to driver list
    driver->next = driver_list;
    driver_list = driver;
    
    return DRIVER_SUCCESS;
}

// Driver unregistration
int driver_unregister(driver_t* driver) {
    if (!driver) {
        return DRIVER_ERROR_INVALID;
    }
    
    // Find driver in list
    driver_t** pp = &driver_list;
    while (*pp) {
        if (*pp == driver) {
            // Remove from list
            *pp = driver->next;
            
            // Cleanup driver
            if (driver->cleanup) {
                driver->cleanup(driver);
            }
            
            DRIVER_CLEAR_FLAG(driver, DRIVER_FLAG_INITIALIZED);
            return DRIVER_SUCCESS;
        }
        pp = &(*pp)->next;
    }
    
    return DRIVER_ERROR_NOT_FOUND;
}

// Find driver by name
driver_t* driver_find(const char* name) {
    if (!name) return NULL;
    
    driver_t* driver = driver_list;
    while (driver) {
        if (strcmp(driver->name, name) == 0) {
            return driver;
        }
        driver = driver->next;
    }
    
    return NULL;
}

// Find driver by type
driver_t* driver_find_by_type(driver_type_t type) {
    driver_t* driver = driver_list;
    while (driver) {
        if (driver->type == type) {
            return driver;
        }
        driver = driver->next;
    }
    
    return NULL;
}

// Initialize all drivers
int driver_init_all(void) {
    int result = DRIVER_SUCCESS;
    driver_t* driver = driver_list;
    
    while (driver) {
        if (!DRIVER_TEST_FLAG(driver, DRIVER_FLAG_INITIALIZED)) {
            if (driver->init) {
                result = driver->init(driver);
                if (result != DRIVER_SUCCESS) {
                    DRIVER_SET_FLAG(driver, DRIVER_FLAG_ERROR);
                } else {
                    DRIVER_SET_FLAG(driver, DRIVER_FLAG_INITIALIZED);
                }
            }
        }
        driver = driver->next;
    }
    
    return result;
}

// Cleanup all drivers
int driver_cleanup_all(void) {
    int result = DRIVER_SUCCESS;
    driver_t* driver = driver_list;
    
    while (driver) {
        if (DRIVER_TEST_FLAG(driver, DRIVER_FLAG_INITIALIZED)) {
            if (driver->cleanup) {
                result = driver->cleanup(driver);
                if (result != DRIVER_SUCCESS) {
                    DRIVER_SET_FLAG(driver, DRIVER_FLAG_ERROR);
                } else {
                    DRIVER_CLEAR_FLAG(driver, DRIVER_FLAG_INITIALIZED);
                }
            }
        }
        driver = driver->next;
    }
    
    return result;
}

// Dump driver information
void driver_dump_info(driver_t* driver) {
    if (!driver) return;
    
    printf("Driver Information:\n");
    printf("  Name: %s\n", driver->name);
    printf("  Description: %s\n", driver->description);
    printf("  Version: %d.%d\n", driver->version >> 8, driver->version & 0xFF);
    printf("  Type: %s\n", driver_type_string(driver->type));
    printf("  Flags: 0x%08x\n", driver->flags);
    
    printf("  Capabilities:\n");
    printf("    Max Transfer: %d bytes\n", driver->caps.max_transfer_size);
    printf("    Buffer Alignment: %d bytes\n", driver->caps.buffer_alignment);
    printf("    DMA Support: %s\n", driver->caps.dma_support ? "Yes" : "No");
    printf("    Interrupt Support: %s\n", driver->caps.interrupt_support ? "Yes" : "No");
    
    printf("  Statistics:\n");
    printf("    Bytes Read: %llu\n", driver->stats.bytes_read);
    printf("    Bytes Written: %llu\n", driver->stats.bytes_written);
    printf("    I/O Errors: %d\n", driver->stats.io_errors);
    printf("    Interrupts: %d\n", driver->stats.interrupts);
    printf("    DMA Transfers: %d\n", driver->stats.dma_transfers);
    printf("    Uptime: %llu seconds\n", driver->stats.uptime);
    
    printf("  Configuration:\n");
    printf("    I/O Base: 0x%08x\n", driver->config.io_base);
    printf("    I/O Size: %d bytes\n", driver->config.io_size);
    printf("    Memory Base: 0x%08x\n", driver->config.mem_base);
    printf("    Memory Size: %d bytes\n", driver->config.mem_size);
    printf("    IRQ: %d\n", driver->config.irq);
    printf("    DMA Channel: %d\n", driver->config.dma_channel);
}

// Get driver type string
const char* driver_type_string(driver_type_t type) {
    if (type >= 0 && type < sizeof(driver_type_strings)/sizeof(char*)) {
        return driver_type_strings[type];
    }
    return "Unknown";
}

// Get driver error string
const char* driver_error_string(int error) {
    switch (error) {
        case DRIVER_SUCCESS:
            return "Success";
        case DRIVER_ERROR_INIT:
            return "Initialization error";
        case DRIVER_ERROR_BUSY:
            return "Device busy";
        case DRIVER_ERROR_TIMEOUT:
            return "Operation timeout";
        case DRIVER_ERROR_IO:
            return "I/O error";
        case DRIVER_ERROR_INVALID:
            return "Invalid parameter";
        case DRIVER_ERROR_MEMORY:
            return "Memory error";
        case DRIVER_ERROR_NOT_FOUND:
            return "Driver not found";
        case DRIVER_ERROR_EXISTS:
            return "Driver already exists";
        case DRIVER_ERROR_NOT_READY:
            return "Device not ready";
        case DRIVER_ERROR_REMOVED:
            return "Device removed";
        default:
            return "Unknown error";
    }
} 