#include "netstack.h"
#include "../memory.h"
#include <string.h>

// Global variables
static netif_t* netif_list = NULL;
static proto_handler_t proto_handlers[0x10000] = {0};
static net_stats_t net_stats = {0};

// Network interface management
netif_t* netif_add(const char* name, const uint8_t* mac, uint32_t ip, uint32_t netmask, uint32_t gateway) {
    netif_t* netif = kmalloc(sizeof(netif_t));
    if (!netif) return NULL;
    
    // Initialize interface
    memset(netif, 0, sizeof(netif_t));
    strncpy(netif->name, name, sizeof(netif->name)-1);
    memcpy(netif->mac, mac, 6);
    netif->ip = ip;
    netif->netmask = netmask;
    netif->gateway = gateway;
    netif->mtu = 1500;
    
    // Add to list
    netif->next = netif_list;
    netif_list = netif;
    
    return netif;
}

void netif_remove(netif_t* netif) {
    if (!netif) return;
    
    // Remove from list
    netif_t** pp = &netif_list;
    while (*pp) {
        if (*pp == netif) {
            *pp = netif->next;
            kfree(netif);
            return;
        }
        pp = &(*pp)->next;
    }
}

netif_t* netif_find(const char* name) {
    netif_t* netif = netif_list;
    while (netif) {
        if (strcmp(netif->name, name) == 0) {
            return netif;
        }
        netif = netif->next;
    }
    return NULL;
}

void netif_set_up(netif_t* netif) {
    if (!netif) return;
    netif->flags |= NIF_UP;
}

void netif_set_down(netif_t* netif) {
    if (!netif) return;
    netif->flags &= ~NIF_UP;
}

// Protocol registration
int proto_register(uint16_t proto, proto_handler_t handler) {
    if (!handler) return -1;
    if (proto_handlers[proto]) return -1;
    proto_handlers[proto] = handler;
    return 0;
}

int proto_unregister(uint16_t proto) {
    if (!proto_handlers[proto]) return -1;
    proto_handlers[proto] = NULL;
    return 0;
}

// Packet handling
void netstack_input(netif_t* netif, const void* data, size_t length) {
    if (!netif || !data || length < 14) return;
    
    // Update statistics
    net_stats.rx_packets++;
    net_stats.rx_bytes += length;
    
    // Get Ethernet header
    const uint8_t* eth = data;
    uint16_t proto = (eth[12] << 8) | eth[13];
    
    // Find protocol handler
    proto_handler_t handler = proto_handlers[proto];
    if (handler) {
        handler(netif, eth + 14, length - 14);
    } else {
        net_stats.rx_dropped++;
    }
}

int netstack_output(netif_t* netif, uint16_t proto, const void* data, size_t length) {
    if (!netif || !data || length > netif->mtu) return -1;
    
    // Allocate buffer for Ethernet frame
    uint8_t* frame = kmalloc(length + 14);
    if (!frame) return -1;
    
    // Build Ethernet header
    memcpy(frame + 0, netif->mac, 6);  // Source MAC
    // Destination MAC will be filled by ARP
    frame[12] = proto >> 8;
    frame[13] = proto & 0xFF;
    
    // Copy payload
    memcpy(frame + 14, data, length);
    
    // Transmit frame
    if (netif->transmit(netif, frame, length + 14) < 0) {
        net_stats.tx_dropped++;
        kfree(frame);
        return -1;
    }
    
    // Update statistics
    net_stats.tx_packets++;
    net_stats.tx_bytes += length + 14;
    
    kfree(frame);
    return 0;
}

// Network utilities
uint32_t ip_to_uint32(const char* ip) {
    uint32_t result = 0;
    uint8_t octet = 0;
    
    while (*ip) {
        if (*ip == '.') {
            result = (result << 8) | octet;
            octet = 0;
        } else if (*ip >= '0' && *ip <= '9') {
            octet = octet * 10 + (*ip - '0');
        }
        ip++;
    }
    
    return (result << 8) | octet;
}

void uint32_to_ip(uint32_t ip, char* buffer) {
    sprintf(buffer, "%d.%d.%d.%d",
        (ip >> 24) & 0xFF,
        (ip >> 16) & 0xFF,
        (ip >> 8) & 0xFF,
        ip & 0xFF);
}

uint16_t htons(uint16_t value) {
    return ((value & 0xFF) << 8) | ((value >> 8) & 0xFF);
}

uint16_t ntohs(uint16_t value) {
    return htons(value);
}

uint32_t htonl(uint32_t value) {
    return ((value & 0xFF) << 24) |
           ((value & 0xFF00) << 8) |
           ((value >> 8) & 0xFF00) |
           ((value >> 24) & 0xFF);
}

uint32_t ntohl(uint32_t value) {
    return htonl(value);
}

// Network statistics
void net_stats_init(void) {
    memset(&net_stats, 0, sizeof(net_stats));
}

void net_stats_get(net_stats_t* stats) {
    if (!stats) return;
    memcpy(stats, &net_stats, sizeof(net_stats_t));
}

void net_stats_reset(void) {
    memset(&net_stats, 0, sizeof(net_stats));
}

// Network debugging
void net_dump_packet(const void* data, size_t length) {
    const uint8_t* bytes = data;
    printf("Packet dump (%d bytes):\n", length);
    
    for (size_t i = 0; i < length; i++) {
        if (i % 16 == 0) {
            printf("\n%04x: ", i);
        }
        printf("%02x ", bytes[i]);
    }
    printf("\n");
}

void net_dump_stats(void) {
    printf("Network Statistics:\n");
    printf("  RX Packets: %llu\n", net_stats.rx_packets);
    printf("  TX Packets: %llu\n", net_stats.tx_packets);
    printf("  RX Bytes: %llu\n", net_stats.rx_bytes);
    printf("  TX Bytes: %llu\n", net_stats.tx_bytes);
    printf("  RX Errors: %u\n", net_stats.rx_errors);
    printf("  TX Errors: %u\n", net_stats.tx_errors);
    printf("  RX Dropped: %u\n", net_stats.rx_dropped);
    printf("  TX Dropped: %u\n", net_stats.tx_dropped);
    printf("  Collisions: %u\n", net_stats.collisions);
}

// Network stack initialization
void netstack_init(void) {
    // Initialize statistics
    net_stats_init();
    
    // Initialize protocol handlers
    memset(proto_handlers, 0, sizeof(proto_handlers));
    
    // Initialize protocols
    arp_init();
    ip_init();
    icmp_init();
    udp_init();
    tcp_init();
} 