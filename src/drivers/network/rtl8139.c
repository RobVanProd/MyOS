#include <network/rtl8139.h>
#include <io.h>
#include <memory.h>
#include <string.h>
#include <kheap.h>
#include <network.h>
#include <pci.h>

// RTL8139 driver instance
static rtl8139_device_t rtl8139_driver;

// Receive buffer
static uint8_t* rx_buffer;
static uint32_t rx_buffer_offset;

// Transmit buffers
static uint8_t* tx_buffers[RTL8139_TX_BUF_COUNT];
static uint32_t tx_current = 0;

// Initialize RTL8139 device
int rtl8139_init_device(rtl8139_device_t* rtl) {
    // Reset the device
    outb(rtl->io_base + RTL8139_CMD, RTL8139_CMD_RESET);
    while (inb(rtl->io_base + RTL8139_CMD) & RTL8139_CMD_RESET);

    // Allocate receive buffer
    rx_buffer = kmalloc_aligned(RTL8139_RX_BUF_SIZE + RTL8139_RX_BUF_PAD);
    if (!rx_buffer) return -1;

    // Allocate transmit buffers
    for (int i = 0; i < RTL8139_TX_BUF_COUNT; i++) {
        tx_buffers[i] = kmalloc_aligned(RTL8139_TX_BUF_SIZE);
        if (!tx_buffers[i]) {
            // Free previously allocated buffers
            for (int j = 0; j < i; j++) {
                kfree(tx_buffers[j]);
            }
            kfree(rx_buffer);
            return -1;
        }
    }

    // Set receive buffer address
    outl(rtl->io_base + RTL8139_RBSTART, (uint32_t)rx_buffer);

    // Enable transmitter and receiver
    outb(rtl->io_base + RTL8139_CMD, RTL8139_CMD_RX_ENABLE | RTL8139_CMD_TX_ENABLE);

    // Set IMR
    outw(rtl->io_base + RTL8139_IMR, RTL8139_INT_ROK | RTL8139_INT_TOK);

    // Configure receive buffer
    outl(rtl->io_base + RTL8139_RCR,
         RTL8139_RCR_APM |      // Accept physical match
         RTL8139_RCR_AB |       // Accept broadcast
         RTL8139_RCR_AM |       // Accept multicast
         RTL8139_RCR_WRAP |     // Wrap
         RTL8139_RCR_MXDMA_UNLIM); // Unlimited DMA burst

    return 0;
}

// Initialize RTL8139 driver
int rtl8139_init(driver_t* driver) {
    rtl8139_device_t* rtl = (rtl8139_device_t*)driver;

    // Find RTL8139 PCI device
    uint8_t bus, slot, func;
    if (pci_find_device_by_id(RTL8139_VENDOR_ID, RTL8139_DEVICE_ID, &bus, &slot, &func) != 0) {
        return -1;
    }

    // Store PCI device info
    rtl->bus = bus;
    rtl->slot = slot;
    rtl->func = func;

    // Enable bus mastering
    uint16_t command = pci_read_config(bus, slot, func, PCI_COMMAND);
    command |= PCI_COMMAND_MASTER;
    pci_write_config(bus, slot, func, PCI_COMMAND, command);

    // Get I/O base address
    rtl->io_base = pci_read_config(bus, slot, func, PCI_BAR0) & ~0x3;

    // Initialize the device
    return rtl8139_init_device(rtl);
}

// Cleanup RTL8139 driver
int rtl8139_cleanup(driver_t* driver) {
    rtl8139_device_t* rtl = (rtl8139_device_t*)driver;

    // Disable transmitter and receiver
    outb(rtl->io_base + RTL8139_CMD, 0);

    // Free receive buffer
    if (rx_buffer) {
        kfree(rx_buffer);
        rx_buffer = NULL;
    }

    // Free transmit buffers
    for (int i = 0; i < RTL8139_TX_BUF_COUNT; i++) {
        if (tx_buffers[i]) {
            kfree(tx_buffers[i]);
            tx_buffers[i] = NULL;
        }
    }

    return 0;
}

// Handle receive
void rtl8139_handle_receive(rtl8139_device_t* rtl) {
    uint16_t rx_status;
    uint16_t rx_size;
    rtl8139_header_t* header;

    while (!(inb(rtl->io_base + RTL8139_CMD) & RTL8139_CMD_RX_ENABLE)) {
        header = (rtl8139_header_t*)(rx_buffer + rx_buffer_offset);
        rx_status = header->status;
        rx_size = header->size;

        if (rx_status & RTL8139_INT_ROK) {
            // Process received packet
            // TODO: Forward packet to network stack

            // Update buffer offset
            rx_buffer_offset = (rx_buffer_offset + rx_size + sizeof(rtl8139_header_t) + 3) & ~3;
            if (rx_buffer_offset >= RTL8139_RX_BUF_SIZE) {
                rx_buffer_offset -= RTL8139_RX_BUF_SIZE;
            }

            // Update CAPR
            outw(rtl->io_base + RTL8139_CAPR, rx_buffer_offset - 16);
        }
    }
}

// Handle transmit
void rtl8139_handle_transmit(rtl8139_device_t* rtl) {
    uint32_t tx_status;

    for (int i = 0; i < RTL8139_TX_BUF_COUNT; i++) {
        tx_status = inl(rtl->io_base + RTL8139_TSD0 + (i * 4));
        if (tx_status & RTL8139_INT_TOK) {
            // Packet transmitted successfully
            // TODO: Notify network stack
        }
    }
}

// Interrupt handler
void rtl8139_handle_interrupt(rtl8139_device_t* rtl) {
    uint16_t status = inw(rtl->io_base + RTL8139_ISR);

    if (status & RTL8139_INT_ROK) {
        rtl8139_handle_receive(rtl);
    }

    if (status & RTL8139_INT_TOK) {
        rtl8139_handle_transmit(rtl);
    }

    // Clear interrupts
    outw(rtl->io_base + RTL8139_ISR, status);
}

// Transmit packet
int rtl8139_transmit_packet(rtl8139_device_t* rtl, const void* data, size_t length) {
    if (length > RTL8139_TX_BUF_SIZE) {
        return -1;
    }

    // Wait for current buffer to be free
    while (inl(rtl->io_base + RTL8139_TSD0 + (tx_current * 4)) & 0x2000);

    // Copy data to transmit buffer
    memcpy(tx_buffers[tx_current], data, length);

    // Start transmission
    outl(rtl->io_base + RTL8139_TSAD0 + (tx_current * 4), (uint32_t)tx_buffers[tx_current]);
    outl(rtl->io_base + RTL8139_TSD0 + (tx_current * 4), length);

    // Move to next buffer
    tx_current = (tx_current + 1) % RTL8139_TX_BUF_COUNT;

    return 0;
}

// Receive packet
int rtl8139_receive_packet(rtl8139_device_t* rtl, void* buffer, size_t max_length) {
    rtl8139_header_t* header;
    uint16_t rx_status;
    uint16_t rx_size;

    if (!buffer || !max_length) {
        return -1;
    }

    header = (rtl8139_header_t*)(rx_buffer + rx_buffer_offset);
    rx_status = header->status;
    rx_size = header->size;

    if (!(rx_status & RTL8139_INT_ROK)) {
        return 0;  // No packet available
    }

    if (rx_size > max_length) {
        return -1;  // Buffer too small
    }

    // Copy packet data
    memcpy(buffer, rx_buffer + rx_buffer_offset + sizeof(rtl8139_header_t), rx_size);

    // Update buffer offset
    rx_buffer_offset = (rx_buffer_offset + rx_size + sizeof(rtl8139_header_t) + 3) & ~3;
    if (rx_buffer_offset >= RTL8139_RX_BUF_SIZE) {
        rx_buffer_offset -= RTL8139_RX_BUF_SIZE;
    }

    // Update CAPR
    outw(rtl->io_base + RTL8139_CAPR, rx_buffer_offset - 16);

    return rx_size;
}

// IOCTL operations
int rtl8139_ioctl(driver_t* driver, int request, void* arg) {
    rtl8139_device_t* rtl = (rtl8139_device_t*)driver;

    switch (request) {
        case NETWORK_IOCTL_GET_MAC: {
            uint8_t* mac = (uint8_t*)arg;
            for (int i = 0; i < 6; i++) {
                mac[i] = inb(rtl->io_base + RTL8139_IDR0 + i);
            }
            return 0;
        }
        case NETWORK_IOCTL_SET_MAC: {
            uint8_t* mac = (uint8_t*)arg;
            // Disable Rx and Tx before changing MAC
            outb(rtl->io_base + RTL8139_CMD, 0);
            for (int i = 0; i < 6; i++) {
                outb(rtl->io_base + RTL8139_IDR0 + i, mac[i]);
            }
            // Re-enable Rx and Tx
            outb(rtl->io_base + RTL8139_CMD, RTL8139_CMD_RX_ENABLE | RTL8139_CMD_TX_ENABLE);
            return 0;
        }
        default:
            return -1;
    }
}

// Create and register RTL8139 driver
driver_t* create_rtl8139_driver(void) {
    // Initialize driver structure
    memset(&rtl8139_driver, 0, sizeof(rtl8139_driver));
    
    // Set up driver operations
    strcpy(rtl8139_driver.driver.name, "rtl8139");
    rtl8139_driver.driver.type = DRIVER_TYPE_NETWORK;
    rtl8139_driver.driver.init = rtl8139_init;
    rtl8139_driver.driver.cleanup = rtl8139_cleanup;
    rtl8139_driver.driver.ioctl = (driver_ioctl_fn)rtl8139_ioctl;

    return &rtl8139_driver.driver;
}