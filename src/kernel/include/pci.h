#ifndef PCI_H
#define PCI_H

#include "io.h"
#include <stdint.h>

// PCI configuration space registers
#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

// PCI configuration space offsets
#define PCI_VENDOR_ID      0x00
#define PCI_DEVICE_ID      0x02
#define PCI_COMMAND        0x04
#define PCI_STATUS         0x06
#define PCI_REVISION_ID    0x08
#define PCI_PROG_IF        0x09
#define PCI_SUBCLASS       0x0A
#define PCI_CLASS          0x0B
#define PCI_CACHE_LINE     0x0C
#define PCI_LATENCY        0x0D
#define PCI_HEADER_TYPE    0x0E
#define PCI_BIST           0x0F
#define PCI_BAR0           0x10
#define PCI_BAR1           0x14
#define PCI_BAR2           0x18
#define PCI_BAR3           0x1C
#define PCI_BAR4           0x20
#define PCI_BAR5           0x24
#define PCI_INTERRUPT_LINE 0x3C
#define PCI_INTERRUPT_PIN  0x3D

// PCI Command Register bits
#define PCI_COMMAND_IO          0x0001
#define PCI_COMMAND_MEMORY      0x0002
#define PCI_COMMAND_MASTER      0x0004
#define PCI_COMMAND_SPECIAL     0x0008
#define PCI_COMMAND_INVALIDATE  0x0010
#define PCI_COMMAND_VGA_PALETTE 0x0020
#define PCI_COMMAND_PARITY      0x0040
#define PCI_COMMAND_WAIT        0x0080
#define PCI_COMMAND_SERR        0x0100
#define PCI_COMMAND_FAST_BACK   0x0200
#define PCI_COMMAND_INTX_DISABLE 0x0400

// PCI device classes
#define PCI_CLASS_UNCLASSIFIED    0x00
#define PCI_CLASS_STORAGE         0x01
#define PCI_CLASS_NETWORK         0x02
#define PCI_CLASS_DISPLAY         0x03
#define PCI_CLASS_MULTIMEDIA      0x04
#define PCI_CLASS_MEMORY          0x05
#define PCI_CLASS_BRIDGE          0x06
#define PCI_CLASS_COMMUNICATION   0x07
#define PCI_CLASS_SYSTEM          0x08
#define PCI_CLASS_INPUT           0x09
#define PCI_CLASS_DOCKING         0x0A
#define PCI_CLASS_PROCESSOR       0x0B
#define PCI_CLASS_SERIAL          0x0C
#define PCI_CLASS_WIRELESS        0x0D
#define PCI_CLASS_INTELLIGENT     0x0E
#define PCI_CLASS_SATELLITE       0x0F
#define PCI_CLASS_ENCRYPTION      0x10
#define PCI_CLASS_SIGNAL          0x11
#define PCI_CLASS_OTHER          0xFF

// PCI BAR types
#define PCI_BAR_TYPE_IO          0x01
#define PCI_BAR_TYPE_MEMORY      0x00
#define PCI_BAR_TYPE_MASK        0x01
#define PCI_BAR_MEM_TYPE_32      0x00
#define PCI_BAR_MEM_TYPE_64      0x04
#define PCI_BAR_MEM_PREFETCH     0x08
#define PCI_BAR_IO_MASK          0xFFFFFFFC
#define PCI_BAR_MEM_MASK         0xFFFFFFF0

// PCI device structure
typedef struct {
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t command;
    uint16_t status;
    uint8_t revision;
    uint8_t prog_if;
    uint8_t subclass;
    uint8_t class_code;
    uint8_t cache_line;
    uint8_t latency;
    uint8_t header_type;
    uint8_t bist;
    uint32_t bar[6];
    uint8_t interrupt_line;
    uint8_t interrupt_pin;
} pci_device_t;

// Function declarations
void pci_init(void);
uint32_t pci_read_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
void pci_write_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value);
void pci_get_device_info(uint8_t bus, uint8_t slot, uint8_t func, pci_device_t* dev);
int pci_device_exists(uint8_t bus, uint8_t slot, uint8_t func);
int pci_find_device_by_id(uint16_t vendor_id, uint16_t device_id, uint8_t* bus, uint8_t* slot, uint8_t* func);
void pci_scan_bus(void);
void pci_scan_device(uint8_t bus, uint8_t device);
void pci_scan_function(uint8_t bus, uint8_t device, uint8_t function);
void pci_enable_bus_mastering(pci_device_t* dev);
uint32_t pci_get_bar_address(pci_device_t* dev, int bar);
uint32_t pci_get_bar_size(pci_device_t* dev, int bar);
void pci_enable_interrupts(pci_device_t* dev);
void pci_disable_interrupts(pci_device_t* dev);
const char* pci_class_string(uint8_t class_code);
void pci_dump_device(pci_device_t* dev);

// Global variables
pci_device_t pci_devices[256];
int num_pci_devices;

#endif /* PCI_H */
