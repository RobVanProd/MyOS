#ifndef RTL8139_H
#define RTL8139_H

#include <driver.h>
#include <pci.h>
#include <network.h>

// RTL8139 vendor and device IDs
#define RTL8139_VENDOR_ID   0x10EC
#define RTL8139_DEVICE_ID   0x8139

// RTL8139 registers
#define RTL8139_IDR0       0x00    // MAC address
#define RTL8139_MAR0       0x08    // Multicast filter
#define RTL8139_TSD0       0x10    // Transmit status (4 32bit regs)
#define RTL8139_TSAD0      0x20    // Transmit start address (4 32bit regs)
#define RTL8139_RBSTART    0x30    // Receive buffer start address
#define RTL8139_CMD        0x37    // Command register
#define RTL8139_CAPR       0x38    // Current address of packet read
#define RTL8139_IMR        0x3C    // Interrupt mask register
#define RTL8139_ISR        0x3E    // Interrupt status register
#define RTL8139_TCR        0x40    // Transmit configuration register
#define RTL8139_RCR        0x44    // Receive configuration register
#define RTL8139_CONFIG1    0x52    // Configuration register 1

// RTL8139 commands
#define RTL8139_CMD_RESET  0x10
#define RTL8139_CMD_RX_EN  0x08
#define RTL8139_CMD_TX_EN  0x04

// RTL8139 interrupt status bits
#define RTL8139_INT_ROK    0x0001  // Receive OK
#define RTL8139_INT_TOK    0x0004  // Transmit OK

// RTL8139 receive configuration
#define RTL8139_RCR_AAP    0x00000001  // Accept all packets
#define RTL8139_RCR_APM    0x00000002  // Accept physical match
#define RTL8139_RCR_AM     0x00000004  // Accept multicast
#define RTL8139_RCR_AB     0x00000008  // Accept broadcast
#define RTL8139_RCR_WRAP   0x00000080  // Wrap around buffer
#define RTL8139_RCR_RBLEN  0x00000600  // Buffer length
#define RTL8139_RCR_MXDMA  0x00000700  // Max DMA burst

// RTL8139 transmit configuration
#define RTL8139_TCR_MXDMA  0x00000700  // Max DMA burst

// RTL8139 receive buffer
#define RTL8139_RX_BUF_SIZE    32768
#define RTL8139_RX_BUF_MASK    (RTL8139_RX_BUF_SIZE - 1)
#define RTL8139_RX_BUF_PAD     16
#define RTL8139_RX_BUF_WRAP    0

// RTL8139 transmit buffer
#define RTL8139_TX_BUF_SIZE    1536
#define RTL8139_TX_BUF_COUNT   4

// RTL8139 packet header
typedef struct {
    uint16_t status;
    uint16_t size;
} __attribute__((packed)) rtl8139_header_t;

// RTL8139 device structure
typedef struct {
    driver_t driver;            // Base driver
    uint16_t io_base;          // I/O base address
    uint8_t* rx_buffer;        // Receive buffer
    uint8_t* tx_buffer[4];     // Transmit buffers
    uint32_t tx_current;       // Current transmit buffer
    uint32_t rx_current;       // Current receive offset
} rtl8139_device_t;

// RTL8139 driver functions
driver_t* create_rtl8139_driver(void);

#endif