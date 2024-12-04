#include "dhcp.h"
#include "../memory.h"
#include <string.h>

// DHCP client instance
static dhcp_client_t dhcp_client;

// DHCP ports
#define DHCP_SERVER_PORT 67
#define DHCP_CLIENT_PORT 68

// DHCP magic cookie
#define DHCP_MAGIC_COOKIE 0x63825363

// Initialize DHCP client
void dhcp_init(net_interface_t* interface) {
    memset(&dhcp_client, 0, sizeof(dhcp_client_t));
    dhcp_client.state = DHCP_STATE_INIT;
    dhcp_client.interface = interface;
}

// Cleanup DHCP client
void dhcp_cleanup(void) {
    dhcp_stop();
}

// Create DHCP message
static void dhcp_create_message(dhcp_message_t* msg, uint8_t type) {
    memset(msg, 0, sizeof(dhcp_message_t));
    
    msg->op = 1; // BOOTREQUEST
    msg->htype = 1; // Ethernet
    msg->hlen = 6; // MAC address length
    msg->xid = dhcp_client.xid;
    msg->secs = 0;
    msg->flags = htons(0x8000); // Broadcast flag
    
    // Copy client MAC address
    memcpy(msg->chaddr, dhcp_client.interface->mac_addr, 6);
    
    // Set magic cookie
    uint32_t* cookie = (uint32_t*)msg->options;
    *cookie = htonl(DHCP_MAGIC_COOKIE);
    
    // Add message type option
    uint8_t* opt = msg->options + 4;
    *opt++ = DHCP_OPT_MSG_TYPE;
    *opt++ = 1;
    *opt++ = type;
    
    // Add parameter request list
    *opt++ = DHCP_OPT_PARAM_REQ;
    *opt++ = 3;
    *opt++ = DHCP_OPT_SUBNET_MASK;
    *opt++ = DHCP_OPT_ROUTER;
    *opt++ = DHCP_OPT_DNS;
    
    // End option
    *opt++ = DHCP_OPT_END;
}

// Send DHCP discover message
static void dhcp_send_discover(void) {
    dhcp_message_t msg;
    dhcp_create_message(&msg, DHCP_DISCOVER);
    
    // Create UDP packet
    uint8_t packet[sizeof(dhcp_message_t) + 100];
    udp_header_t* udp = (udp_header_t*)packet;
    
    udp->src_port = htons(DHCP_CLIENT_PORT);
    udp->dest_port = htons(DHCP_SERVER_PORT);
    udp->length = htons(sizeof(dhcp_message_t) + sizeof(udp_header_t));
    udp->checksum = 0;
    
    memcpy(packet + sizeof(udp_header_t), &msg, sizeof(dhcp_message_t));
    
    // Send packet
    netstack_send_packet(packet, sizeof(dhcp_message_t) + sizeof(udp_header_t));
    
    dhcp_client.state = DHCP_STATE_SELECTING;
}

// Send DHCP request message
static void dhcp_send_request(void) {
    dhcp_message_t msg;
    dhcp_create_message(&msg, DHCP_REQUEST);
    
    // Add requested IP option
    uint8_t* opt = msg.options;
    while (*opt != DHCP_OPT_END) opt++;
    
    *opt++ = DHCP_OPT_REQ_IP;
    *opt++ = 4;
    *(uint32_t*)opt = htonl(dhcp_client.offered_ip);
    opt += 4;
    
    // Add server identifier option
    *opt++ = DHCP_OPT_SERVER_ID;
    *opt++ = 4;
    *(uint32_t*)opt = htonl(dhcp_client.server_ip);
    opt += 4;
    
    *opt = DHCP_OPT_END;
    
    // Create UDP packet
    uint8_t packet[sizeof(dhcp_message_t) + 100];
    udp_header_t* udp = (udp_header_t*)packet;
    
    udp->src_port = htons(DHCP_CLIENT_PORT);
    udp->dest_port = htons(DHCP_SERVER_PORT);
    udp->length = htons(sizeof(dhcp_message_t) + sizeof(udp_header_t));
    udp->checksum = 0;
    
    memcpy(packet + sizeof(udp_header_t), &msg, sizeof(dhcp_message_t));
    
    // Send packet
    netstack_send_packet(packet, sizeof(dhcp_message_t) + sizeof(udp_header_t));
    
    dhcp_client.state = DHCP_STATE_REQUESTING;
}

// Parse DHCP options
static void dhcp_parse_options(const uint8_t* options, size_t length) {
    const uint8_t* end = options + length;
    const uint8_t* opt = options + 4; // Skip magic cookie
    
    while (opt < end && *opt != DHCP_OPT_END) {
        uint8_t code = *opt++;
        uint8_t len = *opt++;
        
        switch (code) {
            case DHCP_OPT_MSG_TYPE:
                // Handle message type
                break;
                
            case DHCP_OPT_SUBNET_MASK:
                if (len == 4) {
                    dhcp_client.interface->netmask = ntohl(*(uint32_t*)opt);
                }
                break;
                
            case DHCP_OPT_ROUTER:
                if (len >= 4) {
                    dhcp_client.interface->gateway = ntohl(*(uint32_t*)opt);
                }
                break;
                
            case DHCP_OPT_LEASE_TIME:
                if (len == 4) {
                    dhcp_client.lease_time = ntohl(*(uint32_t*)opt);
                    dhcp_client.t1_time = dhcp_client.lease_time / 2;
                    dhcp_client.t2_time = dhcp_client.lease_time * 7 / 8;
                }
                break;
                
            case DHCP_OPT_SERVER_ID:
                if (len == 4) {
                    dhcp_client.server_ip = ntohl(*(uint32_t*)opt);
                }
                break;
        }
        
        opt += len;
    }
}

// Handle DHCP packet
void dhcp_handle_packet(const void* data, size_t length) {
    if (length < sizeof(dhcp_message_t)) return;
    
    const dhcp_message_t* msg = (const dhcp_message_t*)data;
    
    // Verify transaction ID
    if (msg->xid != dhcp_client.xid) return;
    
    // Parse options
    dhcp_parse_options(msg->options, sizeof(msg->options));
    
    // Get message type
    uint8_t msg_type = 0;
    const uint8_t* opt = msg->options + 4;
    while (*opt != DHCP_OPT_END) {
        if (*opt == DHCP_OPT_MSG_TYPE && opt[1] == 1) {
            msg_type = opt[2];
            break;
        }
        opt += 2 + opt[1];
    }
    
    switch (msg_type) {
        case DHCP_OFFER:
            if (dhcp_client.state == DHCP_STATE_SELECTING) {
                dhcp_client.offered_ip = ntohl(msg->yiaddr);
                dhcp_send_request();
            }
            break;
            
        case DHCP_ACK:
            if (dhcp_client.state == DHCP_STATE_REQUESTING) {
                dhcp_client.interface->ip_addr = dhcp_client.offered_ip;
                dhcp_client.state = DHCP_STATE_BOUND;
                
                // Set up renewal timer
                dhcp_client.renewal_time = dhcp_client.t1_time;
                dhcp_client.rebind_time = dhcp_client.t2_time;
            }
            break;
            
        case DHCP_NAK:
            // Start over
            dhcp_client.state = DHCP_STATE_INIT;
            dhcp_start();
            break;
    }
}

// Start DHCP client
int dhcp_start(void) {
    if (!dhcp_client.interface) return -1;
    
    // Generate transaction ID
    dhcp_client.xid = 0x12345678; // TODO: Generate random XID
    
    // Send DHCP discover
    dhcp_send_discover();
    
    return 0;
}

// Stop DHCP client
void dhcp_stop(void) {
    if (dhcp_client.state != DHCP_STATE_BOUND) return;
    
    // Send DHCP release
    dhcp_message_t msg;
    dhcp_create_message(&msg, DHCP_RELEASE);
    
    msg.ciaddr = htonl(dhcp_client.interface->ip_addr);
    
    // Create UDP packet
    uint8_t packet[sizeof(dhcp_message_t) + 100];
    udp_header_t* udp = (udp_header_t*)packet;
    
    udp->src_port = htons(DHCP_CLIENT_PORT);
    udp->dest_port = htons(DHCP_SERVER_PORT);
    udp->length = htons(sizeof(dhcp_message_t) + sizeof(udp_header_t));
    udp->checksum = 0;
    
    memcpy(packet + sizeof(udp_header_t), &msg, sizeof(dhcp_message_t));
    
    // Send packet
    netstack_send_packet(packet, sizeof(dhcp_message_t) + sizeof(udp_header_t));
    
    dhcp_client.state = DHCP_STATE_INIT;
}

// Release DHCP lease
void dhcp_release(void) {
    dhcp_stop();
} 