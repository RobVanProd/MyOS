#ifndef DHCP_H
#define DHCP_H

#include <stdint.h>
#include "netstack.h"

// DHCP message types
#define DHCP_DISCOVER 1
#define DHCP_OFFER    2
#define DHCP_REQUEST  3
#define DHCP_ACK      5
#define DHCP_NAK      6
#define DHCP_RELEASE  7

// DHCP options
#define DHCP_OPT_PAD          0
#define DHCP_OPT_SUBNET_MASK  1
#define DHCP_OPT_ROUTER       3
#define DHCP_OPT_DNS          6
#define DHCP_OPT_HOSTNAME     12
#define DHCP_OPT_DOMAIN       15
#define DHCP_OPT_BROADCAST    28
#define DHCP_OPT_REQ_IP       50
#define DHCP_OPT_LEASE_TIME   51
#define DHCP_OPT_MSG_TYPE     53
#define DHCP_OPT_SERVER_ID    54
#define DHCP_OPT_PARAM_REQ    55
#define DHCP_OPT_END          255

// DHCP message structure
typedef struct {
    uint8_t op;           // Message op code
    uint8_t htype;        // Hardware address type
    uint8_t hlen;         // Hardware address length
    uint8_t hops;         // Hops
    uint32_t xid;         // Transaction ID
    uint16_t secs;        // Seconds elapsed
    uint16_t flags;       // Flags
    uint32_t ciaddr;      // Client IP address
    uint32_t yiaddr;      // Your IP address
    uint32_t siaddr;      // Server IP address
    uint32_t giaddr;      // Gateway IP address
    uint8_t chaddr[16];   // Client hardware address
    uint8_t sname[64];    // Server host name
    uint8_t file[128];    // Boot file name
    uint8_t options[308]; // Options
} __attribute__((packed)) dhcp_message_t;

// DHCP client states
typedef enum {
    DHCP_STATE_INIT,
    DHCP_STATE_SELECTING,
    DHCP_STATE_REQUESTING,
    DHCP_STATE_BOUND,
    DHCP_STATE_RENEWING,
    DHCP_STATE_REBINDING
} dhcp_state_t;

// DHCP client structure
typedef struct {
    dhcp_state_t state;
    uint32_t xid;
    uint32_t server_ip;
    uint32_t offered_ip;
    uint32_t lease_time;
    uint32_t t1_time;
    uint32_t t2_time;
    uint32_t renewal_time;
    uint32_t rebind_time;
    net_interface_t* interface;
} dhcp_client_t;

// DHCP client functions
void dhcp_init(net_interface_t* interface);
void dhcp_cleanup(void);
int dhcp_start(void);
void dhcp_stop(void);
void dhcp_release(void);
void dhcp_handle_packet(const void* data, size_t length);

#endif 