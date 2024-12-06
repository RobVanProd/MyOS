#include "network/rtl8139.h"
#include "io.h"
#include "memory.h"
#include "interrupt.h"
#include "string.h"
#include "kprintf.h"

// Global RTL8139 device instance
rtl8139_device_t rtl8139_device;

// Helper functions
static void rtl8139_write8(rtl8139_device_t* rtl, uint16_t reg, uint8_t value) {
    outb(rtl->io_base + reg, value);
}

static void rtl8139_write16(rtl8139_device_t* rtl, uint16_t reg, uint16_t value) {
    outw(rtl->io_base + reg, value);
}

static void rtl8139_write32(rtl8139_device_t* rtl, uint16_t reg, uint32_t value) {
    outl(rtl->io_base + reg, value);
}

static uint8_t rtl8139_read8(rtl8139_device_t* rtl, uint16_t reg) {
    return inb(rtl->io_base + reg);
}

static uint16_t rtl8139_read16(rtl8139_device_t* rtl, uint16_t reg) {
    return inw(rtl->io_base + reg);
}

static uint32_t rtl8139_read32(rtl8139_device_t* rtl, uint16_t reg) {
    return inl(rtl->io_base + reg);
}

// Initialize the RTL8139 device
int rtl8139_init_device(rtl8139_device_t* rtl) {
    // Power on the device
    rtl8139_write8(rtl, RTL8139_CONFIG_1, 0x00);
    
    // Software reset
    rtl8139_write8(rtl, RTL8139_CMD, RTL8139_CMD_RESET);
    
    // Wait for reset to complete
    while (rtl8139_read8(rtl, RTL8139_CMD) & RTL8139_CMD_RESET);
    
    // Allocate receive buffer
    rtl->rx_buffer = (uint8_t*)kmalloc(RTL8139_RX_BUF_SIZE + 16);
    if (!rtl->rx_buffer) {
        return -1;
    }
    
    // Allocate transmit buffers
    for (int i = 0; i < RTL8139_NUM_TX_DESC; i++) {
        rtl->tx_buffer[i] = (uint8_t*)kmalloc(RTL8139_TX_BUF_SIZE);
        if (!rtl->tx_buffer[i]) {
            // Free previously allocated buffers
            for (int j = 0; j < i; j++) {
                kfree(rtl->tx_buffer[j]);
            }
            kfree(rtl->rx_buffer);
            return -1;
        }
    }
    
    // Set receive buffer address
    rtl8139_write32(rtl, RTL8139_RX_BUF, (uint32_t)rtl->rx_buffer);
    
    // Read MAC address
    for (int i = 0; i < 6; i++) {
        rtl->mac_addr[i] = rtl8139_read8(rtl, i);
    }
    
    // Configure receive and transmit settings
    rtl8139_write32(rtl, RTL8139_RX_CONFIG,
        RTL8139_RX_CONFIG_AAP |    // Accept all physical packets
        RTL8139_RX_CONFIG_APM |    // Accept physical match packets
        RTL8139_RX_CONFIG_AM |     // Accept multicast
        RTL8139_RX_CONFIG_AB);     // Accept broadcast
    
    // Enable transmit and receive
    rtl8139_write8(rtl, RTL8139_CMD, RTL8139_CMD_RX_EN | RTL8139_CMD_TX_EN);
    
    // Enable interrupts
    rtl8139_write16(rtl, RTL8139_IMR,
        RTL8139_ISR_ROK |     // Receive OK
        RTL8139_ISR_TOK |     // Transmit OK
        RTL8139_ISR_RX_ERR |  // Receive error
        RTL8139_ISR_TX_ERR);  // Transmit error
    
    return 0;
}

// Driver initialization
int rtl8139_init(driver_t* driver) {
    rtl8139_device_t* rtl = (rtl8139_device_t*)driver;
    
    // Initialize device
    if (rtl8139_init_device(rtl) < 0) {
        return -1;
    }
    
    kprintf("RTL8139: MAC Address %02x:%02x:%02x:%02x:%02x:%02x\n",
        rtl->mac_addr[0], rtl->mac_addr[1], rtl->mac_addr[2],
        rtl->mac_addr[3], rtl->mac_addr[4], rtl->mac_addr[5]);
    
    return 0;
}

// Driver cleanup
int rtl8139_cleanup(driver_t* driver) {
    rtl8139_device_t* rtl = (rtl8139_device_t*)driver;
    
    // Disable device
    rtl8139_write8(rtl, RTL8139_CMD, 0);
    
    // Free buffers
    if (rtl->rx_buffer) {
        kfree(rtl->rx_buffer);
    }
    
    for (int i = 0; i < RTL8139_NUM_TX_DESC; i++) {
        if (rtl->tx_buffer[i]) {
            kfree(rtl->tx_buffer[i]);
        }
    }
    
    return 0;
}

// Send a packet
int rtl8139_send_packet(rtl8139_device_t* rtl, const void* data, size_t length) {
    if (length > RTL8139_TX_BUF_SIZE) {
        return -1;
    }
    
    // Wait for current transmission to complete
    while (!(rtl8139_read32(rtl, RTL8139_TX_STATUS + rtl->tx_cur * 4) & RTL8139_CMD_TX_DONE));
    
    // Copy data to transmit buffer
    memcpy(rtl->tx_buffer[rtl->tx_cur], data, length);
    
    // Set transmit configuration
    rtl8139_write32(rtl, RTL8139_TX_ADDR + rtl->tx_cur * 4, (uint32_t)rtl->tx_buffer[rtl->tx_cur]);
    rtl8139_write32(rtl, RTL8139_TX_STATUS + rtl->tx_cur * 4, length);
    
    // Move to next transmit buffer
    rtl->tx_cur = (rtl->tx_cur + 1) % RTL8139_NUM_TX_DESC;
    
    return length;
}

// Receive a packet
int rtl8139_receive_packet(rtl8139_device_t* rtl, void* buffer, size_t max_len) {
    rtl8139_header_t* header = (rtl8139_header_t*)(rtl->rx_buffer + rtl->rx_cur);
    
    // Check if packet is ready
    if (!(header->status & RTL8139_CMD_RX_BUF_EMPTY)) {
        size_t length = header->size - 4; // Subtract CRC
        
        if (length > max_len) {
            length = max_len;
        }
        
        // Copy packet data
        memcpy(buffer, header->data, length);
        
        // Update receive pointer
        rtl->rx_cur = (rtl->rx_cur + length + 4 + 3) & ~3; // Align to 4 bytes
        if (rtl->rx_cur >= RTL8139_RX_BUF_SIZE) {
            rtl->rx_cur -= RTL8139_RX_BUF_SIZE;
        }
        
        // Update CAPR register
        rtl8139_write16(rtl, RTL8139_RX_BUF, rtl->rx_cur - 16);
        
        return length;
    }
    
    return 0;
}

// Read from device
ssize_t rtl8139_read(driver_t* driver, void* buffer, size_t size) {
    rtl8139_device_t* rtl = (rtl8139_device_t*)driver;
    return rtl8139_receive_packet(rtl, buffer, size);
}

// Write to device
ssize_t rtl8139_write(driver_t* driver, const void* buffer, size_t size) {
    rtl8139_device_t* rtl = (rtl8139_device_t*)driver;
    return rtl8139_send_packet(rtl, buffer, size);
}

// Handle device-specific control operations
int rtl8139_ioctl(driver_t* driver, int request, void* arg) {
    rtl8139_device_t* rtl = (rtl8139_device_t*)driver;
    
    switch (request) {
        case 0: // Get MAC address
            if (arg) {
                memcpy(arg, rtl->mac_addr, 6);
                return 0;
            }
            break;
    }
    
    return -1;
}

// Interrupt handler
void rtl8139_handle_interrupt(rtl8139_device_t* rtl) {
    uint16_t status = rtl8139_read16(rtl, RTL8139_ISR);
    
    // Clear interrupts
    rtl8139_write16(rtl, RTL8139_ISR, status);
    
    if (status & RTL8139_ISR_ROK) {
        // Packet received
        // Handle in read operation
    }
    
    if (status & RTL8139_ISR_TOK) {
        // Packet transmitted
        // Nothing to do
    }
    
    if (status & (RTL8139_ISR_RX_ERR | RTL8139_ISR_TX_ERR)) {
        // Handle errors
        kprintf("RTL8139: Transfer error occurred\n");
    }
}
