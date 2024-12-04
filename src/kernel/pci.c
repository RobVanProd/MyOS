#include "pci.h"
#include "io.h"
#include <string.h>

// PCI configuration addresses
#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

// Maximum PCI values
#define PCI_MAX_BUSES      256
#define PCI_MAX_DEVICES    32
#define PCI_MAX_FUNCTIONS  8

// List of detected PCI devices
static pci_device_t pci_devices[256];
static int num_pci_devices = 0;

// Read from PCI configuration space
uint32_t pci_read_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) |
                                 (func << 8) | (offset & 0xFC) | 0x80000000);
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

// Write to PCI configuration space
void pci_write_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) |
                                 (func << 8) | (offset & 0xFC) | 0x80000000);
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, value);
}

// Get device info from configuration space
static void pci_get_device_info(uint8_t bus, uint8_t slot, uint8_t func, pci_device_t* dev) {
    dev->vendor_id = pci_read_config(bus, slot, func, PCI_CONFIG_VENDOR_ID) & 0xFFFF;
    dev->device_id = pci_read_config(bus, slot, func, PCI_CONFIG_DEVICE_ID) >> 16;
    
    uint32_t class_info = pci_read_config(bus, slot, func, PCI_CONFIG_REVISION_ID);
    dev->revision = class_info & 0xFF;
    dev->prog_if = (class_info >> 8) & 0xFF;
    dev->subclass = (class_info >> 16) & 0xFF;
    dev->class_code = (class_info >> 24) & 0xFF;
    
    uint32_t command_status = pci_read_config(bus, slot, func, PCI_CONFIG_COMMAND);
    dev->command = command_status & 0xFFFF;
    dev->status = command_status >> 16;
    
    uint32_t bist_header = pci_read_config(bus, slot, func, PCI_CONFIG_CACHE_LINE);
    dev->cache_line = bist_header & 0xFF;
    dev->latency = (bist_header >> 8) & 0xFF;
    dev->header_type = (bist_header >> 16) & 0xFF;
    dev->bist = (bist_header >> 24) & 0xFF;
    
    // Read BAR registers
    for (int i = 0; i < 6; i++) {
        dev->bar[i] = pci_read_config(bus, slot, func, PCI_CONFIG_BAR0 + i * 4);
    }
    
    dev->interrupt_line = pci_read_config(bus, slot, func, PCI_CONFIG_INTERRUPT_LINE) & 0xFF;
    dev->interrupt_pin = (pci_read_config(bus, slot, func, PCI_CONFIG_INTERRUPT_PIN) >> 8) & 0xFF;
}

// Check if device exists
static int pci_device_exists(uint8_t bus, uint8_t slot, uint8_t func) {
    uint32_t vendor = pci_read_config(bus, slot, func, PCI_CONFIG_VENDOR_ID) & 0xFFFF;
    return vendor != 0xFFFF;
}

// Scan PCI function
void pci_scan_function(uint8_t bus, uint8_t device, uint8_t function) {
    if (!pci_device_exists(bus, device, function)) return;
    if (num_pci_devices >= 256) return;
    
    pci_get_device_info(bus, device, function, &pci_devices[num_pci_devices]);
    num_pci_devices++;
    
    // If this is a PCI-to-PCI bridge, scan the secondary bus
    if (pci_devices[num_pci_devices-1].class_code == PCI_CLASS_BRIDGE &&
        pci_devices[num_pci_devices-1].subclass == 0x04) {
        uint32_t secondary_bus = (pci_read_config(bus, device, function, 0x18) >> 8) & 0xFF;
        pci_scan_bus(secondary_bus);
    }
}

// Scan PCI device
void pci_scan_device(uint8_t bus, uint8_t device) {
    if (!pci_device_exists(bus, device, 0)) return;
    
    pci_scan_function(bus, device, 0);
    
    // If this is a multi-function device, scan other functions
    uint32_t header_type = pci_read_config(bus, device, 0, PCI_CONFIG_HEADER_TYPE);
    if ((header_type & 0x80) != 0) {
        for (int function = 1; function < PCI_MAX_FUNCTIONS; function++) {
            if (pci_device_exists(bus, device, function)) {
                pci_scan_function(bus, device, function);
            }
        }
    }
}

// Scan PCI bus
void pci_scan_bus(uint8_t bus) {
    for (int device = 0; device < PCI_MAX_DEVICES; device++) {
        pci_scan_device(bus, device);
    }
}

// Initialize PCI subsystem
void pci_init(void) {
    num_pci_devices = 0;
    pci_scan_bus(0);
}

// Find PCI device by vendor and device ID
pci_device_t* pci_get_device(uint16_t vendor_id, uint16_t device_id) {
    for (int i = 0; i < num_pci_devices; i++) {
        if (pci_devices[i].vendor_id == vendor_id &&
            pci_devices[i].device_id == device_id) {
            return &pci_devices[i];
        }
    }
    return NULL;
}

// Enable bus mastering for device
void pci_enable_bus_mastering(pci_device_t* dev) {
    uint16_t command = dev->command;
    command |= PCI_COMMAND_MASTER | PCI_COMMAND_IO | PCI_COMMAND_MEMORY;
    pci_write_config(0, 0, 0, PCI_CONFIG_COMMAND, command);
    dev->command = command;
}

// Get BAR address
uint32_t pci_get_bar_address(pci_device_t* dev, int bar) {
    if (bar < 0 || bar > 5) return 0;
    
    uint32_t bar_value = dev->bar[bar];
    if (bar_value & PCI_BAR_TYPE_IO) {
        return bar_value & ~0x3;
    } else {
        return bar_value & ~0xF;
    }
}

// Get BAR size
uint32_t pci_get_bar_size(pci_device_t* dev, int bar) {
    if (bar < 0 || bar > 5) return 0;
    
    uint32_t bar_value = dev->bar[bar];
    uint32_t size;
    
    // Save original value
    pci_write_config(0, 0, 0, PCI_CONFIG_BAR0 + bar * 4, 0xFFFFFFFF);
    size = pci_read_config(0, 0, 0, PCI_CONFIG_BAR0 + bar * 4);
    pci_write_config(0, 0, 0, PCI_CONFIG_BAR0 + bar * 4, bar_value);
    
    if (bar_value & PCI_BAR_TYPE_IO) {
        size = ~(size & ~0x3) + 1;
    } else {
        size = ~(size & ~0xF) + 1;
    }
    
    return size;
}

// Get interrupt line
int pci_get_interrupt_line(pci_device_t* dev) {
    return dev->interrupt_line;
}

// Enable interrupts
void pci_enable_interrupts(pci_device_t* dev) {
    uint16_t command = dev->command;
    command &= ~PCI_COMMAND_INTERRUPT_DISABLE;
    pci_write_config(0, 0, 0, PCI_CONFIG_COMMAND, command);
    dev->command = command;
}

// Disable interrupts
void pci_disable_interrupts(pci_device_t* dev) {
    uint16_t command = dev->command;
    command |= PCI_COMMAND_INTERRUPT_DISABLE;
    pci_write_config(0, 0, 0, PCI_CONFIG_COMMAND, command);
    dev->command = command;
}

// Get class string
const char* pci_class_string(uint8_t class_code) {
    switch (class_code) {
        case PCI_CLASS_UNCLASSIFIED: return "Unclassified";
        case PCI_CLASS_STORAGE: return "Mass Storage Controller";
        case PCI_CLASS_NETWORK: return "Network Controller";
        case PCI_CLASS_DISPLAY: return "Display Controller";
        case PCI_CLASS_MULTIMEDIA: return "Multimedia Controller";
        case PCI_CLASS_MEMORY: return "Memory Controller";
        case PCI_CLASS_BRIDGE: return "Bridge Device";
        case PCI_CLASS_COMMUNICATION: return "Communication Controller";
        case PCI_CLASS_SYSTEM: return "System Peripheral";
        case PCI_CLASS_INPUT: return "Input Device";
        case PCI_CLASS_DOCKING: return "Docking Station";
        case PCI_CLASS_PROCESSOR: return "Processor";
        case PCI_CLASS_SERIAL: return "Serial Bus Controller";
        case PCI_CLASS_WIRELESS: return "Wireless Controller";
        case PCI_CLASS_INTELLIGENT: return "Intelligent Controller";
        case PCI_CLASS_SATELLITE: return "Satellite Controller";
        case PCI_CLASS_ENCRYPTION: return "Encryption Controller";
        case PCI_CLASS_SIGNAL: return "Signal Processing Controller";
        default: return "Unknown Class";
    }
}

// Get vendor string
const char* pci_vendor_string(uint16_t vendor_id) {
    switch (vendor_id) {
        case 0x8086: return "Intel Corporation";
        case 0x1022: return "Advanced Micro Devices";
        case 0x10DE: return "NVIDIA Corporation";
        case 0x1002: return "ATI Technologies";
        case 0x10EC: return "Realtek Semiconductor";
        case 0x1AF4: return "Red Hat, Inc.";
        case 0x1B36: return "Red Hat, Inc.";
        default: return "Unknown Vendor";
    }
}

// Dump PCI device information
void pci_dump_device(pci_device_t* dev) {
    printf("PCI Device Information:\n");
    printf("  Vendor: %s (0x%04X)\n", pci_vendor_string(dev->vendor_id), dev->vendor_id);
    printf("  Device ID: 0x%04X\n", dev->device_id);
    printf("  Class: %s (0x%02X)\n", pci_class_string(dev->class_code), dev->class_code);
    printf("  Subclass: 0x%02X\n", dev->subclass);
    printf("  Prog IF: 0x%02X\n", dev->prog_if);
    printf("  Revision: 0x%02X\n", dev->revision);
    printf("  IRQ Line: %d\n", dev->interrupt_line);
    printf("  IRQ Pin: %d\n", dev->interrupt_pin);
    
    for (int i = 0; i < 6; i++) {
        if (dev->bar[i]) {
            printf("  BAR%d: 0x%08X (Size: %d bytes)\n", i, 
                   pci_get_bar_address(dev, i),
                   pci_get_bar_size(dev, i));
        }
    }
} 