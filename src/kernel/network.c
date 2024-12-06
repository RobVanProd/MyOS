#include "network.h"
#include <string.h>
#include <stdio.h>
#include "memory.h"

// Network globals
static network_interface_t* interfaces[4] = {NULL}; // Support up to 4 network interfaces
static int num_interfaces = 0;

// Socket management
#define MAX_SOCKETS 64
typedef struct {
    int type;
    int protocol;
    uint16_t local_port;
    uint32_t remote_ip;
    uint16_t remote_port;
    int state;
    void* data;
} socket_t;

static socket_t sockets[MAX_SOCKETS] = {0};

// Initialize networking
void network_init(void) {
    // Initialize network interfaces array
    for (int i = 0; i < 4; i++) {
        interfaces[i] = NULL;
    }
    
    // Initialize sockets array
    for (int i = 0; i < MAX_SOCKETS; i++) {
        sockets[i].type = 0;
        sockets[i].protocol = 0;
        sockets[i].state = 0;
        sockets[i].data = NULL;
    }
}

// Network interface management
int network_interface_up(network_interface_t* interface) {
    if (!interface) return -1;
    return 0;
}

int network_interface_down(network_interface_t* interface) {
    if (!interface) return -1;
    return 0;
}

// Calculate IP checksum
uint16_t ip_checksum(void* data, uint32_t length) {
    uint32_t sum = 0;
    uint16_t* ptr = (uint16_t*)data;
    
    while (length > 1) {
        sum += *ptr++;
        length -= 2;
    }
    
    if (length > 0) {
        sum += *(uint8_t*)ptr;
    }
    
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return ~sum;
}

// Send an IP packet
int ip_send(network_interface_t* interface, uint32_t dest_ip, uint8_t protocol, const void* data, uint32_t length) {
    uint8_t packet_buffer[2048];
    ip_header_t* packet = (ip_header_t*)packet_buffer;
    
    packet->version_ihl = 0x45; // IPv4, 5 DWORDs header
    packet->type_of_service = 0;
    packet->total_length = htons(sizeof(ip_header_t) + length);
    packet->identification = htons(0); // Should be incremented for each packet
    packet->flags_fragment_offset = 0;
    packet->time_to_live = 64;
    packet->protocol = protocol;
    packet->source_ip = interface->ip_addr;
    packet->dest_ip = dest_ip;
    
    // Copy data
    memcpy(packet->data, data, length);
    
    // Calculate checksum
    packet->header_checksum = 0;
    packet->header_checksum = ip_checksum(packet, sizeof(ip_header_t));
    
    return network_send_packet(interface, packet, sizeof(ip_header_t) + length);
}

// Send a raw network packet
int network_send_packet(network_interface_t* interface, const void* data, uint32_t length) {
    if (!interface || !interface->send || !data || length == 0) {
        return -1;
    }
    return interface->send(interface, data, length);
}

// Handle received IP packet
void network_receive_packet(network_interface_t* interface, const void* data, uint32_t length) {
    if (!data || length < sizeof(ip_header_t)) return;
    
    ip_header_t* ip_header = (ip_header_t*)data;
    
    // Verify checksum
    uint16_t orig_checksum = ip_header->header_checksum;
    ip_header->header_checksum = 0;
    if (ip_checksum(ip_header, sizeof(ip_header_t)) != orig_checksum) {
        return; // Invalid checksum
    }
    ip_header->header_checksum = orig_checksum;
    
    // Handle different protocols
    switch (ip_header->protocol) {
        case IP_PROTOCOL_ICMP:
            icmp_receive(interface, ip_header);
            break;
        default:
            break;
    }
}

// Handle received ICMP packet
void icmp_receive(network_interface_t* interface, ip_header_t* ip_header) {
    if (!interface || !ip_header) return;
    
    icmp_header_t* icmp = (icmp_header_t*)ip_header->data;
    uint32_t icmp_length = ntohs(ip_header->total_length) - sizeof(ip_header_t);
    
    if (icmp_length < sizeof(icmp_header_t)) return;
    
    switch (icmp->type) {
        case ICMP_ECHO_REQUEST:
            // Send echo reply
            icmp->type = ICMP_ECHO_REPLY;
            icmp->checksum = 0;
            icmp->checksum = ip_checksum(icmp, icmp_length);
            
            ip_send(interface, ntohl(ip_header->source_ip), IP_PROTOCOL_ICMP, 
                   icmp, icmp_length);
            break;
        default:
            break;
    }
}

// Socket creation
int socket_create(int type, int protocol) {
    // Find free socket slot
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i].type == 0) {
            sockets[i].type = type;
            sockets[i].protocol = protocol;
            sockets[i].state = 0;
            return i;
        }
    }
    
    return -1;
}

// Socket binding
int socket_bind(int socket, uint16_t port) {
    if (socket < 0 || socket >= MAX_SOCKETS) return -1;
    if (sockets[socket].type == 0) return -1;
    
    sockets[socket].local_port = port;
    return 0;
}

// Socket connection
int socket_connect(int socket, uint32_t ip, uint16_t port) {
    if (socket < 0 || socket >= MAX_SOCKETS) return -1;
    if (sockets[socket].type == 0) return -1;
    
    sockets[socket].remote_ip = ip;
    sockets[socket].remote_port = port;
    
    // For TCP, initiate connection
    if (sockets[socket].protocol == IP_PROTO_TCP) {
        // TODO: Implement TCP connection
        return -1;
    }
    
    return 0;
}

// Network utilities
uint32_t ip_to_uint32(const char* ip_str) {
    uint32_t ip = 0;
    uint8_t octet = 0;
    
    while (*ip_str) {
        if (*ip_str == '.') {
            ip = (ip << 8) | octet;
            octet = 0;
        } else if (*ip_str >= '0' && *ip_str <= '9') {
            octet = octet * 10 + (*ip_str - '0');
        }
        ip_str++;
    }
    
    return (ip << 8) | octet;
}

void uint32_to_ip(uint32_t ip, char* buffer) {
    sprintf(buffer, "%d.%d.%d.%d",
            (ip >> 24) & 0xFF,
            (ip >> 16) & 0xFF,
            (ip >> 8) & 0xFF,
            ip & 0xFF);
}

// Byte order conversion
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