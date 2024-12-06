#include "netstack.h"
#include "memory.h"
#include "terminal.h"
#include <string.h>

// Network interface list
static net_interface_t* interfaces = NULL;

// Socket list
static socket_t* sockets = NULL;

// Initialize network stack
void netstack_init(void) {
    interfaces = NULL;
    sockets = NULL;
}

// Cleanup network stack
void netstack_cleanup(void) {
    // Free all sockets
    socket_t* socket = sockets;
    while (socket) {
        socket_t* next = socket->next;
        netstack_socket_destroy(socket);
        socket = next;
    }
    
    // Free all interfaces
    net_interface_t* interface = interfaces;
    while (interface) {
        net_interface_t* next = interface->next;
        kfree(interface);
        interface = next;
    }
}

// Register network interface
int netstack_register_interface(net_interface_t* interface) {
    if (!interface) return -1;
    
    // Add to interface list
    interface->next = interfaces;
    interfaces = interface;
    
    return 0;
}

// Unregister network interface
void netstack_unregister_interface(net_interface_t* interface) {
    if (!interface) return;
    
    // Remove from interface list
    if (interfaces == interface) {
        interfaces = interface->next;
    } else {
        net_interface_t* prev = interfaces;
        while (prev && prev->next != interface) {
            prev = prev->next;
        }
        if (prev) {
            prev->next = interface->next;
        }
    }
}

// Get first network interface
net_interface_t* netstack_get_interface(void) {
    return interfaces;
}

// Calculate IP/TCP/UDP checksum
uint16_t netstack_checksum(const void* data, size_t length) {
    const uint16_t* ptr = (const uint16_t*)data;
    uint32_t sum = 0;
    
    while (length > 1) {
        sum += *ptr++;
        length -= 2;
    }
    
    if (length > 0) {
        sum += *(const uint8_t*)ptr;
    }
    
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return ~sum;
}

// Handle incoming network packet
void netstack_handle_packet(const void* data, size_t length) {
    if (length < sizeof(ethernet_header_t)) return;
    
    const ethernet_header_t* eth = (const ethernet_header_t*)data;
    uint16_t ethertype = ntohs(eth->ethertype);
    
    // Skip ethernet header
    data = (const uint8_t*)data + sizeof(ethernet_header_t);
    length -= sizeof(ethernet_header_t);
    
    switch (ethertype) {
        case ETH_TYPE_IP:
            netstack_handle_ipv4(data, length);
            break;
            
        case ETH_TYPE_ARP:
            netstack_handle_arp(data, length);
            break;
    }
}

// Handle IPv4 packet
void netstack_handle_ipv4(const void* data, size_t length) {
    if (length < sizeof(ipv4_header_t)) return;
    
    const ipv4_header_t* ip = (const ipv4_header_t*)data;
    uint8_t ihl = (ip->version_ihl & 0x0F) * 4;
    
    // Verify checksum
    if (netstack_checksum(ip, ihl) != 0) return;
    
    // Skip IP header
    data = (const uint8_t*)data + ihl;
    length -= ihl;
    
    switch (ip->protocol) {
        case IP_PROTO_ICMP:
            netstack_handle_icmp(data, length);
            break;
            
        case IP_PROTO_TCP:
            netstack_handle_tcp(data, length);
            break;
            
        case IP_PROTO_UDP:
            netstack_handle_udp(data, length);
            break;
    }
}

// Handle ARP packet
void netstack_handle_arp(const void* data, size_t length) {
    (void)data;   // Suppress unused parameter warning
    (void)length; // Suppress unused parameter warning
    // TODO: Implement ARP handling
}

// Handle ICMP packet
void netstack_handle_icmp(const void* data, size_t length) {
    if (length < sizeof(icmp_header_t)) return;
    
    const icmp_header_t* icmp = (const icmp_header_t*)data;
    
    // Verify checksum
    if (netstack_checksum(icmp, length) != 0) return;
    
    // Handle ICMP echo request
    if (icmp->type == 8 && icmp->code == 0) {
        // TODO: Send ICMP echo reply
    }
}

// Handle TCP packet
void netstack_handle_tcp(const void* data, size_t length) {
    if (length < sizeof(tcp_header_t)) return;
    
    const tcp_header_t* tcp = (const tcp_header_t*)data;
    uint16_t dest_port = ntohs(tcp->dest_port);
    
    // Find matching socket
    socket_t* socket = sockets;
    while (socket) {
        if (socket->protocol == IP_PROTO_TCP &&
            socket->local_port == dest_port &&
            (socket->state == SOCKET_LISTENING ||
             (socket->remote_port == dest_port))) {
            // Handle TCP packet for socket
            // TODO: Implement TCP state machine
            break;
        }
        socket = socket->next;
    }
}

// Handle UDP packet
void netstack_handle_udp(const void* data, size_t length) {
    if (length < sizeof(udp_header_t)) return;
    
    const udp_header_t* udp = (const udp_header_t*)data;
    uint16_t dest_port = ntohs(udp->dest_port);
    
    // Find matching socket
    socket_t* socket = sockets;
    while (socket) {
        if (socket->protocol == IP_PROTO_UDP &&
            socket->local_port == dest_port) {
            // Copy data to socket receive buffer
            size_t data_length = length - sizeof(udp_header_t);
            if (data_length > 0 && socket->rx_buffer) {
                memcpy(socket->rx_buffer,
                       (const uint8_t*)data + sizeof(udp_header_t),
                       data_length);
                socket->rx_size = data_length;
            }
            break;
        }
        socket = socket->next;
    }
}

// Send network packet
int netstack_send_packet(const void* data, size_t length) {
    net_interface_t* interface = netstack_get_interface();
    if (!interface) return -1;
    
    return interface->transmit(data, length);
}

// Create network socket
socket_t* netstack_socket_create(int protocol) {
    socket_t* socket = kmalloc(sizeof(socket_t));
    if (!socket) return NULL;
    
    memset(socket, 0, sizeof(socket_t));
    socket->protocol = protocol;
    
    // Add to socket list
    socket->next = sockets;
    sockets = socket;
    
    return socket;
}

// Destroy network socket
void netstack_socket_destroy(socket_t* socket) {
    if (!socket) return;
    
    // Remove from socket list
    if (sockets == socket) {
        sockets = socket->next;
    } else {
        socket_t* prev = sockets;
        while (prev && prev->next != socket) {
            prev = prev->next;
        }
        if (prev) {
            prev->next = socket->next;
        }
    }
    
    // Free buffers
    if (socket->rx_buffer) kfree(socket->rx_buffer);
    if (socket->tx_buffer) kfree(socket->tx_buffer);
    
    kfree(socket);
}

// Bind socket to local port
int netstack_socket_bind(socket_t* socket, uint16_t port) {
    if (!socket) return -1;
    
    // Check if port already in use
    socket_t* s = sockets;
    while (s) {
        if (s != socket && s->local_port == port) {
            return -1;
        }
        s = s->next;
    }
    
    socket->local_port = port;
    return 0;
}

// Connect socket to remote host
int netstack_socket_connect(socket_t* socket, uint32_t ip, uint16_t port) {
    if (!socket) return -1;
    
    socket->remote_ip = ip;
    socket->remote_port = port;
    
    if (socket->protocol == IP_PROTO_TCP) {
        // TODO: Implement TCP connection establishment
        return -1;
    }
    
    return 0;
}

// Listen for incoming connections
int netstack_socket_listen(socket_t* socket) {
    if (!socket || socket->protocol != IP_PROTO_TCP) return -1;
    
    socket->state = SOCKET_LISTENING;
    return 0;
}

// Accept incoming connection
socket_t* netstack_socket_accept(socket_t* socket) {
    if (!socket || socket->protocol != IP_PROTO_TCP ||
        socket->state != SOCKET_LISTENING) return NULL;
    
    // TODO: Implement TCP connection acceptance
    return NULL;
}

// Send data through socket
int netstack_socket_send(socket_t* socket, const void* data, size_t length) {
    if (!socket || !data || length == 0) return -1;
    
    if (socket->protocol == IP_PROTO_UDP) {
        // Construct UDP packet
        uint8_t packet[1500];
        udp_header_t* udp = (udp_header_t*)packet;
        
        udp->src_port = htons(socket->local_port);
        udp->dest_port = htons(socket->remote_port);
        udp->length = htons(sizeof(udp_header_t) + length);
        udp->checksum = 0; // Optional for UDP
        
        memcpy(packet + sizeof(udp_header_t), data, length);
        
        // Send packet
        return netstack_send_packet(packet, sizeof(udp_header_t) + length);
    } else if (socket->protocol == IP_PROTO_TCP) {
        // TODO: Implement TCP send
        return -1;
    }
    
    return -1;
}

// Receive data from socket
int netstack_socket_receive(socket_t* socket, void* buffer, size_t max_length) {
    if (!socket || !buffer || max_length == 0) return -1;
    
    if (socket->protocol == IP_PROTO_UDP) {
        // Copy data from receive buffer
        if (socket->rx_buffer && socket->rx_size > 0) {
            size_t length = socket->rx_size;
            if (length > max_length) length = max_length;
            
            memcpy(buffer, socket->rx_buffer, length);
            socket->rx_size = 0;
            
            return length;
        }
    } else if (socket->protocol == IP_PROTO_TCP) {
        // TODO: Implement TCP receive
        return -1;
    }
    
    return 0;
}

// Format MAC address string
void netstack_format_mac(char* buffer, const uint8_t* mac) {
    char temp[8];
    for (int i = 0; i < 6; i++) {
        int_to_hex_string(mac[i], temp);
        if (i > 0) {
            buffer[i*3-1] = ':';
        }
        buffer[i*3] = temp[0];
        buffer[i*3+1] = temp[1];
    }
    buffer[17] = '\0';
}

// Format IP address string
void netstack_format_ip(char* buffer, uint32_t ip) {
    int_to_string((ip >> 24) & 0xFF, buffer);
    int len = strlen(buffer);
    buffer[len] = '.';
    int_to_string((ip >> 16) & 0xFF, buffer + len + 1);
    len = strlen(buffer);
    buffer[len] = '.';
    int_to_string((ip >> 8) & 0xFF, buffer + len + 1);
    len = strlen(buffer);
    buffer[len] = '.';
    int_to_string(ip & 0xFF, buffer + len + 1);
}