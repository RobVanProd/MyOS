#ifndef RTL8139_H
#define RTL8139_H

#include <stdint.h>
#include "driver.h"

// RTL8139 registers
#define RTL8139_REG_MAC0         0x00
#define RTL8139_REG_MAR0         0x08
#define RTL8139_REG_TxStatus0    0x10
#define RTL8139_REG_TxAddr0      0x20
#define RTL8139_REG_RxBuf        0x30
#define RTL8139_REG_RxEarlyCnt   0x34
#define RTL8139_REG_RxEarlyStatus 0x36
#define RTL8139_REG_ChipCmd      0x37
#define RTL8139_REG_RxBufPtr     0x38
#define RTL8139_REG_RxBufAddr    0x3C
#define RTL8139_REG_IntrMask     0x3C
#define RTL8139_REG_IntrStatus   0x3E
#define RTL8139_REG_TxConfig     0x40
#define RTL8139_REG_RxConfig     0x44

// RTL8139 commands
#define RTL8139_CMD_RESET        0x10
#define RTL8139_CMD_RX_ENABLE    0x08
#define RTL8139_CMD_TX_ENABLE    0x04

// Function declarations
void rtl8139_initialize(void);
int rtl8139_send_packet(const uint8_t* data, uint16_t length);
int rtl8139_receive_packet(uint8_t* buffer, uint16_t* length);
driver_t* rtl8139_get_driver(void);

#endif // RTL8139_H
