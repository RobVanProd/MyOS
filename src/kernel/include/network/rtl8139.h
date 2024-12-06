#ifndef NETWORK_RTL8139_H
#define NETWORK_RTL8139_H

#include <stdint.h>
#include "../io.h"
#include "../pci.h"
#include "../driver.h"
#include "../network.h"

// RTL8139 vendor and device IDs
#define RTL8139_VENDOR_ID 0x10EC
#define RTL8139_DEVICE_ID 0x8139

// RTL8139 registers
#define RTL8139_IDR0               0x00    // MAC address
#define RTL8139_MAR0               0x08    // Multicast filter
#define RTL8139_TSD0               0x10    // Transmit status (4 registers)
#define RTL8139_TSAD0              0x20    // Transmit start address (4 registers)
#define RTL8139_RBSTART            0x30    // Receive buffer start address
#define RTL8139_CMD                0x37    // Command register
#define RTL8139_CAPR               0x38    // Current address of packet read
#define RTL8139_IMR                0x3C    // Interrupt mask register
#define RTL8139_ISR                0x3E    // Interrupt status register
#define RTL8139_TCR                0x40    // Transmit configuration register
#define RTL8139_RCR                0x44    // Receive configuration register
#define RTL8139_CONFIG1            0x52    // Configuration register 1
#define RTL8139_RX_MISSED          0x4C    // Receive missed packets
#define RTL8139_TIMER              0x40    // Timer

// Command register bits
#define RTL8139_CMD_RESET          0x10
#define RTL8139_CMD_RX_ENABLE      0x08
#define RTL8139_CMD_TX_ENABLE      0x04
#define RTL8139_CMD_BUFE           0x01
#define RTL8139_CMD_TX_DONE        0x02
#define RTL8139_CMD_RX_BUF_EMPTY   0x01

// Interrupt status/mask bits
#define RTL8139_INT_ROK            0x0001  // Receive OK
#define RTL8139_INT_TOK            0x0004  // Transmit OK
#define RTL8139_INT_RER            0x0002  // Receive error
#define RTL8139_INT_TER            0x0008  // Transmit error
#define RTL8139_INT_RX_BUFF        0x0010  // Receive buffer
#define RTL8139_INT_LINK           0x0020  // Link
#define RTL8139_INT_FIFO           0x0040  // FIFO
#define RTL8139_INT_TIMEOUT        0x4000  // Timeout
#define RTL8139_INT_SERR           0x8000  // System error

// Receive configuration register bits
#define RTL8139_RCR_AAP            0x00000001  // Accept all packets
#define RTL8139_RCR_APM            0x00000002  // Accept physical match
#define RTL8139_RCR_AM             0x00000004  // Accept multicast
#define RTL8139_RCR_AB             0x00000008  // Accept broadcast
#define RTL8139_RCR_WRAP           0x00000080  // Wrap
#define RTL8139_RCR_MXDMA          0x00000700  // Max DMA burst size mask
#define RTL8139_RCR_MXDMA_16       0x00000000  // 16 bytes
#define RTL8139_RCR_MXDMA_32       0x00000100  // 32 bytes
#define RTL8139_RCR_MXDMA_64       0x00000200  // 64 bytes
#define RTL8139_RCR_MXDMA_128      0x00000300  // 128 bytes
#define RTL8139_RCR_MXDMA_256      0x00000400  // 256 bytes
#define RTL8139_RCR_MXDMA_512      0x00000500  // 512 bytes
#define RTL8139_RCR_MXDMA_1K       0x00000600  // 1K bytes
#define RTL8139_RCR_MXDMA_UNLIM    0x00000700  // Unlimited
#define RTL8139_RX_CONFIG_AR       0x00000010  // Accept runt

// Transmit configuration register bits
#define RTL8139_TCR_MXDMA_2048     0x00700000  // 2048 DMA burst
#define RTL8139_TCR_IFG_NORMAL     0x00030000  // Normal interframe gap
#define RTL8139_TX_CONFIG_CRC      0x00010000  // CRC
#define RTL8139_TX_CONFIG_IFG      0x03000000  // Interframe gap
#define RTL8139_TX_CONFIG_LOOP     0x60000000  // Loop

// Buffer sizes
#define RTL8139_RX_BUF_SIZE        8192      // 8K
#define RTL8139_RX_BUF_PAD         16          // Padding for alignment
#define RTL8139_TX_BUF_SIZE        1536      // 1.5K
#define RTL8139_TX_BUF_COUNT       4           // Number of transmit buffers

// RTL8139 packet header
typedef struct {
    uint16_t status;
    uint16_t size;
    uint8_t data[0];
} __attribute__((packed)) rtl8139_header_t;

// RTL8139 device structure
typedef struct rtl8139_device {
    driver_t driver;               // Base driver structure
    uint8_t bus;                  // PCI bus number
    uint8_t slot;                 // PCI slot number
    uint8_t func;                 // PCI function number
    uint32_t io_base;             // I/O base address
    uint32_t mem_base;            // Memory base address
    uint8_t mac_addr[6];          // MAC address
    uint8_t* rx_buffer;           // Receive buffer
    uint8_t* tx_buffer[4];        // Transmit buffers
    uint32_t tx_cur;              // Current transmit buffer
    uint32_t rx_cur;              // Current read position in rx buffer
    uint32_t irq;                 // IRQ
} rtl8139_device_t;

// RTL8139 driver functions
int rtl8139_init(driver_t* driver);
int rtl8139_cleanup(driver_t* driver);
ssize_t rtl8139_read(driver_t* driver, void* buffer, size_t size);
ssize_t rtl8139_write(driver_t* driver, const void* buffer, size_t size);
int rtl8139_ioctl(driver_t* driver, int request, void* arg);
void rtl8139_handle_interrupt(rtl8139_device_t* rtl);
int rtl8139_init_device(rtl8139_device_t* rtl);
int rtl8139_send_packet(rtl8139_device_t* rtl, const void* data, size_t length);
int rtl8139_receive_packet(rtl8139_device_t* rtl, void* buffer, size_t max_len);

// Global variables
extern rtl8139_device_t* rtl8139_device;

#endif // NETWORK_RTL8139_H
