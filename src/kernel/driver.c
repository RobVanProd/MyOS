#include "driver.h"
#include "memory.h"
#include "terminal.h"
#include <string.h>
#include <stdbool.h>

// Global driver list
driver_t* driver_list = NULL;

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
    driver_t* prev = NULL;
    driver_t* curr = driver_list;
    
    while (curr) {
        if (curr == driver) {
            // Remove from list
            if (prev) {
                prev->next = curr->next;
            } else {
                driver_list = curr->next;
            }
            
            // Cleanup driver
            if (driver->cleanup) {
                driver->cleanup(driver);
            }
            
            DRIVER_CLEAR_FLAG(driver, DRIVER_FLAG_INITIALIZED);
            return DRIVER_SUCCESS;
        }
        
        prev = curr;
        curr = curr->next;
    }
    
    return DRIVER_ERROR_NOT_FOUND;
}

// Find driver by name
driver_t* driver_find(const char* name) {
    if (!name) {
        return NULL;
    }
    
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
                    return result;
                }
            }
            DRIVER_SET_FLAG(driver, DRIVER_FLAG_INITIALIZED);
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
                    return result;
                }
            }
            DRIVER_CLEAR_FLAG(driver, DRIVER_FLAG_INITIALIZED);
        }
        driver = driver->next;
    }
    
    return result;
}

// Convert integer to string
static void int_to_string(uint32_t value, char* str) {
    if (value == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }
    
    int i = 0;
    while (value > 0) {
        str[i++] = '0' + (value % 10);
        value /= 10;
    }
    str[i] = '\0';
    
    // Reverse the string
    for (int j = 0; j < i / 2; j++) {
        char temp = str[j];
        str[j] = str[i - 1 - j];
        str[i - 1 - j] = temp;
    }
}

// Convert integer to hex string
static void int_to_hex_string(uint32_t value, char* str) {
    const char* hex = "0123456789ABCDEF";
    int i = 0;
    
    str[i++] = '0';
    str[i++] = 'x';
    
    bool leading_zero = true;
    for (int j = 28; j >= 0; j -= 4) {
        uint8_t digit = (value >> j) & 0xF;
        if (digit != 0 || !leading_zero || j == 0) {
            str[i++] = hex[digit];
            leading_zero = false;
        }
    }
    str[i] = '\0';
}

// Dump driver information
void driver_dump_info(driver_t* driver) {
    if (!driver) return;
    
    char version[32];
    
    terminal_writestring("Driver Information:\n");
    terminal_writestring("  Name: ");
    terminal_writestring(driver->name);
    terminal_writestring("\n");
    
    terminal_writestring("  Description: ");
    terminal_writestring(driver->description);
    terminal_writestring("\n");
    
    terminal_writestring("  Version: ");
    int_to_string(driver->version >> 8, version);
    terminal_writestring(version);
    terminal_writestring(".");
    int_to_string(driver->version & 0xFF, version);
    terminal_writestring(version);
    terminal_writestring("\n");
    
    terminal_writestring("  Type: ");
    terminal_writestring(driver_type_strings[driver->type]);
    terminal_writestring("\n");
    
    terminal_writestring("  Flags: ");
    int_to_hex_string(driver->flags, version);
    terminal_writestring(version);
    terminal_writestring("\n");
    
    terminal_writestring("  Capabilities:\n");
    terminal_writestring("    Max Transfer Size: ");
    int_to_string(driver->caps.max_transfer_size, version);
    terminal_writestring(version);
    terminal_writestring("\n");
    
    terminal_writestring("    Buffer Alignment: ");
    int_to_string(driver->caps.buffer_alignment, version);
    terminal_writestring(version);
    terminal_writestring("\n");
    
    terminal_writestring("    DMA Support: ");
    terminal_writestring(driver->caps.dma_support ? "Yes" : "No");
    terminal_writestring("\n");
    
    terminal_writestring("    Interrupt Support: ");
    terminal_writestring(driver->caps.interrupt_support ? "Yes" : "No");
    terminal_writestring("\n");
    
    terminal_writestring("  Statistics:\n");
    terminal_writestring("    Bytes Read: ");
    int_to_string(driver->stats.bytes_read, version);
    terminal_writestring(version);
    terminal_writestring("\n");
    
    terminal_writestring("    Bytes Written: ");
    int_to_string(driver->stats.bytes_written, version);
    terminal_writestring(version);
    terminal_writestring("\n");
    
    terminal_writestring("    I/O Errors: ");
    int_to_string(driver->stats.io_errors, version);
    terminal_writestring(version);
    terminal_writestring("\n");
    
    terminal_writestring("    Interrupts: ");
    int_to_string(driver->stats.interrupts, version);
    terminal_writestring(version);
    terminal_writestring("\n");
    
    terminal_writestring("    DMA Transfers: ");
    int_to_string(driver->stats.dma_transfers, version);
    terminal_writestring(version);
    terminal_writestring("\n");
    
    terminal_writestring("    Uptime: ");
    int_to_string(driver->stats.uptime, version);
    terminal_writestring(version);
    terminal_writestring(" seconds\n");
    
    terminal_writestring("  Configuration:\n");
    terminal_writestring("    I/O Base: ");
    int_to_hex_string(driver->config.io_base, version);
    terminal_writestring(version);
    terminal_writestring("\n");
    
    terminal_writestring("    I/O Size: ");
    int_to_string(driver->config.io_size, version);
    terminal_writestring(version);
    terminal_writestring("\n");
    
    terminal_writestring("    Memory Base: ");
    int_to_hex_string(driver->config.mem_base, version);
    terminal_writestring(version);
    terminal_writestring("\n");
    
    terminal_writestring("    Memory Size: ");
    int_to_string(driver->config.mem_size, version);
    terminal_writestring(version);
    terminal_writestring("\n");
    
    terminal_writestring("    IRQ: ");
    int_to_string(driver->config.irq, version);
    terminal_writestring(version);
    terminal_writestring("\n");
    
    terminal_writestring("    DMA Channel: ");
    int_to_string(driver->config.dma_channel, version);
    terminal_writestring(version);
    terminal_writestring("\n");
}

// Get driver type string
const char* driver_type_string(driver_type_t type) {
    if (type < 0 || type >= sizeof(driver_type_strings) / sizeof(driver_type_strings[0])) {
        return "Unknown";
    }
    return driver_type_strings[type];
}

// Get driver error string
const char* driver_error_string(int error) {
    switch (error) {
        case DRIVER_SUCCESS:
            return "Success";
        case DRIVER_ERROR_INIT:
            return "Initialization error";
        case DRIVER_ERROR_BUSY:
            return "Driver busy";
        case DRIVER_ERROR_TIMEOUT:
            return "Operation timed out";
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
            return "Driver not ready";
        case DRIVER_ERROR_REMOVED:
            return "Driver removed";
        default:
            return "Unknown error";
    }
}