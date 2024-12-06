#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>

// Network interface flags
#define NIC_FLAG_UP        0x01
#define NIC_FLAG_PROMISC   0x02
#define NIC_FLAG_BROADCAST 0x04

// Protocol numbers
#define PROTO_IP   0x0800
#define PROTO_ARP  0x0806
#define PROTO_IPv6 0x86DD

// IP protocol numbers
#define IP_PROTO_ICMP 1
#define IP_PROTO_TCP  6
#define IP_PROTO_UDP  17

// Maximum packet size
#define MAX_PACKET_SIZE 1518

// MAC address length
#define MAC_ADDR_LEN 6

// Network IOCTL commands
#define NETWORK_IOCTL_GET_MAC       0x1001
#define NETWORK_IOCTL_SET_MAC       0x1002
#define NETWORK_IOCTL_GET_IP        0x1003
#define NETWORK_IOCTL_SET_IP        0x1004
#define NETWORK_IOCTL_GET_NETMASK   0x1005
#define NETWORK_IOCTL_SET_NETMASK   0x1006
#define NETWORK_IOCTL_GET_GATEWAY   0x1007
#define NETWORK_IOCTL_SET_GATEWAY   0x1008
#define NETWORK_IOCTL_SET_FLAGS     0x1009
#define NETWORK_IOCTL_GET_FLAGS     0x100A
#define NETWORK_IOCTL_GET_STATS     0x100B

// Network structures
typedef struct network_interface {
    char name[32];
    uint8_t mac_addr[6];
    uint32_t ip_addr;
    uint32_t netmask;
    uint32_t gateway;
    int (*send)(struct network_interface* interface, const void* data, uint32_t length);
    void (*receive)(struct network_interface* interface, const void* data, uint32_t length);
} network_interface_t;

typedef struct {
    uint8_t dest_mac[MAC_ADDR_LEN];
    uint8_t src_mac[MAC_ADDR_LEN];
    uint16_t ethertype;
    uint8_t data[];
} __attribute__((packed)) ethernet_frame_t;

typedef struct {
    uint8_t version_ihl;
    uint8_t type_of_service;
    uint16_t total_length;
    uint16_t identification;
    uint16_t flags_fragment_offset;
    uint8_t time_to_live;
    uint8_t protocol;
    uint16_t header_checksum;
    uint32_t source_ip;
    uint32_t dest_ip;
    uint8_t data[];
} __attribute__((packed)) ip_header_t;

typedef struct {
    uint16_t src_port;
    uint16_t dest_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint16_t flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent;
    uint8_t data[];
} __attribute__((packed)) tcp_header_t;

typedef struct {
    uint16_t src_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum;
    uint8_t data[];
} __attribute__((packed)) udp_header_t;

typedef struct {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t identifier;
    uint16_t sequence;
    uint8_t data[];
} __attribute__((packed)) icmp_header_t;

// ICMP types
#define ICMP_ECHO_REPLY 0
#define ICMP_ECHO_REQUEST 8

// IP protocol numbers
#define IP_PROTOCOL_ICMP 1

// Network functions
void network_init(void);
int network_interface_up(network_interface_t* interface);
int network_interface_down(network_interface_t* interface);
int network_send_packet(network_interface_t* interface, const void* data, uint32_t length);
void network_receive_packet(network_interface_t* interface, const void* data, uint32_t length);

// IP functions
uint16_t ip_checksum(void* data, uint32_t length);
int ip_send(network_interface_t* interface, uint32_t dest_ip, uint8_t protocol, const void* data, uint32_t length);
void ip_receive(network_interface_t* interface, ip_header_t* packet);

// TCP functions
int tcp_connect(uint32_t dest_ip, uint16_t dest_port);
int tcp_listen(uint16_t port);
int tcp_accept(int socket);
int tcp_send(int socket, void* data, uint16_t length);
int tcp_receive(int socket, void* buffer, uint16_t max_length);
int tcp_close(int socket);

// UDP functions
int udp_bind(uint16_t port);
int udp_sendto(int socket, uint32_t dest_ip, uint16_t dest_port, void* data, uint16_t length);
int udp_receive(int socket, void* buffer, uint16_t max_length, uint32_t* src_ip, uint16_t* src_port);
int udp_close(int socket);

// ARP functions
void arp_request(network_interface_t* interface, uint32_t ip);
void arp_reply(network_interface_t* interface, uint8_t* dest_mac, uint32_t dest_ip);
void arp_receive(network_interface_t* interface, void* packet);

// ICMP functions
void icmp_send_echo(network_interface_t* interface, uint32_t dest_ip, uint16_t id, uint16_t sequence);
void icmp_receive(network_interface_t* interface, ip_header_t* ip_header);

// Socket functions
int socket_create(int type, int protocol);
int socket_bind(int socket, uint16_t port);
int socket_connect(int socket, uint32_t ip, uint16_t port);
int socket_listen(int socket, int backlog);
int socket_accept(int socket);
int socket_send(int socket, void* data, uint16_t length);
int socket_receive(int socket, void* buffer, uint16_t max_length);
int socket_close(int socket);

// Network utilities
uint32_t ip_to_uint32(const char* ip_str);
void uint32_to_ip(uint32_t ip, char* buffer);
uint16_t htons(uint16_t value);
uint16_t ntohs(uint16_t value);
uint32_t htonl(uint32_t value);
uint32_t ntohl(uint32_t value);

#endif