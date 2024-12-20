#ifndef STORAGE_ATA_H
#define STORAGE_ATA_H

#include <stdint.h>
#include "../io.h"
#include "../driver.h"

// ATA controller ports
#define ATA_PRIMARY_DATA         0x1F0
#define ATA_PRIMARY_ERROR        0x1F1
#define ATA_PRIMARY_SECCOUNT     0x1F2
#define ATA_PRIMARY_LBA_LO       0x1F3
#define ATA_PRIMARY_LBA_MID      0x1F4
#define ATA_PRIMARY_LBA_HI       0x1F5
#define ATA_PRIMARY_DRIVE_HEAD   0x1F6
#define ATA_PRIMARY_STATUS       0x1F7
#define ATA_PRIMARY_COMMAND      0x1F7

#define ATA_SECONDARY_DATA       0x170
#define ATA_SECONDARY_ERROR      0x171
#define ATA_SECONDARY_SECCOUNT   0x172
#define ATA_SECONDARY_LBA_LO     0x173
#define ATA_SECONDARY_LBA_MID    0x174
#define ATA_SECONDARY_LBA_HI     0x175
#define ATA_SECONDARY_DRIVE_HEAD 0x176
#define ATA_SECONDARY_STATUS     0x177
#define ATA_SECONDARY_COMMAND    0x177

// ATA status register bits
#define ATA_SR_BSY  0x80    // Busy
#define ATA_SR_DRDY 0x40    // Drive ready
#define ATA_SR_DF   0x20    // Drive write fault
#define ATA_SR_DSC  0x10    // Drive seek complete
#define ATA_SR_DRQ  0x08    // Data request ready
#define ATA_SR_CORR 0x04    // Corrected data
#define ATA_SR_IDX  0x02    // Index
#define ATA_SR_ERR  0x01    // Error

// ATA commands
#define ATA_CMD_READ_PIO        0x20
#define ATA_CMD_READ_PIO_EXT    0x24
#define ATA_CMD_WRITE_PIO       0x30
#define ATA_CMD_WRITE_PIO_EXT   0x34
#define ATA_CMD_CACHE_FLUSH     0xE7
#define ATA_CMD_CACHE_FLUSH_EXT 0xEA
#define ATA_CMD_IDENTIFY        0xEC

// ATA drive structure
typedef struct {
    uint16_t base;          // Base I/O port
    uint16_t ctrl;          // Control I/O port
    uint8_t slave;          // Is this a slave device?
    uint8_t type;           // Drive type (ATA/ATAPI)
    uint16_t signature;     // Drive signature
    uint16_t capabilities;  // Features supported
    uint32_t command_sets;  // Command sets supported
    uint32_t size;          // Size in sectors
    char model[41];         // Model string
} ata_drive_t;

// ATA driver functions
void ata_init(void);
int ata_detect_drives(void);
int ata_identify(ata_drive_t* drive);
int ata_read_sectors(ata_drive_t* drive, uint32_t lba, uint8_t sectors, void* buffer);
int ata_write_sectors(ata_drive_t* drive, uint32_t lba, uint8_t sectors, const void* buffer);
void ata_flush_cache(ata_drive_t* drive);

// ATA utility functions
void ata_wait_busy(ata_drive_t* drive);
void ata_wait_drq(ata_drive_t* drive);
uint8_t ata_status_read(ata_drive_t* drive);
void ata_select_drive(ata_drive_t* drive);
void ata_delay_400ns(ata_drive_t* drive);

// ATA error handling
const char* ata_error_string(uint8_t error);
void ata_print_error(uint8_t error);

// Global variables
extern ata_drive_t ata_primary_master;
extern ata_drive_t ata_primary_slave;
extern ata_drive_t ata_secondary_master;
extern ata_drive_t ata_secondary_slave;

#endif // STORAGE_ATA_H
