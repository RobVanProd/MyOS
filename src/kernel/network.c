#include "network.h"
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
    if (!interface || num_interfaces >= 4) return -1;
    
    interface->flags |= NIC_FLAG_UP;
    interfaces[num_interfaces++] = interface;
    
    return 0;
}

int network_interface_down(network_interface_t* interface) {
    if (!interface) return -1;
    
    interface->flags &= ~NIC_FLAG_UP;
    
    // Remove from interfaces array
    for (int i = 0; i < num_interfaces; i++) {
        if (interfaces[i] == interface) {
            for (int j = i; j < num_interfaces - 1; j++) {
                interfaces[j] = interfaces[j + 1];
            }
            interfaces[--num_interfaces] = NULL;
            break;
        }
    }
    
    return 0;
}

// Calculate IP checksum
uint16_t ip_checksum(void* data, uint16_t length) {
    uint32_t sum = 0;
    uint16_t* ptr = (uint16_t*)data;
    
    // Add up all 16-bit words
    while (length > 1) {
        sum += *ptr++;
        length -= 2;
    }
    
    // Add last byte if present
    if (length > 0) {
        sum += *(uint8_t*)ptr;
    }
    
    // Add carries
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return ~sum;
}

// Send an IP packet
int ip_send(network_interface_t* interface, uint32_t dest_ip, uint8_t protocol, void* data, uint16_t length) {
    if (!interface || !data) return -1;
    
    // Allocate buffer for IP packet
    uint16_t total_length = sizeof(ip_header_t) + length;
    ip_header_t* packet = kmalloc(total_length);
    if (!packet) return -1;
    
    // Fill IP header
    packet->version_ihl = 0x45; // IPv4, 5 DWORDS header length
    packet->tos = 0;
    packet->total_length = htons(total_length);
    packet->id = htons(0); // Should be incremented for each packet
    packet->flags_fragment = 0;
    packet->ttl = 64;
    packet->protocol = protocol;
    packet->src_ip = interface->ip;
    packet->dest_ip = dest_ip;
    
    // Copy data
    memcpy(packet->data, data, length);
    
    // Calculate checksum
    packet->checksum = 0;
    packet->checksum = ip_checksum(packet, sizeof(ip_header_t));
    
    // Send packet
    int result = network_send_packet(interface, packet, total_length);
    
    kfree(packet);
    return result;
}

// Handle received IP packet
void ip_receive(network_interface_t* interface, ip_header_t* packet) {
    if (!interface || !packet) return;
    
    // Verify checksum
    uint16_t orig_checksum = packet->checksum;
    packet->checksum = 0;
    if (ip_checksum(packet, sizeof(ip_header_t)) != orig_checksum) {
        return; // Invalid checksum
    }
    
    // Handle based on protocol
    switch (packet->protocol) {
        case IP_PROTO_ICMP:
            icmp_receive(interface, packet);
            break;
            
        case IP_PROTO_TCP:
            // TODO: Implement TCP handling
            break;
            
        case IP_PROTO_UDP:
            // TODO: Implement UDP handling
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