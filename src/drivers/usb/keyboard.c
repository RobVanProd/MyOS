#include "keyboard.h"
#include "../../kernel/memory.h"
#include <string.h>
#include <stdio.h>

// USB HID keyboard driver instance
static usb_driver_t usb_keyboard_driver;
static usb_keyboard_t* usb_keyboards[USB_MAX_DEVICES];
static int num_keyboards = 0;

// USB HID keyboard key names
static const char* key_names[] = {
    "NONE",    "ERR_OVF", "POST_FAIL", "ERR_UNDEF", "A",      "B",      "C",      "D",
    "E",       "F",       "G",         "H",         "I",      "J",      "K",      "L",
    "M",       "N",       "O",         "P",         "Q",      "R",      "S",      "T",
    "U",       "V",       "W",         "X",         "Y",      "Z",      "1",      "2",
    "3",       "4",       "5",         "6",         "7",      "8",      "9",      "0",
    "ENTER",   "ESC",     "BSPACE",    "TAB",       "SPACE",  "MINUS",  "EQUAL",  "LBRACE",
    "RBRACE",  "BSLASH",  "HASH",      "SEMI",      "QUOTE",  "GRAVE",  "COMMA",  "DOT",
    "SLASH",   "CAPS",    "F1",        "F2",        "F3",     "F4",     "F5",     "F6",
    "F7",      "F8",      "F9",        "F10",       "F11",    "F12",    "PSCREEN","SCROLL",
    "PAUSE",   "INSERT",  "HOME",      "PGUP",      "DELETE", "END",    "PGDN",   "RIGHT",
    "LEFT",    "DOWN",    "UP",        "NUM",       "KPSLASH","KPSTAR", "KPMINUS","KPPLUS",
    "KPENTER", "KP1",     "KP2",       "KP3",       "KP4",    "KP5",    "KP6",    "KP7",
    "KP8",     "KP9",     "KP0",       "KPDOT"
};

// USB HID keyboard report handler
static void usb_keyboard_handle_report(usb_keyboard_t* kbd, usb_keyboard_report_t* report) {
    // Check for changes
    if (memcmp(report, &kbd->last_report, sizeof(usb_keyboard_report_t)) == 0) {
        return;
    }
    
    // Save last report
    memcpy(&kbd->last_report, report, sizeof(usb_keyboard_report_t));
    
    // Call callback if registered
    if (kbd->callback) {
        kbd->callback(report);
    }
}

// USB HID keyboard interrupt transfer callback
static void usb_keyboard_interrupt_callback(usb_transfer_t* transfer) {
    usb_keyboard_t* kbd = (usb_keyboard_t*)transfer->dev->driver;
    
    if (transfer->status == 0 && transfer->length == USB_HID_KEYBOARD_REPORT_SIZE) {
        usb_keyboard_handle_report(kbd, (usb_keyboard_report_t*)transfer->data);
    }
    
    // Resubmit transfer
    usb_submit_transfer(transfer);
}

// USB HID keyboard probe function
static int usb_keyboard_probe(usb_device_t* dev) {
    if (num_keyboards >= USB_MAX_DEVICES) {
        return -1;
    }
    
    // Allocate keyboard structure
    usb_keyboard_t* kbd = kmalloc(sizeof(usb_keyboard_t));
    if (!kbd) {
        return -1;
    }
    
    // Initialize keyboard
    memset(kbd, 0, sizeof(usb_keyboard_t));
    kbd->dev = dev;
    
    // Find keyboard interface
    usb_interface_descriptor_t* interface = NULL;
    uint8_t* ptr = (uint8_t*)dev->config;
    uint8_t* end = ptr + dev->config->wTotalLength;
    
    while (ptr < end) {
        uint8_t length = *ptr;
        uint8_t type = *(ptr + 1);
        
        if (type == USB_DESC_INTERFACE) {
            interface = (usb_interface_descriptor_t*)ptr;
            if (interface->bInterfaceClass == USB_HID_KEYBOARD_INTERFACE_CLASS &&
                interface->bInterfaceSubClass == USB_HID_KEYBOARD_INTERFACE_SUBCLASS &&
                interface->bInterfaceProtocol == USB_HID_KEYBOARD_INTERFACE_PROTOCOL) {
                kbd->interface = interface->bInterfaceNumber;
                break;
            }
        }
        
        ptr += length;
    }
    
    if (!interface) {
        kfree(kbd);
        return -1;
    }
    
    // Find interrupt IN endpoint
    ptr = (uint8_t*)interface;
    end = ptr + interface->bLength + interface->bNumEndpoints * sizeof(usb_endpoint_descriptor_t);
    
    while (ptr < end) {
        uint8_t length = *ptr;
        uint8_t type = *(ptr + 1);
        
        if (type == USB_DESC_ENDPOINT) {
            usb_endpoint_descriptor_t* endpoint = (usb_endpoint_descriptor_t*)ptr;
            if ((endpoint->bEndpointAddress & 0x80) && // IN endpoint
                (endpoint->bmAttributes & 0x03) == USB_TRANSFER_INT) { // Interrupt transfer
                kbd->endpoint = endpoint->bEndpointAddress;
                break;
            }
        }
        
        ptr += length;
    }
    
    if (!kbd->endpoint) {
        kfree(kbd);
        return -1;
    }
    
    // Set boot protocol
    usb_setup_packet_t setup = {
        .bmRequestType = 0x21, // Interface, class, OUT
        .bRequest = 0x0B,     // SET_PROTOCOL
        .wValue = 0,          // Boot protocol
        .wIndex = kbd->interface,
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
    
    if (usb_submit_transfer(&transfer) < 0) {
        kfree(kbd);
        return -1;
    }
    
    // Allocate and submit interrupt transfer
    transfer.endpoint = kbd->endpoint;
    transfer.type = USB_TRANSFER_INT;
    transfer.direction = USB_DIR_IN;
    transfer.data = kmalloc(USB_HID_KEYBOARD_REPORT_SIZE);
    transfer.length = USB_HID_KEYBOARD_REPORT_SIZE;
    transfer.callback = usb_keyboard_interrupt_callback;
    
    if (!transfer.data || usb_submit_transfer(&transfer) < 0) {
        kfree(transfer.data);
        kfree(kbd);
        return -1;
    }
    
    // Add to keyboard list
    usb_keyboards[num_keyboards++] = kbd;
    dev->driver = kbd;
    
    printf("USB keyboard initialized\n");
    return 0;
}

// USB HID keyboard disconnect function
static int usb_keyboard_disconnect(usb_device_t* dev) {
    usb_keyboard_t* kbd = (usb_keyboard_t*)dev->driver;
    if (!kbd) return -1;
    
    // Remove from keyboard list
    for (int i = 0; i < num_keyboards; i++) {
        if (usb_keyboards[i] == kbd) {
            usb_keyboards[i] = usb_keyboards[--num_keyboards];
            break;
        }
    }
    
    kfree(kbd);
    dev->driver = NULL;
    
    printf("USB keyboard disconnected\n");
    return 0;
}

// USB HID keyboard driver initialization
driver_t* usb_keyboard_driver_init(void) {
    // Initialize driver structure
    memset(&usb_keyboard_driver, 0, sizeof(usb_keyboard_driver));
    DRIVER_INIT(&usb_keyboard_driver.driver, "usb_keyboard", DRIVER_TYPE_USB);
    
    // Set driver operations
    usb_keyboard_driver.probe = usb_keyboard_probe;
    usb_keyboard_driver.disconnect = usb_keyboard_disconnect;
    
    // Register driver
    if (usb_register_driver(&usb_keyboard_driver) != 0) {
        return NULL;
    }
    
    return &usb_keyboard_driver.driver;
}

// USB HID keyboard driver cleanup
void usb_keyboard_driver_cleanup(void) {
    // Unregister driver
    usb_unregister_driver(&usb_keyboard_driver);
    
    // Free all keyboards
    for (int i = 0; i < num_keyboards; i++) {
        kfree(usb_keyboards[i]);
    }
    num_keyboards = 0;
}

// USB HID keyboard device functions
int usb_keyboard_set_leds(usb_keyboard_t* kbd, uint8_t leds) {
    if (!kbd) return -1;
    
    // Set LED state
    usb_setup_packet_t setup = {
        .bmRequestType = 0x21, // Interface, class, OUT
        .bRequest = 0x09,     // SET_REPORT
        .wValue = 0x0200,     // Output report
        .wIndex = kbd->interface,
        .wLength = 1
    };
    
    usb_transfer_t transfer = {
        .dev = kbd->dev,
        .endpoint = 0,
        .type = USB_TRANSFER_CONTROL,
        .direction = USB_DIR_OUT,
        .data = &leds,
        .length = 1
    };
    
    if (usb_submit_transfer(&transfer) < 0) {
        return -1;
    }
    
    kbd->led_state = leds;
    return 0;
}

int usb_keyboard_set_callback(usb_keyboard_t* kbd, void (*callback)(usb_keyboard_report_t* report)) {
    if (!kbd) return -1;
    
    kbd->callback = callback;
    return 0;
}

// USB HID keyboard utility functions
const char* usb_keyboard_key_name(uint8_t key) {
    if (key >= sizeof(key_names) / sizeof(key_names[0])) {
        return "UNKNOWN";
    }
    return key_names[key];
}

int usb_keyboard_is_modifier(uint8_t key) {
    return key >= 0xE0 && key <= 0xE7;
}

int usb_keyboard_is_keypad(uint8_t key) {
    return key >= 0x54 && key <= 0x63;
}

int usb_keyboard_is_function(uint8_t key) {
    return key >= 0x3A && key <= 0x45;
} 