#ifndef NETSTACK_H
#define NETSTACK_H

#include <stdint.h>
#include <stddef.h>

// Network byte order functions
static inline uint16_t ntohs(uint16_t x) {
    return ((x & 0xFF) << 8) | ((x & 0xFF00) >> 8);
}

static inline uint16_t htons(uint16_t x) {
    return ntohs(x);
}

static inline uint32_t ntohl(uint32_t x) {
    return ((x & 0xFF) << 24) | 
           ((x & 0xFF00) << 8) |
           ((x & 0xFF0000) >> 8) |
           ((x & 0xFF000000) >> 24);
}

static inline uint32_t htonl(uint32_t x) {
    return ntohl(x);
}

// Ethernet frame header
typedef struct {
    uint8_t dest_mac[6];
    uint8_t src_mac[6];
    uint16_t ethertype;
} __attribute__((packed)) ethernet_header_t;

// IPv4 header
typedef struct {
    uint8_t version_ihl;
    uint8_t tos;
    uint16_t total_length;
    uint16_t id;
    uint16_t flags_fragment;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dest_ip;
} __attribute__((packed)) ipv4_header_t;

// TCP header
typedef struct {
    uint16_t src_port;
    uint16_t dest_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint16_t flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent_ptr;
} __attribute__((packed)) tcp_header_t;

// UDP header
typedef struct {
    uint16_t src_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum;
} __attribute__((packed)) udp_header_t;

// ICMP header
typedef struct {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint32_t rest;
} __attribute__((packed)) icmp_header_t;

// Protocol numbers
#define IP_PROTO_ICMP    1
#define IP_PROTO_TCP     6
#define IP_PROTO_UDP     17

// Ethernet types
#define ETH_TYPE_IP      0x0800
#define ETH_TYPE_ARP     0x0806

// Socket states
#define SOCKET_CLOSED      0
#define SOCKET_LISTENING   1
#define SOCKET_CONNECTING  2
#define SOCKET_CONNECTED   3
#define SOCKET_CLOSING     4

// Forward declarations for linked list structures
typedef struct net_interface_struct net_interface_t;
typedef struct socket_struct socket_t;

// Network interface structure
struct net_interface_struct {
    uint8_t mac_addr[6];
    uint32_t ip_addr;
    uint32_t netmask;
    uint32_t gateway;
    
    // Interface operations
    int (*transmit)(const void* data, size_t length);
    int (*receive)(void* buffer, size_t max_length);
    
    // Linked list
    net_interface_t* next;
};

// Socket structure
struct socket_struct {
    int protocol;
    uint16_t local_port;
    uint16_t remote_port;
    uint32_t remote_ip;
    int state;
    
    // Socket buffers
    void* rx_buffer;
    size_t rx_size;
    void* tx_buffer;
    size_t tx_size;
    
    // Linked list
    socket_t* next;
};

// Network stack functions
void netstack_init(void);
void netstack_cleanup(void);

// Interface management
int netstack_register_interface(net_interface_t* interface);
void netstack_unregister_interface(net_interface_t* interface);
net_interface_t* netstack_get_interface(void);

// Packet handling
void netstack_handle_packet(const void* data, size_t length);
void netstack_handle_ipv4(const void* data, size_t length);
void netstack_handle_arp(const void* data, size_t length);
void netstack_handle_icmp(const void* data, size_t length);
void netstack_handle_tcp(const void* data, size_t length);
void netstack_handle_udp(const void* data, size_t length);

// Socket operations
socket_t* netstack_socket_create(int protocol);
void netstack_socket_destroy(socket_t* socket);
int netstack_socket_bind(socket_t* socket, uint16_t port);
int netstack_socket_connect(socket_t* socket, uint32_t ip, uint16_t port);
int netstack_socket_listen(socket_t* socket);
socket_t* netstack_socket_accept(socket_t* socket);
int netstack_socket_send(socket_t* socket, const void* data, size_t length);
int netstack_socket_receive(socket_t* socket, void* buffer, size_t max_length);

// Utility functions
uint16_t netstack_checksum(const void* data, size_t length);
void netstack_format_mac(char* buffer, const uint8_t* mac);
void netstack_format_ip(char* buffer, uint32_t ip);

#endif