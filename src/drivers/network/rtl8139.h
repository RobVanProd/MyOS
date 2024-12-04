#ifndef RTL8139_H
#define RTL8139_H

#include "../../kernel/driver.h"
#include "../../kernel/pci.h"

// RTL8139 vendor and device IDs
#define RTL8139_VENDOR_ID    0x10EC
#define RTL8139_DEVICE_ID    0x8139

// RTL8139 registers
#define RTL8139_IDR0         0x00    // MAC address
#define RTL8139_MAR0         0x08    // Multicast filter
#define RTL8139_TSD0         0x10    // Transmit status (4 registers)
#define RTL8139_TSAD0        0x20    // Transmit start address (4 registers)
#define RTL8139_RBSTART      0x30    // Receive buffer start address
#define RTL8139_CMD          0x37    // Command register
#define RTL8139_CAPR         0x38    // Current address of packet read
#define RTL8139_IMR          0x3C    // Interrupt mask register
#define RTL8139_ISR          0x3E    // Interrupt status register
#define RTL8139_TCR          0x40    // Transmit configuration register
#define RTL8139_RCR          0x44    // Receive configuration register
#define RTL8139_CONFIG1      0x52    // Configuration register 1

// RTL8139 commands
#define RTL8139_CMD_RESET    0x10
#define RTL8139_CMD_RX_ENABLE 0x08
#define RTL8139_CMD_TX_ENABLE 0x04

// RTL8139 interrupt status bits
#define RTL8139_ISR_ROK      0x0001  // Receive OK
#define RTL8139_ISR_TOK      0x0004  // Transmit OK

// RTL8139 receive config
#define RTL8139_RCR_AAP      (1 << 0)  // Accept all packets
#define RTL8139_RCR_APM      (1 << 1)  // Accept physical match
#define RTL8139_RCR_AM       (1 << 2)  // Accept multicast
#define RTL8139_RCR_AB       (1 << 3)  // Accept broadcast
#define RTL8139_RCR_WRAP     (1 << 7)  // Wrap around buffer
#define RTL8139_RCR_RBLEN_32K (0 << 11)// 32K receive buffer
#define RTL8139_RCR_MXDMA_UNLIMITED (7 << 8) // Unlimited DMA burst

// RTL8139 transmit config
#define RTL8139_TCR_MXDMA_2048 (7 << 8)
#define RTL8139_TCR_IFG_STD  (3 << 24)

// RTL8139 packet header
typedef struct {
    uint16_t status;
    uint16_t size;
} __attribute__((packed)) rtl8139_header_t;

// RTL8139 transmit status
#define RTL8139_TX_STATUS_OWN    (1 << 13)
#define RTL8139_TX_STATUS_TOK    (1 << 15)

// RTL8139 buffer sizes
#define RTL8139_RX_BUFFER_SIZE   32768
#define RTL8139_TX_BUFFER_SIZE   1536
#define RTL8139_NUM_TX_DESC      4

// RTL8139 device structure
typedef struct {
    driver_t driver;            // Base driver
    uint32_t io_base;          // I/O base address
    uint32_t mem_base;         // Memory base address
    uint8_t mac_addr[6];       // MAC address
    uint8_t* rx_buffer;        // Receive buffer
    uint8_t* tx_buffer[4];     // Transmit buffers
    uint32_t tx_current;       // Current transmit descriptor
    uint32_t rx_current;       // Current receive offset
    uint32_t irq;             // IRQ number
} rtl8139_driver_t;

// RTL8139 driver functions
int rtl8139_init(driver_t* driver);
int rtl8139_cleanup(driver_t* driver);
int rtl8139_read(driver_t* driver, void* buffer, size_t size, uint32_t offset);
int rtl8139_write(driver_t* driver, const void* buffer, size_t size, uint32_t offset);
int rtl8139_ioctl(driver_t* driver, uint32_t cmd, void* arg);

// RTL8139 utility functions
void rtl8139_reset(rtl8139_driver_t* driver);
void rtl8139_init_rings(rtl8139_driver_t* driver);
void rtl8139_enable(rtl8139_driver_t* driver);
void rtl8139_handle_interrupt(rtl8139_driver_t* driver);
int rtl8139_transmit_packet(rtl8139_driver_t* driver, const void* data, size_t length);
int rtl8139_receive_packet(rtl8139_driver_t* driver, void* buffer, size_t max_length);

// RTL8139 IOCTL commands
#define IOCTL_RTL8139_GET_MAC          0x2000
#define IOCTL_RTL8139_SET_PROMISCUOUS  0x2001
#define IOCTL_RTL8139_GET_STATS        0x2002

// RTL8139 statistics
typedef struct {
    uint64_t rx_packets;
    uint64_t tx_packets;
    uint64_t rx_bytes;
    uint64_t tx_bytes;
    uint32_t rx_errors;
    uint32_t tx_errors;
    uint32_t rx_dropped;
    uint32_t tx_dropped;
    uint32_t multicast;
    uint32_t collisions;
} rtl8139_stats_t;

#endif 