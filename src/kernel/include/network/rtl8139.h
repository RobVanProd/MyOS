#ifndef NETWORK_RTL8139_H
#define NETWORK_RTL8139_H

#include <stdint.h>
#include "../io.h"
#include "../pci.h"
#include "../driver.h"

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

// Command register bits
#define RTL8139_CMD_RESET          0x10
#define RTL8139_CMD_RX_ENABLE      0x08
#define RTL8139_CMD_TX_ENABLE      0x04
#define RTL8139_CMD_BUFE           0x01

// Interrupt status/mask bits
#define RTL8139_INT_ROK            0x0001  // Receive OK
#define RTL8139_INT_TOK            0x0004  // Transmit OK
#define RTL8139_INT_RER            0x0002  // Receive error
#define RTL8139_INT_TER            0x0008  // Transmit error

// Receive configuration register bits
#define RTL8139_RCR_AAP            0x00000001  // Accept all packets
#define RTL8139_RCR_APM            0x00000002  // Accept physical match
#define RTL8139_RCR_AM             0x00000004  // Accept multicast
#define RTL8139_RCR_AB             0x00000008  // Accept broadcast
#define RTL8139_RCR_WRAP           0x00000080  // Wrap
#define RTL8139_RCR_RBLEN_32K      0x00000000  // 32K receive buffer
#define RTL8139_RCR_MXDMA_UNLIMITED 0x00000700  // Unlimited DMA burst

// Transmit configuration register bits
#define RTL8139_TCR_MXDMA_2048     0x00700000  // 2048 DMA burst
#define RTL8139_TCR_IFG_NORMAL     0x00030000  // Normal interframe gap

// RTL8139 packet header
typedef struct {
    uint16_t status;
    uint16_t length;
} __attribute__((packed)) rtl8139_header_t;

// RTL8139 device structure
typedef struct {
    uint32_t io_base;              // I/O base address
    uint8_t mac_addr[6];           // MAC address
    uint8_t* rx_buffer;            // Receive buffer
    uint32_t rx_buffer_size;       // Size of receive buffer
    uint32_t current_rx_read;      // Current read position in rx buffer
    void* tx_buffers[4];           // Transmit buffers
    uint8_t current_tx_buffer;     // Current transmit buffer
    uint32_t tx_buffer_size;       // Size of each transmit buffer
} rtl8139_device_t;

// RTL8139 driver functions
void rtl8139_init(void);
int rtl8139_detect(void);
void rtl8139_reset(void);
void rtl8139_enable(void);
void rtl8139_disable(void);
void rtl8139_handle_interrupt(void);

// Packet transmission and reception
int rtl8139_transmit(const void* data, uint16_t length);
int rtl8139_receive(void* buffer, uint16_t* length);

// Buffer management
void rtl8139_init_buffers(void);
void rtl8139_free_buffers(void);

// Utility functions
void rtl8139_print_mac(void);
void rtl8139_print_status(void);
const char* rtl8139_get_status_string(uint16_t status);

// Global variables
extern rtl8139_device_t* rtl8139_device;

#endif // NETWORK_RTL8139_H
