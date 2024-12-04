#ifndef USB_KEYBOARD_H
#define USB_KEYBOARD_H

#include "usb.h"

// USB HID keyboard constants
#define USB_HID_KEYBOARD_INTERFACE_CLASS    0x03
#define USB_HID_KEYBOARD_INTERFACE_SUBCLASS 0x01
#define USB_HID_KEYBOARD_INTERFACE_PROTOCOL 0x01

// USB HID keyboard report descriptor size
#define USB_HID_KEYBOARD_REPORT_SIZE 8

// USB HID keyboard modifier keys
#define USB_HID_KEYBOARD_LCTRL   0x01
#define USB_HID_KEYBOARD_LSHIFT  0x02
#define USB_HID_KEYBOARD_LALT    0x04
#define USB_HID_KEYBOARD_LGUI    0x08
#define USB_HID_KEYBOARD_RCTRL   0x10
#define USB_HID_KEYBOARD_RSHIFT  0x20
#define USB_HID_KEYBOARD_RALT    0x40
#define USB_HID_KEYBOARD_RGUI    0x80

// USB HID keyboard LED states
#define USB_HID_KEYBOARD_LED_NUMLOCK    0x01
#define USB_HID_KEYBOARD_LED_CAPSLOCK   0x02
#define USB_HID_KEYBOARD_LED_SCROLLLOCK 0x04
#define USB_HID_KEYBOARD_LED_COMPOSE    0x08
#define USB_HID_KEYBOARD_LED_KANA       0x10

// USB HID keyboard report structure
typedef struct {
    uint8_t modifiers;
    uint8_t reserved;
    uint8_t keys[6];
} __attribute__((packed)) usb_keyboard_report_t;

// USB HID keyboard device structure
typedef struct {
    usb_device_t* dev;
    uint8_t interface;
    uint8_t endpoint;
    uint8_t protocol;
    uint8_t led_state;
    usb_keyboard_report_t last_report;
    void (*callback)(usb_keyboard_report_t* report);
} usb_keyboard_t;

// USB HID keyboard driver functions
driver_t* usb_keyboard_driver_init(void);
void usb_keyboard_driver_cleanup(void);

// USB HID keyboard device functions
int usb_keyboard_set_leds(usb_keyboard_t* kbd, uint8_t leds);
int usb_keyboard_set_callback(usb_keyboard_t* kbd, void (*callback)(usb_keyboard_report_t* report));

// USB HID keyboard utility functions
const char* usb_keyboard_key_name(uint8_t key);
int usb_keyboard_is_modifier(uint8_t key);
int usb_keyboard_is_keypad(uint8_t key);
int usb_keyboard_is_function(uint8_t key);

#endif 