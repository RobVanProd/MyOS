#ifndef USB_H
#define USB_H

#include <stdint.h>
#include <stddef.h>
#include "../../kernel/driver.h"

// USB constants
#define USB_MAX_DEVICES      128
#define USB_MAX_ENDPOINTS    32
#define USB_MAX_INTERFACES   32
#define USB_MAX_CONFIGS      8

// USB speeds
#define USB_SPEED_LOW       0
#define USB_SPEED_FULL      1
#define USB_SPEED_HIGH      2
#define USB_SPEED_SUPER     3

// USB request types
#define USB_REQ_GET_STATUS        0x00
#define USB_REQ_CLEAR_FEATURE     0x01
#define USB_REQ_SET_FEATURE       0x03
#define USB_REQ_SET_ADDRESS       0x05
#define USB_REQ_GET_DESCRIPTOR    0x06
#define USB_REQ_SET_DESCRIPTOR    0x07
#define USB_REQ_GET_CONFIG        0x08
#define USB_REQ_SET_CONFIG        0x09
#define USB_REQ_GET_INTERFACE     0x0A
#define USB_REQ_SET_INTERFACE     0x0B
#define USB_REQ_SYNCH_FRAME       0x0C

// USB descriptor types
#define USB_DESC_DEVICE           0x01
#define USB_DESC_CONFIG           0x02
#define USB_DESC_STRING           0x03
#define USB_DESC_INTERFACE        0x04
#define USB_DESC_ENDPOINT         0x05
#define USB_DESC_DEVICE_QUAL      0x06
#define USB_DESC_OTHER_SPEED      0x07
#define USB_DESC_INTERFACE_POWER  0x08
#define USB_DESC_OTG             0x09
#define USB_DESC_DEBUG           0x0A
#define USB_DESC_INTERFACE_ASSOC 0x0B
#define USB_DESC_BOS            0x0F
#define USB_DESC_DEVICE_CAP     0x10

// USB device states
#define USB_STATE_DETACHED      0
#define USB_STATE_ATTACHED      1
#define USB_STATE_POWERED       2
#define USB_STATE_DEFAULT       3
#define USB_STATE_ADDRESS       4
#define USB_STATE_CONFIGURED    5
#define USB_STATE_SUSPENDED     6

// USB transfer types
#define USB_TRANSFER_CONTROL    0
#define USB_TRANSFER_ISOC       1
#define USB_TRANSFER_BULK       2
#define USB_TRANSFER_INT        3

// USB direction
#define USB_DIR_OUT            0
#define USB_DIR_IN             1

// USB standard request structure
typedef struct {
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} __attribute__((packed)) usb_setup_packet_t;

// USB device descriptor
typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bcdUSB;
    uint8_t bDeviceClass;
    uint8_t bDeviceSubClass;
    uint8_t bDeviceProtocol;
    uint8_t bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t iManufacturer;
    uint8_t iProduct;
    uint8_t iSerialNumber;
    uint8_t bNumConfigurations;
} __attribute__((packed)) usb_device_descriptor_t;

// USB configuration descriptor
typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t wTotalLength;
    uint8_t bNumInterfaces;
    uint8_t bConfigurationValue;
    uint8_t iConfiguration;
    uint8_t bmAttributes;
    uint8_t bMaxPower;
} __attribute__((packed)) usb_config_descriptor_t;

// USB interface descriptor
typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndpoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t iInterface;
} __attribute__((packed)) usb_interface_descriptor_t;

// USB endpoint descriptor
typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bEndpointAddress;
    uint8_t bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t bInterval;
} __attribute__((packed)) usb_endpoint_descriptor_t;

// USB string descriptor
typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t wString[];
} __attribute__((packed)) usb_string_descriptor_t;

// USB device structure
typedef struct usb_device {
    uint8_t address;
    uint8_t speed;
    uint8_t state;
    uint8_t max_packet_size;
    usb_device_descriptor_t descriptor;
    usb_config_descriptor_t* config;
    usb_interface_descriptor_t* interface;
    usb_endpoint_descriptor_t endpoints[USB_MAX_ENDPOINTS];
    void* hc_data;
    struct usb_device* next;
} usb_device_t;

// USB transfer structure
typedef struct {
    usb_device_t* dev;
    uint8_t endpoint;
    uint8_t type;
    uint8_t direction;
    void* data;
    size_t length;
    void (*callback)(struct usb_transfer* transfer);
    int status;
    void* hc_data;
} usb_transfer_t;

// USB host controller interface
typedef struct {
    int (*init)(void);
    int (*shutdown)(void);
    int (*detect)(void);
    int (*enumerate)(usb_device_t* dev);
    int (*control)(usb_transfer_t* transfer);
    int (*bulk)(usb_transfer_t* transfer);
    int (*interrupt)(usb_transfer_t* transfer);
    int (*isoc)(usb_transfer_t* transfer);
} usb_hc_t;

// USB driver interface
typedef struct {
    driver_t driver;
    uint16_t vendor_id;
    uint16_t product_id;
    int (*probe)(usb_device_t* dev);
    int (*disconnect)(usb_device_t* dev);
} usb_driver_t;

// USB core functions
void usb_init(void);
void usb_shutdown(void);
int usb_register_driver(usb_driver_t* driver);
int usb_unregister_driver(usb_driver_t* driver);
int usb_register_hc(usb_hc_t* hc);
int usb_enumerate_device(usb_device_t* dev);
int usb_set_address(usb_device_t* dev, uint8_t address);
int usb_set_configuration(usb_device_t* dev, uint8_t config);
int usb_get_descriptor(usb_device_t* dev, uint8_t type, uint8_t index, void* data, size_t length);

// USB transfer functions
usb_transfer_t* usb_alloc_transfer(void);
void usb_free_transfer(usb_transfer_t* transfer);
int usb_submit_transfer(usb_transfer_t* transfer);
int usb_cancel_transfer(usb_transfer_t* transfer);

// USB utility functions
const char* usb_speed_string(uint8_t speed);
const char* usb_state_string(uint8_t state);
const char* usb_class_string(uint8_t class);
void usb_dump_device(usb_device_t* dev);
void usb_dump_config(usb_config_descriptor_t* config);
void usb_dump_interface(usb_interface_descriptor_t* interface);
void usb_dump_endpoint(usb_endpoint_descriptor_t* endpoint);

#endif 