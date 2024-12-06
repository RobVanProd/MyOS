#ifndef ATA_H
#define ATA_H

#include <driver.h>

// ATA ports
#define ATA_PRIMARY_BASE        0x1F0
#define ATA_PRIMARY_CONTROL     0x3F6
#define ATA_SECONDARY_BASE      0x170
#define ATA_SECONDARY_CONTROL   0x376

// Maximum number of ATA devices
#define ATA_MAX_DEVICES         4

// ATA registers
#define ATA_REG_DATA        0x00
#define ATA_REG_ERROR       0x01
#define ATA_REG_FEATURES    0x01
#define ATA_REG_SECCOUNT0   0x02
#define ATA_REG_LBA0        0x03
#define ATA_REG_LBA1        0x04
#define ATA_REG_LBA2        0x05
#define ATA_REG_HDDEVSEL    0x06
#define ATA_REG_COMMAND     0x07
#define ATA_REG_STATUS      0x07
#define ATA_REG_SECCOUNT1   0x08
#define ATA_REG_LBA3        0x09
#define ATA_REG_LBA4        0x0A
#define ATA_REG_LBA5        0x0B
#define ATA_REG_CONTROL     0x0C
#define ATA_REG_ALTSTATUS   0x0C
#define ATA_REG_DEVADDRESS  0x0D

// ATA commands
#define ATA_CMD_READ_PIO          0x20
#define ATA_CMD_READ_PIO_EXT      0x24
#define ATA_CMD_READ_DMA          0xC8
#define ATA_CMD_READ_DMA_EXT      0x25
#define ATA_CMD_WRITE_PIO         0x30
#define ATA_CMD_WRITE_PIO_EXT     0x34
#define ATA_CMD_WRITE_DMA         0xCA
#define ATA_CMD_WRITE_DMA_EXT     0x35
#define ATA_CMD_CACHE_FLUSH       0xE7
#define ATA_CMD_CACHE_FLUSH_EXT   0xEA
#define ATA_CMD_PACKET            0xA0
#define ATA_CMD_IDENTIFY_PACKET   0xA1
#define ATA_CMD_IDENTIFY          0xEC

// ATA status
#define ATA_SR_BSY     0x80    // Busy
#define ATA_SR_DRDY    0x40    // Drive ready
#define ATA_SR_DF      0x20    // Drive write fault
#define ATA_SR_DSC     0x10    // Drive seek complete
#define ATA_SR_DRQ     0x08    // Data request ready
#define ATA_SR_CORR    0x04    // Corrected data
#define ATA_SR_IDX     0x02    // Index
#define ATA_SR_ERR     0x01    // Error

// ATA error
#define ATA_ER_BBK     0x80    // Bad block
#define ATA_ER_UNC     0x40    // Uncorrectable data
#define ATA_ER_MC      0x20    // Media changed
#define ATA_ER_IDNF    0x10    // ID mark not found
#define ATA_ER_MCR     0x08    // Media change request
#define ATA_ER_ABRT    0x04    // Command aborted
#define ATA_ER_TK0NF   0x02    // Track 0 not found
#define ATA_ER_AMNF    0x01    // No address mark

// ATA identification space
#define ATA_IDENT_DEVICETYPE   0
#define ATA_IDENT_CYLINDERS    2
#define ATA_IDENT_HEADS        6
#define ATA_IDENT_SECTORS      12
#define ATA_IDENT_SERIAL       20
#define ATA_IDENT_MODEL        54
#define ATA_IDENT_CAPABILITIES 98
#define ATA_IDENT_FIELDVALID  106
#define ATA_IDENT_MAX_LBA     120
#define ATA_IDENT_COMMANDSETS 164
#define ATA_IDENT_MAX_LBA_EXT 200

// ATA device structure
typedef struct {
    uint16_t base;          // I/O base port
    uint16_t ctrl;          // Control port
    uint16_t bmide;         // Bus master IDE port
    uint8_t  nIEN;          // nIEN (No Interrupt) bit
    uint8_t  selected;      // Currently selected drive
    uint8_t  lba;           // Using LBA?
    uint32_t bar0;          // BAR0
    uint32_t bar1;          // BAR1
    uint32_t bar2;          // BAR2
    uint32_t bar3;          // BAR3
    uint32_t bar4;          // BAR4
    uint32_t bar5;          // BAR5
} ata_device_t;

// ATA driver structure
typedef struct {
    driver_t driver;        // Base driver
    ata_device_t devices[4]; // ATA devices (primary/secondary, master/slave)
    uint8_t current_device; // Currently selected device
} ata_driver_t;

// ATA driver functions
int ata_init(driver_t* driver);
int ata_cleanup(driver_t* driver);
int ata_read(driver_t* driver, void* buffer, size_t size, uint32_t offset);
int ata_write(driver_t* driver, const void* buffer, size_t size, uint32_t offset);
int ata_ioctl(driver_t* driver, uint32_t cmd, void* arg);

// ATA utility functions
void ata_select_device(ata_driver_t* driver, uint8_t device);
void ata_400ns_delay(ata_device_t* device);
uint8_t ata_status_wait(ata_device_t* device, uint8_t mask, uint8_t value);
int ata_identify(ata_driver_t* driver, uint8_t device);
void ata_io_wait(ata_device_t* device);
void ata_soft_reset(ata_device_t* device);

// ATA IOCTL commands
#define IOCTL_ATA_GET_SECTOR_COUNT    0x1000
#define IOCTL_ATA_GET_SECTOR_SIZE     0x1001
#define IOCTL_ATA_GET_MODEL           0x1002
#define IOCTL_ATA_GET_SERIAL          0x1003
#define IOCTL_ATA_FLUSH_CACHE         0x1004
#define IOCTL_ATA_SELECT_DEVICE       0x1005

#endif // ATA_H