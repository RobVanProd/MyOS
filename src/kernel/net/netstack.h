#ifndef NETSTACK_H
#define NETSTACK_H

#include <stdint.h>
#include <stddef.h>

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

// Network interface flags
#define NIF_UP          0x0001
#define NIF_PROMISC     0x0002
#define NIF_BROADCAST   0x0004
#define NIF_MULTICAST   0x0008
#define NIF_LOOPBACK    0x0010

// Network interface structure
typedef struct netif {
    char name[16];
    uint8_t mac[6];
    uint32_t ip;
    uint32_t netmask;
    uint32_t gateway;
    uint16_t mtu;
    uint16_t flags;
    void* driver;
    int (*transmit)(struct netif* netif, const void* data, size_t length);
    struct netif* next;
} netif_t;

// Protocol handler function type
typedef void (*proto_handler_t)(netif_t* netif, const void* packet, size_t length);

// Network stack initialization
void netstack_init(void);

// Network interface management
netif_t* netif_add(const char* name, const uint8_t* mac, uint32_t ip, uint32_t netmask, uint32_t gateway);
void netif_remove(netif_t* netif);
netif_t* netif_find(const char* name);
void netif_set_up(netif_t* netif);
void netif_set_down(netif_t* netif);

// Protocol registration
int proto_register(uint16_t proto, proto_handler_t handler);
int proto_unregister(uint16_t proto);

// Packet handling
void netstack_input(netif_t* netif, const void* data, size_t length);
int netstack_output(netif_t* netif, uint16_t proto, const void* data, size_t length);

// ARP functions
void arp_init(void);
int arp_resolve(netif_t* netif, uint32_t ip, uint8_t* mac);
void arp_update(netif_t* netif, uint32_t ip, const uint8_t* mac);

// IP functions
void ip_init(void);
int ip_output(netif_t* netif, uint8_t proto, const void* data, size_t length, uint32_t src_ip, uint32_t dst_ip);
uint16_t ip_checksum(const void* data, size_t length);

// ICMP functions
void icmp_init(void);
int icmp_echo_request(netif_t* netif, uint32_t dst_ip, uint16_t id, uint16_t seq, const void* data, size_t length);

// UDP functions
void udp_init(void);
int udp_bind(uint16_t port, void (*handler)(const void* data, size_t length, uint32_t src_ip, uint16_t src_port));
int udp_unbind(uint16_t port);
int udp_sendto(netif_t* netif, const void* data, size_t length, uint32_t dst_ip, uint16_t dst_port, uint16_t src_port);

// TCP functions
void tcp_init(void);
int tcp_listen(uint16_t port, void (*handler)(int sock));
int tcp_connect(netif_t* netif, uint32_t dst_ip, uint16_t dst_port, uint16_t src_port);
int tcp_send(int sock, const void* data, size_t length);
int tcp_recv(int sock, void* buffer, size_t length);
int tcp_close(int sock);

// Socket structure
typedef struct {
    uint8_t protocol;
    uint16_t local_port;
    uint32_t local_ip;
    uint16_t remote_port;
    uint32_t remote_ip;
    uint32_t seq_no;
    uint32_t ack_no;
    uint16_t window;
    uint8_t state;
    void* data;
} socket_t;

// Socket functions
int socket_create(uint8_t protocol);
int socket_bind(int sock, uint32_t ip, uint16_t port);
int socket_connect(int sock, uint32_t ip, uint16_t port);
int socket_listen(int sock, int backlog);
int socket_accept(int sock);
int socket_send(int sock, const void* data, size_t length);
int socket_recv(int sock, void* buffer, size_t length);
int socket_close(int sock);

// Network utilities
uint32_t ip_to_uint32(const char* ip);
void uint32_to_ip(uint32_t ip, char* buffer);
uint16_t htons(uint16_t value);
uint16_t ntohs(uint16_t value);
uint32_t htonl(uint32_t value);
uint32_t ntohl(uint32_t value);

// Network statistics
typedef struct {
    uint64_t rx_packets;
    uint64_t tx_packets;
    uint64_t rx_bytes;
    uint64_t tx_bytes;
    uint32_t rx_errors;
    uint32_t tx_errors;
    uint32_t rx_dropped;
    uint32_t tx_dropped;
    uint32_t collisions;
} net_stats_t;

// Network statistics functions
void net_stats_init(void);
void net_stats_get(net_stats_t* stats);
void net_stats_reset(void);

// Network debugging
void net_dump_packet(const void* data, size_t length);
void net_dump_stats(void);

#endif 