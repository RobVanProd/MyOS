#ifndef PCI_H
#define PCI_H

#include <stdint.h>
#include "io.h"

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

// PCI device list
typedef struct pci_device_list {
    pci_device_t device;
    struct pci_device_list* next;
} pci_device_list_t;

// PCI configuration functions
uint32_t pci_read_config_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint16_t pci_read_config_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint8_t pci_read_config_byte(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
void pci_write_config_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value);
void pci_write_config_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t value);
void pci_write_config_byte(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint8_t value);

// PCI device management functions
void pci_init(void);
pci_device_t* pci_get_device(uint16_t vendor_id, uint16_t device_id);
pci_device_t* pci_get_device_by_class(uint8_t class_code, uint8_t subclass);
void pci_enumerate_devices(void);
void pci_enable_bus_mastering(pci_device_t* device);
void pci_enable_memory_space(pci_device_t* device);
void pci_enable_io_space(pci_device_t* device);

// PCI device list management
extern pci_device_list_t* pci_devices;
void pci_add_device(pci_device_t device);
void pci_remove_device(pci_device_t* device);
pci_device_t* pci_find_device(uint16_t vendor_id, uint16_t device_id);

// PCI utility functions
const char* pci_get_vendor_name(uint16_t vendor_id);
const char* pci_get_device_name(uint16_t vendor_id, uint16_t device_id);
const char* pci_get_class_name(uint8_t class_code);
const char* pci_get_subclass_name(uint8_t class_code, uint8_t subclass);

#endif // PCI_H
