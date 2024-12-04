#ifndef PCI_H
#define PCI_H

#include <stdint.h>

// PCI configuration space registers
#define PCI_CONFIG_VENDOR_ID      0x00
#define PCI_CONFIG_DEVICE_ID      0x02
#define PCI_CONFIG_COMMAND        0x04
#define PCI_CONFIG_STATUS         0x06
#define PCI_CONFIG_REVISION_ID    0x08
#define PCI_CONFIG_PROG_IF        0x09
#define PCI_CONFIG_SUBCLASS       0x0A
#define PCI_CONFIG_CLASS          0x0B
#define PCI_CONFIG_CACHE_LINE     0x0C
#define PCI_CONFIG_LATENCY        0x0D
#define PCI_CONFIG_HEADER_TYPE    0x0E
#define PCI_CONFIG_BIST           0x0F
#define PCI_CONFIG_BAR0           0x10
#define PCI_CONFIG_BAR1           0x14
#define PCI_CONFIG_BAR2           0x18
#define PCI_CONFIG_BAR3           0x1C
#define PCI_CONFIG_BAR4           0x20
#define PCI_CONFIG_BAR5           0x24
#define PCI_CONFIG_CARDBUS_CIS    0x28
#define PCI_CONFIG_SUBSYS_VENDOR  0x2C
#define PCI_CONFIG_SUBSYS_ID      0x2E
#define PCI_CONFIG_ROM_BASE       0x30
#define PCI_CONFIG_CAPABILITIES   0x34
#define PCI_CONFIG_INTERRUPT_LINE 0x3C
#define PCI_CONFIG_INTERRUPT_PIN  0x3D
#define PCI_CONFIG_MIN_GRANT      0x3E
#define PCI_CONFIG_MAX_LATENCY    0x3F

// PCI command register bits
#define PCI_COMMAND_IO            0x0001
#define PCI_COMMAND_MEMORY        0x0002
#define PCI_COMMAND_MASTER        0x0004
#define PCI_COMMAND_SPECIAL       0x0008
#define PCI_COMMAND_INVALIDATE    0x0010
#define PCI_COMMAND_VGA_PALETTE   0x0020
#define PCI_COMMAND_PARITY        0x0040
#define PCI_COMMAND_WAIT          0x0080
#define PCI_COMMAND_SERR          0x0100
#define PCI_COMMAND_FAST_BACK     0x0200
#define PCI_COMMAND_INTERRUPT_DISABLE 0x0400

// PCI status register bits
#define PCI_STATUS_INTERRUPT      0x0008
#define PCI_STATUS_CAPABILITIES   0x0010
#define PCI_STATUS_66MHZ         0x0020
#define PCI_STATUS_FAST_BACK     0x0080
#define PCI_STATUS_MASTER_ABORT  0x2000
#define PCI_STATUS_PARITY_ERROR  0x4000
#define PCI_STATUS_DEVSEL_MASK   0x0600
#define PCI_STATUS_DEVSEL_FAST   0x0000
#define PCI_STATUS_DEVSEL_MEDIUM 0x0200
#define PCI_STATUS_DEVSEL_SLOW   0x0400

// PCI BAR types
#define PCI_BAR_TYPE_MASK        0x01
#define PCI_BAR_TYPE_MEMORY      0x00
#define PCI_BAR_TYPE_IO          0x01
#define PCI_BAR_MEMORY_TYPE_32   0x00
#define PCI_BAR_MEMORY_TYPE_64   0x04
#define PCI_BAR_MEMORY_PREFETCH  0x08

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
    uint32_t cardbus_cis;
    uint16_t subsys_vendor;
    uint16_t subsys_id;
    uint32_t rom_base;
    uint8_t capabilities;
    uint8_t interrupt_line;
    uint8_t interrupt_pin;
    uint8_t min_grant;
    uint8_t max_latency;
} pci_device_t;

// PCI device class codes
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
#define PCI_CLASS_ACCELERATOR     0x12
#define PCI_CLASS_INSTRUMENTATION 0x13
#define PCI_CLASS_COPROCESSOR     0x40

// PCI function prototypes
void pci_init(void);
uint32_t pci_read_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
void pci_write_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value);
pci_device_t* pci_get_device(uint16_t vendor_id, uint16_t device_id);
void pci_enable_bus_mastering(pci_device_t* dev);
uint32_t pci_get_bar_address(pci_device_t* dev, int bar);
uint32_t pci_get_bar_size(pci_device_t* dev, int bar);
int pci_get_interrupt_line(pci_device_t* dev);
void pci_enable_interrupts(pci_device_t* dev);
void pci_disable_interrupts(pci_device_t* dev);

// PCI scanning functions
void pci_scan_bus(void);
void pci_scan_device(uint8_t bus, uint8_t device);
void pci_scan_function(uint8_t bus, uint8_t device, uint8_t function);

// PCI utility functions
const char* pci_class_string(uint8_t class_code);
const char* pci_vendor_string(uint16_t vendor_id);
void pci_dump_device(pci_device_t* dev);

#endif 