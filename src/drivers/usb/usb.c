#include "usb.h"
#include "../../kernel/memory.h"
#include <string.h>
#include <stdio.h>

// Global variables
static usb_device_t* usb_devices = NULL;
static usb_driver_t* usb_drivers = NULL;
static usb_hc_t* usb_hc = NULL;

// USB device management
static usb_device_t* usb_alloc_device(void) {
    usb_device_t* dev = kmalloc(sizeof(usb_device_t));
    if (!dev) return NULL;
    
    memset(dev, 0, sizeof(usb_device_t));
    dev->state = USB_STATE_DETACHED;
    dev->next = usb_devices;
    usb_devices = dev;
    
    return dev;
}

static void usb_free_device(usb_device_t* dev) {
    if (!dev) return;
    
    // Remove from device list
    usb_device_t** pp = &usb_devices;
    while (*pp) {
        if (*pp == dev) {
            *pp = dev->next;
            break;
        }
        pp = &(*pp)->next;
    }
    
    // Free configuration descriptor
    if (dev->config) {
        kfree(dev->config);
    }
    
    kfree(dev);
}

// USB driver management
int usb_register_driver(usb_driver_t* driver) {
    if (!driver) return -1;
    
    // Add to driver list
    driver->driver.next = (driver_t*)usb_drivers;
    usb_drivers = driver;
    
    return 0;
}

int usb_unregister_driver(usb_driver_t* driver) {
    if (!driver) return -1;
    
    // Remove from driver list
    usb_driver_t** pp = &usb_drivers;
    while (*pp) {
        if (*pp == driver) {
            *pp = (usb_driver_t*)driver->driver.next;
            return 0;
        }
        pp = (usb_driver_t**)&(*pp)->driver.next;
    }
    
    return -1;
}

// USB host controller management
int usb_register_hc(usb_hc_t* hc) {
    if (!hc) return -1;
    if (usb_hc) return -1;  // Only one HC supported for now
    
    usb_hc = hc;
    return 0;
}

// USB enumeration
int usb_enumerate_device(usb_device_t* dev) {
    if (!dev || !usb_hc) return -1;
    
    // Get device descriptor
    if (usb_get_descriptor(dev, USB_DESC_DEVICE, 0, &dev->descriptor, sizeof(dev->descriptor)) < 0) {
        return -1;
    }
    
    // Set device address
    if (usb_set_address(dev, dev->address) < 0) {
        return -1;
    }
    
    // Get full configuration descriptor
    usb_config_descriptor_t config;
    if (usb_get_descriptor(dev, USB_DESC_CONFIG, 0, &config, sizeof(config)) < 0) {
        return -1;
    }
    
    // Allocate and get full configuration
    dev->config = kmalloc(config.wTotalLength);
    if (!dev->config) {
        return -1;
    }
    
    if (usb_get_descriptor(dev, USB_DESC_CONFIG, 0, dev->config, config.wTotalLength) < 0) {
        kfree(dev->config);
        dev->config = NULL;
        return -1;
    }
    
    // Set configuration
    if (usb_set_configuration(dev, dev->config->bConfigurationValue) < 0) {
        return -1;
    }
    
    // Find and probe matching driver
    usb_driver_t* driver = usb_drivers;
    while (driver) {
        if (driver->vendor_id == dev->descriptor.idVendor &&
            driver->product_id == dev->descriptor.idProduct) {
            if (driver->probe(dev) == 0) {
                return 0;
            }
        }
        driver = (usb_driver_t*)driver->driver.next;
    }
    
    return -1;
}

// USB device requests
int usb_set_address(usb_device_t* dev, uint8_t address) {
    usb_setup_packet_t setup = {
        .bmRequestType = 0x00,
        .bRequest = USB_REQ_SET_ADDRESS,
        .wValue = address,
        .wIndex = 0,
        .wLength = 0
    };
    
    usb_transfer_t transfer = {
        .dev = dev,
        .endpoint = 0,
        .type = USB_TRANSFER_CONTROL,
        .direction = USB_DIR_OUT,
        .data = NULL,
        .length = 0
    };
    
    if (usb_hc->control(&transfer) < 0) {
        return -1;
    }
    
    dev->address = address;
    dev->state = USB_STATE_ADDRESS;
    return 0;
}

int usb_set_configuration(usb_device_t* dev, uint8_t config) {
    usb_setup_packet_t setup = {
        .bmRequestType = 0x00,
        .bRequest = USB_REQ_SET_CONFIG,
        .wValue = config,
        .wIndex = 0,
        .wLength = 0
    };
    
    usb_transfer_t transfer = {
        .dev = dev,
        .endpoint = 0,
        .type = USB_TRANSFER_CONTROL,
        .direction = USB_DIR_OUT,
        .data = NULL,
        .length = 0
    };
    
    if (usb_hc->control(&transfer) < 0) {
        return -1;
    }
    
    dev->state = USB_STATE_CONFIGURED;
    return 0;
}

int usb_get_descriptor(usb_device_t* dev, uint8_t type, uint8_t index, void* data, size_t length) {
    usb_setup_packet_t setup = {
        .bmRequestType = 0x80,
        .bRequest = USB_REQ_GET_DESCRIPTOR,
        .wValue = (type << 8) | index,
        .wIndex = 0,
        .wLength = length
    };
    
    usb_transfer_t transfer = {
        .dev = dev,
        .endpoint = 0,
        .type = USB_TRANSFER_CONTROL,
        .direction = USB_DIR_IN,
        .data = data,
        .length = length
    };
    
    return usb_hc->control(&transfer);
}

// USB transfer management
usb_transfer_t* usb_alloc_transfer(void) {
    usb_transfer_t* transfer = kmalloc(sizeof(usb_transfer_t));
    if (transfer) {
        memset(transfer, 0, sizeof(usb_transfer_t));
    }
    return transfer;
}

void usb_free_transfer(usb_transfer_t* transfer) {
    if (transfer) {
        kfree(transfer);
    }
}

int usb_submit_transfer(usb_transfer_t* transfer) {
    if (!transfer || !usb_hc) return -1;
    
    switch (transfer->type) {
        case USB_TRANSFER_CONTROL:
            return usb_hc->control(transfer);
        case USB_TRANSFER_BULK:
            return usb_hc->bulk(transfer);
        case USB_TRANSFER_INT:
            return usb_hc->interrupt(transfer);
        case USB_TRANSFER_ISOC:
            return usb_hc->isoc(transfer);
        default:
            return -1;
    }
}

int usb_cancel_transfer(usb_transfer_t* transfer) {
    // TODO: Implement transfer cancellation
    return -1;
}

// USB utility functions
const char* usb_speed_string(uint8_t speed) {
    switch (speed) {
        case USB_SPEED_LOW:    return "Low Speed";
        case USB_SPEED_FULL:   return "Full Speed";
        case USB_SPEED_HIGH:   return "High Speed";
        case USB_SPEED_SUPER:  return "Super Speed";
        default:              return "Unknown Speed";
    }
}

const char* usb_state_string(uint8_t state) {
    switch (state) {
        case USB_STATE_DETACHED:   return "Detached";
        case USB_STATE_ATTACHED:   return "Attached";
        case USB_STATE_POWERED:    return "Powered";
        case USB_STATE_DEFAULT:    return "Default";
        case USB_STATE_ADDRESS:    return "Address";
        case USB_STATE_CONFIGURED: return "Configured";
        case USB_STATE_SUSPENDED:  return "Suspended";
        default:                  return "Unknown State";
    }
}

const char* usb_class_string(uint8_t class) {
    switch (class) {
        case 0x00: return "Device";
        case 0x01: return "Audio";
        case 0x02: return "CDC";
        case 0x03: return "HID";
        case 0x05: return "Physical";
        case 0x06: return "Image";
        case 0x07: return "Printer";
        case 0x08: return "Mass Storage";
        case 0x09: return "Hub";
        case 0x0A: return "CDC-Data";
        case 0x0B: return "Smart Card";
        case 0x0D: return "Content Security";
        case 0x0E: return "Video";
        case 0x0F: return "Personal Healthcare";
        case 0xDC: return "Diagnostic";
        case 0xE0: return "Wireless Controller";
        case 0xEF: return "Miscellaneous";
        case 0xFE: return "Application Specific";
        case 0xFF: return "Vendor Specific";
        default:  return "Unknown Class";
    }
}

void usb_dump_device(usb_device_t* dev) {
    if (!dev) return;
    
    printf("USB Device:\n");
    printf("  Address: %d\n", dev->address);
    printf("  Speed: %s\n", usb_speed_string(dev->speed));
    printf("  State: %s\n", usb_state_string(dev->state));
    printf("  Vendor ID: 0x%04X\n", dev->descriptor.idVendor);
    printf("  Product ID: 0x%04X\n", dev->descriptor.idProduct);
    printf("  Class: %s (0x%02X)\n", usb_class_string(dev->descriptor.bDeviceClass),
           dev->descriptor.bDeviceClass);
    printf("  Configurations: %d\n", dev->descriptor.bNumConfigurations);
}

void usb_dump_config(usb_config_descriptor_t* config) {
    if (!config) return;
    
    printf("Configuration Descriptor:\n");
    printf("  Total Length: %d\n", config->wTotalLength);
    printf("  Interfaces: %d\n", config->bNumInterfaces);
    printf("  Configuration Value: %d\n", config->bConfigurationValue);
    printf("  Attributes: 0x%02X\n", config->bmAttributes);
    printf("  Max Power: %dmA\n", config->bMaxPower * 2);
}

void usb_dump_interface(usb_interface_descriptor_t* interface) {
    if (!interface) return;
    
    printf("Interface Descriptor:\n");
    printf("  Number: %d\n", interface->bInterfaceNumber);
    printf("  Alternate Setting: %d\n", interface->bAlternateSetting);
    printf("  Endpoints: %d\n", interface->bNumEndpoints);
    printf("  Class: %s (0x%02X)\n", usb_class_string(interface->bInterfaceClass),
           interface->bInterfaceClass);
    printf("  Subclass: 0x%02X\n", interface->bInterfaceSubClass);
    printf("  Protocol: 0x%02X\n", interface->bInterfaceProtocol);
}

void usb_dump_endpoint(usb_endpoint_descriptor_t* endpoint) {
    if (!endpoint) return;
    
    printf("Endpoint Descriptor:\n");
    printf("  Address: 0x%02X\n", endpoint->bEndpointAddress);
    printf("  Attributes: 0x%02X\n", endpoint->bmAttributes);
    printf("  Max Packet Size: %d\n", endpoint->wMaxPacketSize);
    printf("  Interval: %d\n", endpoint->bInterval);
}

// USB initialization
void usb_init(void) {
    // Initialize USB subsystem
    usb_devices = NULL;
    usb_drivers = NULL;
    usb_hc = NULL;
    
    // Initialize host controller if available
    if (usb_hc && usb_hc->init) {
        usb_hc->init();
    }
}

void usb_shutdown(void) {
    // Free all devices
    while (usb_devices) {
        usb_device_t* dev = usb_devices;
        usb_devices = dev->next;
        usb_free_device(dev);
    }
    
    // Shutdown host controller
    if (usb_hc && usb_hc->shutdown) {
        usb_hc->shutdown();
    }
    
    usb_hc = NULL;
} 