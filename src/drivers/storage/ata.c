#include "ata.h"
#include "../../kernel/io.h"
#include <string.h>

// ATA driver instance
static ata_driver_t ata_driver;

// Read a register
static inline uint8_t ata_read_reg(ata_device_t* device, uint8_t reg) {
    return inb(device->base + reg);
}

// Write a register
static inline void ata_write_reg(ata_device_t* device, uint8_t reg, uint8_t data) {
    outb(device->base + reg, data);
}

// Read data port
static inline uint16_t ata_read_data(ata_device_t* device) {
    return inw(device->base + ATA_REG_DATA);
}

// Write data port
static inline void ata_write_data(ata_device_t* device, uint16_t data) {
    outw(device->base + ATA_REG_DATA, data);
}

// Wait for 400ns
void ata_400ns_delay(ata_device_t* device) {
    ata_read_reg(device, ATA_REG_ALTSTATUS);
    ata_read_reg(device, ATA_REG_ALTSTATUS);
    ata_read_reg(device, ATA_REG_ALTSTATUS);
    ata_read_reg(device, ATA_REG_ALTSTATUS);
}

// Wait for status
uint8_t ata_status_wait(ata_device_t* device, uint8_t mask, uint8_t value) {
    uint8_t status;
    do {
        status = ata_read_reg(device, ATA_REG_STATUS);
    } while ((status & mask) != value);
    return status;
}

// Select device
void ata_select_device(ata_driver_t* driver, uint8_t device) {
    if (device > 3) return;
    if (driver->current_device == device) return;
    
    ata_device_t* dev = &driver->devices[device];
    ata_write_reg(dev, ATA_REG_HDDEVSEL, 0xA0 | (device << 4));
    ata_400ns_delay(dev);
    
    driver->current_device = device;
}

// Software reset
void ata_soft_reset(ata_device_t* device) {
    outb(device->ctrl, 0x04);
    ata_400ns_delay(device);
    outb(device->ctrl, 0x00);
}

// Initialize ATA driver
int ata_init(driver_t* driver) {
    ata_driver_t* ata = (ata_driver_t*)driver;
    
    // Initialize primary and secondary channels
    ata->devices[0].base = 0x1F0;
    ata->devices[0].ctrl = 0x3F6;
    ata->devices[1].base = 0x1F0;
    ata->devices[1].ctrl = 0x3F6;
    ata->devices[2].base = 0x170;
    ata->devices[2].ctrl = 0x376;
    ata->devices[3].base = 0x170;
    ata->devices[3].ctrl = 0x376;
    
    // Detect and initialize devices
    for (int i = 0; i < 4; i++) {
        ata_identify(ata, i);
    }
    
    return DRIVER_SUCCESS;
}

// Cleanup ATA driver
int ata_cleanup(driver_t* driver) {
    return DRIVER_SUCCESS;
}

// Identify ATA device
int ata_identify(ata_driver_t* driver, uint8_t device) {
    ata_device_t* dev = &driver->devices[device];
    uint16_t buffer[256];
    
    // Select device
    ata_select_device(driver, device);
    
    // Disable interrupts
    ata_write_reg(dev, ATA_REG_CONTROL, 0x02);
    
    // Send IDENTIFY command
    ata_write_reg(dev, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    
    // Wait for response
    uint8_t status = ata_read_reg(dev, ATA_REG_STATUS);
    if (status == 0) return DRIVER_ERROR_NOT_FOUND;
    
    // Wait until BSY clears
    ata_status_wait(dev, ATA_SR_BSY, 0);
    
    // Check for ATAPI
    if (ata_read_reg(dev, ATA_REG_LBA1) || ata_read_reg(dev, ATA_REG_LBA2)) {
        return DRIVER_ERROR_NOT_SUPPORTED;
    }
    
    // Wait for ERR or DRQ
    status = ata_status_wait(dev, ATA_SR_ERR | ATA_SR_DRQ, ATA_SR_DRQ);
    if (status & ATA_SR_ERR) {
        return DRIVER_ERROR_IO;
    }
    
    // Read identification space
    for (int i = 0; i < 256; i++) {
        buffer[i] = ata_read_data(dev);
    }
    
    return DRIVER_SUCCESS;
}

// Read sectors
static int ata_read_sectors(ata_device_t* device, uint32_t lba, uint8_t sectors, void* buffer) {
    // Select device and set up LBA
    ata_write_reg(device, ATA_REG_HDDEVSEL, 0xE0 | ((lba >> 24) & 0x0F));
    
    // Write sector count and LBA
    ata_write_reg(device, ATA_REG_SECCOUNT0, sectors);
    ata_write_reg(device, ATA_REG_LBA0, (uint8_t)lba);
    ata_write_reg(device, ATA_REG_LBA1, (uint8_t)(lba >> 8));
    ata_write_reg(device, ATA_REG_LBA2, (uint8_t)(lba >> 16));
    
    // Send read command
    ata_write_reg(device, ATA_REG_COMMAND, ATA_CMD_READ_PIO);
    
    uint16_t* buf = (uint16_t*)buffer;
    
    // Read data
    for (int i = 0; i < sectors; i++) {
        // Wait for data
        if (ata_status_wait(device, ATA_SR_BSY | ATA_SR_DRQ, ATA_SR_DRQ) & ATA_SR_ERR) {
            return DRIVER_ERROR_IO;
        }
        
        // Transfer data
        for (int j = 0; j < 256; j++) {
            buf[j] = ata_read_data(device);
        }
        
        buf += 256;
    }
    
    return DRIVER_SUCCESS;
}

// Write sectors
static int ata_write_sectors(ata_device_t* device, uint32_t lba, uint8_t sectors, const void* buffer) {
    // Select device and set up LBA
    ata_write_reg(device, ATA_REG_HDDEVSEL, 0xE0 | ((lba >> 24) & 0x0F));
    
    // Write sector count and LBA
    ata_write_reg(device, ATA_REG_SECCOUNT0, sectors);
    ata_write_reg(device, ATA_REG_LBA0, (uint8_t)lba);
    ata_write_reg(device, ATA_REG_LBA1, (uint8_t)(lba >> 8));
    ata_write_reg(device, ATA_REG_LBA2, (uint8_t)(lba >> 16));
    
    // Send write command
    ata_write_reg(device, ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);
    
    const uint16_t* buf = (const uint16_t*)buffer;
    
    // Write data
    for (int i = 0; i < sectors; i++) {
        // Wait until ready
        if (ata_status_wait(device, ATA_SR_BSY, 0) & ATA_SR_ERR) {
            return DRIVER_ERROR_IO;
        }
        
        // Transfer data
        for (int j = 0; j < 256; j++) {
            ata_write_data(device, buf[j]);
        }
        
        buf += 256;
        
        // Flush cache
        ata_write_reg(device, ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
        ata_status_wait(device, ATA_SR_BSY, 0);
    }
    
    return DRIVER_SUCCESS;
}

// Read from device
int ata_read(driver_t* driver, void* buffer, size_t size, uint32_t offset) {
    ata_driver_t* ata = (ata_driver_t*)driver;
    ata_device_t* device = &ata->devices[ata->current_device];
    
    // Calculate sectors
    uint32_t start_sector = offset / 512;
    uint32_t sector_count = (size + 511) / 512;
    
    return ata_read_sectors(device, start_sector, sector_count, buffer);
}

// Write to device
int ata_write(driver_t* driver, const void* buffer, size_t size, uint32_t offset) {
    ata_driver_t* ata = (ata_driver_t*)driver;
    ata_device_t* device = &ata->devices[ata->current_device];
    
    // Calculate sectors
    uint32_t start_sector = offset / 512;
    uint32_t sector_count = (size + 511) / 512;
    
    return ata_write_sectors(device, start_sector, sector_count, buffer);
}

// IOCTL operations
int ata_ioctl(driver_t* driver, uint32_t cmd, void* arg) {
    ata_driver_t* ata = (ata_driver_t*)driver;
    ata_device_t* device = &ata->devices[ata->current_device];
    
    switch (cmd) {
        case IOCTL_ATA_SELECT_DEVICE:
            if (!arg) return DRIVER_ERROR_INVALID;
            ata_select_device(ata, *(uint8_t*)arg);
            return DRIVER_SUCCESS;
            
        case IOCTL_ATA_FLUSH_CACHE:
            ata_write_reg(device, ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
            ata_status_wait(device, ATA_SR_BSY, 0);
            return DRIVER_SUCCESS;
            
        default:
            return DRIVER_ERROR_NOT_SUPPORTED;
    }
}

// Create and register ATA driver
driver_t* create_ata_driver(void) {
    // Initialize driver structure
    memset(&ata_driver, 0, sizeof(ata_driver_t));
    DRIVER_INIT(&ata_driver.driver, "ata", DRIVER_TYPE_STORAGE);
    
    // Set up driver operations
    ata_driver.driver.init = ata_init;
    ata_driver.driver.cleanup = ata_cleanup;
    ata_driver.driver.read = ata_read;
    ata_driver.driver.write = ata_write;
    ata_driver.driver.ioctl = ata_ioctl;
    
    // Register driver
    if (driver_register(&ata_driver.driver) != DRIVER_SUCCESS) {
        return NULL;
    }
    
    return &ata_driver.driver;
} 