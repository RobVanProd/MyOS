#include "rtl8139.h"
#include "../../kernel/io.h"
#include "../../kernel/memory.h"
#include <string.h>

// RTL8139 driver instance
static rtl8139_driver_t rtl8139_driver;

// Receive buffer
static uint8_t* rx_buffer;
static uint32_t rx_buffer_offset;

// Transmit buffers
static uint8_t* tx_buffers[4];
static uint32_t tx_current;

// Initialize RTL8139 device
static int rtl8139_init_device(rtl8139_driver_t* driver) {
    // Software reset
    outb(driver->io_base + RTL8139_CMD, RTL8139_CMD_RESET);
    while ((inb(driver->io_base + RTL8139_CMD) & RTL8139_CMD_RESET) != 0);
    
    // Allocate receive buffer
    rx_buffer = kmalloc_aligned(RTL8139_RX_BUFFER_SIZE + 16);
    if (!rx_buffer) return DRIVER_ERROR_MEMORY;
    rx_buffer_offset = 0;
    
    // Allocate transmit buffers
    for (int i = 0; i < RTL8139_NUM_TX_DESC; i++) {
        tx_buffers[i] = kmalloc_aligned(RTL8139_TX_BUFFER_SIZE);
        if (!tx_buffers[i]) {
            // Clean up on failure
            for (int j = 0; j < i; j++) {
                kfree(tx_buffers[j]);
            }
            kfree(rx_buffer);
            return DRIVER_ERROR_MEMORY;
        }
    }
    tx_current = 0;
    
    // Set receive buffer address
    outl(driver->io_base + RTL8139_RBSTART, (uint32_t)rx_buffer);
    
    // Enable receive and transmit
    outb(driver->io_base + RTL8139_CMD, RTL8139_CMD_RX_ENABLE | RTL8139_CMD_TX_ENABLE);
    
    // Configure receive buffer
    outl(driver->io_base + RTL8139_RCR, RTL8139_RCR_AAP |     // Accept all packets
                                       RTL8139_RCR_APM |     // Accept physical match
                                       RTL8139_RCR_AM |      // Accept multicast
                                       RTL8139_RCR_AB |      // Accept broadcast
                                       RTL8139_RCR_WRAP |    // Wrap around buffer
                                       RTL8139_RCR_RBLEN_32K |
                                       RTL8139_RCR_MXDMA_UNLIMITED);
    
    // Configure transmit
    outl(driver->io_base + RTL8139_TCR, RTL8139_TCR_MXDMA_2048 |
                                       RTL8139_TCR_IFG_STD);
    
    // Enable interrupts
    outw(driver->io_base + RTL8139_IMR, RTL8139_ISR_ROK | RTL8139_ISR_TOK);
    
    // Read MAC address
    for (int i = 0; i < 6; i++) {
        driver->mac_addr[i] = inb(driver->io_base + RTL8139_IDR0 + i);
    }
    
    return DRIVER_SUCCESS;
}

// Initialize RTL8139 driver
int rtl8139_init(driver_t* driver) {
    rtl8139_driver_t* rtl = (rtl8139_driver_t*)driver;
    
    // Find RTL8139 PCI device
    pci_device_t* pci_dev = pci_get_device(RTL8139_VENDOR_ID, RTL8139_DEVICE_ID);
    if (!pci_dev) return DRIVER_ERROR_NOT_FOUND;
    
    // Enable bus mastering
    pci_enable_bus_mastering(pci_dev);
    
    // Get I/O base address
    rtl->io_base = pci_get_bar_address(pci_dev, 0);
    if (!rtl->io_base) return DRIVER_ERROR_IO;
    
    // Get IRQ line
    rtl->irq = pci_get_interrupt_line(pci_dev);
    
    // Initialize device
    return rtl8139_init_device(rtl);
}

// Cleanup RTL8139 driver
int rtl8139_cleanup(driver_t* driver) {
    rtl8139_driver_t* rtl = (rtl8139_driver_t*)driver;
    
    // Disable receive and transmit
    outb(rtl->io_base + RTL8139_CMD, 0);
    
    // Disable interrupts
    outw(rtl->io_base + RTL8139_IMR, 0);
    
    // Free buffers
    kfree(rx_buffer);
    for (int i = 0; i < RTL8139_NUM_TX_DESC; i++) {
        kfree(tx_buffers[i]);
    }
    
    return DRIVER_SUCCESS;
}

// Handle receive
static void rtl8139_handle_receive(rtl8139_driver_t* driver) {
    uint16_t status;
    
    while ((status = inb(driver->io_base + RTL8139_CMD)) & RTL8139_CMD_RX_ENABLE) {
        if (status & RTL8139_CMD_RX_EMPTY) break;
        
        // Get packet header
        rtl8139_header_t* header = (rtl8139_header_t*)(rx_buffer + rx_buffer_offset);
        
        // Check for errors
        if (header->status & (RTL8139_RX_STATUS_FAE |
                            RTL8139_RX_STATUS_CRC |
                            RTL8139_RX_STATUS_RUNT |
                            RTL8139_RX_STATUS_LONG)) {
            driver->stats.rx_errors++;
            continue;
        }
        
        // Get packet size
        uint16_t size = header->size - 4; // Subtract CRC
        
        // Update statistics
        driver->stats.rx_packets++;
        driver->stats.rx_bytes += size;
        
        // Move to next packet
        rx_buffer_offset = (rx_buffer_offset + size + 4 + 3) & ~3;
        if (rx_buffer_offset > RTL8139_RX_BUFFER_SIZE) {
            rx_buffer_offset = 0;
        }
        
        // Update CAPR
        outw(driver->io_base + RTL8139_CAPR, rx_buffer_offset - 16);
    }
}

// Handle transmit
static void rtl8139_handle_transmit(rtl8139_driver_t* driver) {
    uint32_t status = inl(driver->io_base + RTL8139_TSD0 + (tx_current * 4));
    
    if (status & RTL8139_TX_STATUS_TOK) {
        // Transmission successful
        driver->stats.tx_packets++;
        driver->stats.tx_bytes += (status >> 16) & 0x1FFF;
    } else if (status & RTL8139_TX_STATUS_TUN) {
        // Transmit FIFO underrun
        driver->stats.tx_errors++;
    }
    
    // Move to next transmit buffer
    tx_current = (tx_current + 1) % RTL8139_NUM_TX_DESC;
}

// Interrupt handler
void rtl8139_handle_interrupt(rtl8139_driver_t* driver) {
    uint16_t status = inw(driver->io_base + RTL8139_ISR);
    
    // Clear interrupts
    outw(driver->io_base + RTL8139_ISR, status);
    
    // Handle receive
    if (status & RTL8139_ISR_ROK) {
        rtl8139_handle_receive(driver);
    }
    
    // Handle transmit
    if (status & RTL8139_ISR_TOK) {
        rtl8139_handle_transmit(driver);
    }
}

// Transmit packet
int rtl8139_transmit_packet(rtl8139_driver_t* driver, const void* data, size_t length) {
    if (length > RTL8139_TX_BUFFER_SIZE) {
        return DRIVER_ERROR_INVALID;
    }
    
    // Wait for transmit buffer to be available
    uint32_t status;
    do {
        status = inl(driver->io_base + RTL8139_TSD0 + (tx_current * 4));
    } while (!(status & RTL8139_TX_STATUS_OWN));
    
    // Copy data to transmit buffer
    memcpy(tx_buffers[tx_current], data, length);
    
    // Set up transmit
    outl(driver->io_base + RTL8139_TSAD0 + (tx_current * 4), (uint32_t)tx_buffers[tx_current]);
    outl(driver->io_base + RTL8139_TSD0 + (tx_current * 4), length);
    
    return DRIVER_SUCCESS;
}

// Receive packet
int rtl8139_receive_packet(rtl8139_driver_t* driver, void* buffer, size_t max_length) {
    rtl8139_header_t* header = (rtl8139_header_t*)(rx_buffer + rx_buffer_offset);
    
    // Check if packet available
    if (!(inb(driver->io_base + RTL8139_CMD) & RTL8139_CMD_RX_EMPTY)) {
        return 0;
    }
    
    // Check for errors
    if (header->status & (RTL8139_RX_STATUS_FAE |
                         RTL8139_RX_STATUS_CRC |
                         RTL8139_RX_STATUS_RUNT |
                         RTL8139_RX_STATUS_LONG)) {
        driver->stats.rx_errors++;
        return DRIVER_ERROR_IO;
    }
    
    // Get packet size
    uint16_t size = header->size - 4; // Subtract CRC
    if (size > max_length) {
        return DRIVER_ERROR_INVALID;
    }
    
    // Copy data to buffer
    memcpy(buffer, rx_buffer + rx_buffer_offset + sizeof(rtl8139_header_t), size);
    
    // Move to next packet
    rx_buffer_offset = (rx_buffer_offset + size + 4 + 3) & ~3;
    if (rx_buffer_offset > RTL8139_RX_BUFFER_SIZE) {
        rx_buffer_offset = 0;
    }
    
    // Update CAPR
    outw(driver->io_base + RTL8139_CAPR, rx_buffer_offset - 16);
    
    return size;
}

// IOCTL operations
int rtl8139_ioctl(driver_t* driver, uint32_t cmd, void* arg) {
    rtl8139_driver_t* rtl = (rtl8139_driver_t*)driver;
    
    switch (cmd) {
        case IOCTL_RTL8139_GET_MAC:
            if (!arg) return DRIVER_ERROR_INVALID;
            memcpy(arg, rtl->mac_addr, 6);
            return DRIVER_SUCCESS;
            
        case IOCTL_RTL8139_SET_PROMISCUOUS:
            if (!arg) return DRIVER_ERROR_INVALID;
            if (*(int*)arg) {
                outl(rtl->io_base + RTL8139_RCR,
                     inl(rtl->io_base + RTL8139_RCR) | RTL8139_RCR_AAP);
            } else {
                outl(rtl->io_base + RTL8139_RCR,
                     inl(rtl->io_base + RTL8139_RCR) & ~RTL8139_RCR_AAP);
            }
            return DRIVER_SUCCESS;
            
        case IOCTL_RTL8139_GET_STATS:
            if (!arg) return DRIVER_ERROR_INVALID;
            memcpy(arg, &rtl->stats, sizeof(rtl8139_stats_t));
            return DRIVER_SUCCESS;
            
        default:
            return DRIVER_ERROR_NOT_SUPPORTED;
    }
}

// Create and register RTL8139 driver
driver_t* create_rtl8139_driver(void) {
    // Initialize driver structure
    memset(&rtl8139_driver, 0, sizeof(rtl8139_driver_t));
    DRIVER_INIT(&rtl8139_driver.driver, "rtl8139", DRIVER_TYPE_NETWORK);
    
    // Set up driver operations
    rtl8139_driver.driver.init = rtl8139_init;
    rtl8139_driver.driver.cleanup = rtl8139_cleanup;
    rtl8139_driver.driver.read = rtl8139_receive_packet;
    rtl8139_driver.driver.write = rtl8139_transmit_packet;
    rtl8139_driver.driver.ioctl = rtl8139_ioctl;
    
    // Register driver
    if (driver_register(&rtl8139_driver.driver) != DRIVER_SUCCESS) {
        return NULL;
    }
    
    return &rtl8139_driver.driver;
} 