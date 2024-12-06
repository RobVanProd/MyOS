#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include <stdbool.h>
#include "driver.h"

// Network IOCTL commands
#define NETWORK_IOCTL_GET_MAC      1
#define NETWORK_IOCTL_SET_MAC      2

// Network interface structure
typedef struct {
    uint8_t mac_address[6];
    bool is_up;
    void* driver_data;
} network_interface_t;

// Network driver operations
typedef struct {
    int (*init)(void* driver_data);
    int (*send_packet)(void* driver_data, const void* data, size_t length);
    int (*receive_packet)(void* driver_data, void* buffer, size_t buffer_size);
    int (*get_mac_address)(void* driver_data, uint8_t* mac_address);
} network_driver_ops_t;

// Network driver structure
typedef struct {
    driver_t driver;
    network_driver_ops_t ops;
    network_interface_t interface;
} network_driver_t;

// Function declarations
int network_init(void);
int network_register_driver(network_driver_t* driver);
int network_send_packet(network_driver_t* driver, const void* data, size_t length);
int network_receive_packet(network_driver_t* driver, void* buffer, size_t buffer_size);

#endif /* NETWORK_H */
