#include "pci.h"
#include "hal.h"
#include "string.h"
#include "driver.h"
#include "io.h"
#include "terminal.h"

// PCI configuration addresses
#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

// Maximum PCI values
#define PCI_MAX_BUSES      256
#define PCI_MAX_DEVICES    32
#define PCI_MAX_FUNCTIONS  8

// List of detected PCI devices
pci_device_t pci_devices[256];
int num_pci_devices = 0;

// Initialize PCI subsystem
void pci_init(void) {
    // Reset device count
    num_pci_devices = 0;
    
    // Scan all PCI buses
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint16_t slot = 0; slot < 32; slot++) {
            uint16_t vendor = pci_read_config(bus, slot, 0, 0) & 0xFFFF;
            if (vendor != 0xFFFF) {
                uint16_t device = (pci_read_config(bus, slot, 0, 0) >> 16) & 0xFFFF;
                uint16_t class = (pci_read_config(bus, slot, 0, 0x0B) >> 24) & 0xFF;
                uint16_t subclass = (pci_read_config(bus, slot, 0, 0x0A) >> 16) & 0xFF;
                kprintf("Found PCI device: Vendor=%x, Device=%x, Class=%x, Subclass=%x\n", 
                       vendor, device, class, subclass);
            }
        }
    }
    
    // Print detected devices
    kprintf("PCI: Detected %d devices\n", num_pci_devices);
    for (int i = 0; i < num_pci_devices; i++) {
        pci_dump_device(&pci_devices[i]);
    }
}

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

// Get device information
void pci_get_device_info(uint8_t bus, uint8_t slot, uint8_t func, pci_device_t* dev) {
    uint32_t vendor_device = pci_read_config(bus, slot, func, PCI_VENDOR_ID);
    dev->vendor_id = vendor_device & 0xFFFF;
    dev->device_id = vendor_device >> 16;

    uint32_t class_info = pci_read_config(bus, slot, func, PCI_REVISION_ID);
    dev->revision = class_info & 0xFF;
    dev->prog_if = (class_info >> 8) & 0xFF;
    dev->subclass = (class_info >> 16) & 0xFF;
    dev->class_code = (class_info >> 24) & 0xFF;

    uint32_t command_status = pci_read_config(bus, slot, func, PCI_COMMAND);
    dev->command = command_status & 0xFFFF;
    dev->status = command_status >> 16;

    uint32_t bist_header = pci_read_config(bus, slot, func, PCI_CACHE_LINE);
    dev->cache_line = bist_header & 0xFF;
    dev->latency = (bist_header >> 8) & 0xFF;
    dev->header_type = (bist_header >> 16) & 0xFF;
    dev->bist = (bist_header >> 24) & 0xFF;

    // Read BARs
    for (int i = 0; i < 6; i++) {
        dev->bar[i] = pci_read_config(bus, slot, func, PCI_BAR0 + i * 4);
    }

    // Read interrupt information
    uint32_t interrupt_info = pci_read_config(bus, slot, func, PCI_INTERRUPT_LINE);
    dev->interrupt_line = interrupt_info & 0xFF;
    dev->interrupt_pin = (interrupt_info >> 8) & 0xFF;
}

// Check if PCI device exists
int pci_device_exists(uint8_t bus, uint8_t slot, uint8_t func) {
    uint32_t vendor = pci_read_config(bus, slot, func, PCI_VENDOR_ID) & 0xFFFF;
    return vendor != 0xFFFF;
}

// Scan PCI bus for devices
void pci_scan_bus(void) {
    for (uint16_t bus = 0; bus < 256; bus++) {
        pci_scan_device(bus, 0);
    }
}

// Scan PCI device
void pci_scan_device(uint8_t bus, uint8_t device) {
    uint8_t function = 0;

    if (!pci_device_exists(bus, device, function)) {
        return;
    }

    pci_scan_function(bus, device, function);
    uint8_t header_type = pci_read_config(bus, device, 0, PCI_HEADER_TYPE) >> 16;

    if ((header_type & 0x80) != 0) {
        /* Multi-function device */
        for (function = 1; function < 8; function++) {
            if (pci_device_exists(bus, device, function)) {
                pci_scan_function(bus, device, function);
            }
        }
    }
}

// Scan PCI function
void pci_scan_function(uint8_t bus, uint8_t device, uint8_t function) {
    if (num_pci_devices >= 256) {
        return;
    }

    pci_device_t* dev = &pci_devices[num_pci_devices++];
    pci_get_device_info(bus, device, function, dev);

    if (dev->class_code == PCI_CLASS_BRIDGE && dev->subclass == 0x04) {
        uint32_t secondary_bus = (pci_read_config(bus, device, function, 0x18) >> 8) & 0xFF;
        if (secondary_bus != 0) {
            pci_scan_bus();
        }
    }
}

// Enable bus mastering
void pci_enable_bus_mastering(pci_device_t* dev) {
    (void)dev; // Unused parameter
    uint32_t command = pci_read_config(0, 0, 0, PCI_COMMAND) & 0xFFFF;
    command |= PCI_COMMAND_MASTER;
    pci_write_config(0, 0, 0, PCI_COMMAND, command);
}

// Get BAR address
uint32_t pci_get_bar_address(pci_device_t* dev, int bar) {
    if (bar < 0 || bar > 5) {
        return 0;
    }
    
    uint32_t bar_value = dev->bar[bar];
    if (bar_value & PCI_BAR_TYPE_IO) {
        return bar_value & PCI_BAR_IO_MASK;
    } else {
        return bar_value & PCI_BAR_MEM_MASK;
    }
}

// Get BAR size
uint32_t pci_get_bar_size(pci_device_t* dev, int bar) {
    if (bar < 0 || bar > 5) {
        return 0;
    }

    uint32_t old_value = dev->bar[bar];
    pci_write_config(0, 0, 0, PCI_BAR0 + bar * 4, 0xFFFFFFFF);
    uint32_t size = pci_read_config(0, 0, 0, PCI_BAR0 + bar * 4);
    pci_write_config(0, 0, 0, PCI_BAR0 + bar * 4, old_value);

    if (!size) {
        return 0;
    }

    if (old_value & PCI_BAR_TYPE_IO) {
        size &= PCI_BAR_IO_MASK;
        size = ~size + 1;
    } else {
        size &= PCI_BAR_MEM_MASK;
        size = ~size + 1;
    }

    return size;
}

// Enable interrupts
void pci_enable_interrupts(pci_device_t* dev) {
    (void)dev; // Unused parameter
    uint32_t command = pci_read_config(0, 0, 0, PCI_COMMAND) & 0xFFFF;
    command &= ~PCI_COMMAND_INTX_DISABLE;
    pci_write_config(0, 0, 0, PCI_COMMAND, command);
}

// Disable interrupts
void pci_disable_interrupts(pci_device_t* dev) {
    (void)dev; // Unused parameter
    uint32_t command = pci_read_config(0, 0, 0, PCI_COMMAND) & 0xFFFF;
    command |= PCI_COMMAND_INTX_DISABLE;
    pci_write_config(0, 0, 0, PCI_COMMAND, command);
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

// Dump PCI device information
void pci_dump_device(pci_device_t* dev) {
    char temp[9];
    kprintf("PCI Device:\n");
    
    int_to_hex_string(dev->vendor_id, temp);
    kprintf("  Vendor ID: 0x%s\n", temp);
    
    int_to_hex_string(dev->device_id, temp);
    kprintf("  Device ID: 0x%s\n", temp);
    
    kprintf("  Class: %s (0x%02x)\n", pci_class_string(dev->class_code), dev->class_code);
    kprintf("  Subclass: 0x%02x\n", dev->subclass);
    kprintf("  Prog IF: 0x%02x\n", dev->prog_if);
    kprintf("  Revision: 0x%02x\n", dev->revision);
    
    kprintf("  Command: 0x%04x\n", dev->command);
    kprintf("  Status: 0x%04x\n", dev->status);
    
    for (int i = 0; i < 6; i++) {
        if (dev->bar[i]) {
            int_to_hex_string(dev->bar[i], temp);
            kprintf("  BAR%d: 0x%s\n", i, temp);
        }
    }
    
    if (dev->interrupt_line != 0xFF) {
        kprintf("  IRQ Line: %d\n", dev->interrupt_line);
    }
    if (dev->interrupt_pin) {
        kprintf("  IRQ Pin: %d\n", dev->interrupt_pin);
    }
}

int pci_find_device_by_id(uint16_t vendor_id, uint16_t device_id, uint8_t* bus_out, uint8_t* slot_out, uint8_t* func_out) {
    // Search for device with matching vendor and device ID
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            for (uint8_t func = 0; func < 8; func++) {
                uint32_t reg = pci_read_config(bus, slot, func, 0);
                uint16_t vid = reg & 0xFFFF;
                uint16_t did = (reg >> 16) & 0xFFFF;
                
                if (vid == vendor_id && did == device_id) {
                    if (bus_out) *bus_out = bus;
                    if (slot_out) *slot_out = slot;
                    if (func_out) *func_out = func;
                    return 1;
                }
            }
        }
    }
    return 0;
}