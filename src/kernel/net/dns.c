#include "dns.h"
#include "../memory.h"
#include <string.h>

// DNS client instance
static dns_client_t dns_client;

// DNS port
#define DNS_PORT 53

// Maximum DNS packet size
#define DNS_MAX_PACKET_SIZE 512

// Maximum hostname length
#define DNS_MAX_NAME_LENGTH 256

// DNS query callback structure
typedef struct dns_query {
    uint16_t id;
    char hostname[DNS_MAX_NAME_LENGTH];
    dns_callback_t callback;
    struct dns_query* next;
} dns_query_t;

// List of pending queries
static dns_query_t* pending_queries = NULL;

// Initialize DNS client
void dns_init(net_interface_t* interface, uint32_t server_ip) {
    dns_client.next_id = 1;
    dns_client.server_ip = server_ip;
    dns_client.interface = interface;
    pending_queries = NULL;
}

// Cleanup DNS client
void dns_cleanup(void) {
    // Free pending queries
    dns_query_t* query = pending_queries;
    while (query) {
        dns_query_t* next = query->next;
        kfree(query);
        query = next;
    }
    pending_queries = NULL;
}

// Encode DNS name
int dns_encode_name(uint8_t* buffer, const char* name) {
    uint8_t* ptr = buffer;
    const char* label = name;
    
    while (*label) {
        // Find label length
        const char* end = strchr(label, '.');
        size_t len = end ? (size_t)(end - label) : strlen(label);
        
        // Check label length
        if (len > 63) return -1;
        
        // Write length byte
        *ptr++ = len;
        
        // Write label
        memcpy(ptr, label, len);
        ptr += len;
        
        // Move to next label
        if (end) {
            label = end + 1;
        } else {
            break;
        }
    }
    
    // Write terminating zero
    *ptr++ = 0;
    
    return ptr - buffer;
}

// Decode DNS name
int dns_decode_name(const uint8_t* data, size_t length, char* name, size_t max_length) {
    const uint8_t* ptr = data;
    const uint8_t* end = data + length;
    char* out = name;
    size_t total = 0;
    int jumped = 0;
    
    while (ptr < end) {
        // Check for compression pointer
        if ((*ptr & 0xC0) == 0xC0) {
            if (ptr + 2 > end) return -1;
            
            uint16_t offset = ((ptr[0] & 0x3F) << 8) | ptr[1];
            if (offset >= length) return -1;
            
            if (!jumped) {
                ptr += 2;
                jumped = 1;
            }
            ptr = data + offset;
            continue;
        }
        
        // Get label length
        uint8_t len = *ptr++;
        if (len == 0) break;
        
        // Check buffer space
        if (total + len + 1 > max_length) return -1;
        
        // Copy label
        if (total > 0) {
            *out++ = '.';
            total++;
        }
        memcpy(out, ptr, len);
        out += len;
        total += len;
        ptr += len;
    }
    
    // Null terminate
    if (total < max_length) {
        *out = '\0';
        return total;
    }
    
    return -1;
}

// Send DNS query
static int dns_send_query(const char* hostname, uint16_t id) {
    uint8_t packet[DNS_MAX_PACKET_SIZE];
    uint8_t* ptr = packet;
    
    // Create DNS header
    dns_header_t* header = (dns_header_t*)ptr;
    header->id = htons(id);
    header->flags = htons(DNS_FLAG_RD); // Recursion desired
    header->qdcount = htons(1);
    header->ancount = 0;
    header->nscount = 0;
    header->arcount = 0;
    ptr += sizeof(dns_header_t);
    
    // Encode hostname
    int name_len = dns_encode_name(ptr, hostname);
    if (name_len < 0) return -1;
    ptr += name_len;
    
    // Add question section
    dns_question_t* question = (dns_question_t*)ptr;
    question->qtype = htons(DNS_TYPE_A);
    question->qclass = htons(DNS_CLASS_IN);
    ptr += sizeof(dns_question_t);
    
    // Create UDP packet
    uint8_t udp_packet[DNS_MAX_PACKET_SIZE + sizeof(udp_header_t)];
    udp_header_t* udp = (udp_header_t*)udp_packet;
    
    udp->src_port = htons(DNS_PORT);
    udp->dest_port = htons(DNS_PORT);
    udp->length = htons(ptr - packet + sizeof(udp_header_t));
    udp->checksum = 0;
    
    memcpy(udp_packet + sizeof(udp_header_t), packet, ptr - packet);
    
    // Send packet
    return netstack_send_packet(udp_packet, ptr - packet + sizeof(udp_header_t));
}

// Resolve hostname
int dns_resolve(const char* hostname, dns_callback_t callback) {
    if (!hostname || !callback) return -1;
    
    // Create query structure
    dns_query_t* query = kmalloc(sizeof(dns_query_t));
    if (!query) return -1;
    
    query->id = dns_client.next_id++;
    strncpy(query->hostname, hostname, DNS_MAX_NAME_LENGTH - 1);
    query->hostname[DNS_MAX_NAME_LENGTH - 1] = '\0';
    query->callback = callback;
    
    // Add to pending queries
    query->next = pending_queries;
    pending_queries = query;
    
    // Send query
    return dns_send_query(hostname, query->id);
}

// Handle DNS packet
void dns_handle_packet(const void* data, size_t length) {
    if (length < sizeof(dns_header_t)) return;
    
    const dns_header_t* header = (const dns_header_t*)data;
    uint16_t id = ntohs(header->id);
    
    // Find matching query
    dns_query_t* prev = NULL;
    dns_query_t* query = pending_queries;
    
    while (query) {
        if (query->id == id) {
            // Parse response
            if (ntohs(header->flags) & DNS_FLAG_QR) {
                const uint8_t* ptr = (const uint8_t*)data + sizeof(dns_header_t);
                const uint8_t* end = (const uint8_t*)data + length;
                
                // Skip question section
                int name_len = 0;
                while (ptr < end && *ptr) {
                    if ((*ptr & 0xC0) == 0xC0) {
                        ptr += 2;
                        break;
                    }
                    uint8_t len = *ptr++;
                    ptr += len;
                }
                ptr += sizeof(dns_question_t);
                
                // Parse answer section
                uint16_t ancount = ntohs(header->ancount);
                while (ancount > 0 && ptr < end) {
                    // Skip name
                    while (ptr < end && *ptr) {
                        if ((*ptr & 0xC0) == 0xC0) {
                            ptr += 2;
                            break;
                        }
                        uint8_t len = *ptr++;
                        ptr += len;
                    }
                    
                    // Parse record
                    const dns_record_t* record = (const dns_record_t*)ptr;
                    ptr += sizeof(dns_record_t);
                    
                    if (ntohs(record->type) == DNS_TYPE_A &&
                        ntohs(record->class) == DNS_CLASS_IN &&
                        ntohs(record->rdlength) == 4) {
                        // Found IPv4 address
                        uint32_t ip = ntohl(*(uint32_t*)ptr);
                        query->callback(query->hostname, ip);
                        break;
                    }
                    
                    ptr += ntohs(record->rdlength);
                    ancount--;
                }
            }
            
            // Remove query from list
            if (prev) {
                prev->next = query->next;
            } else {
                pending_queries = query->next;
            }
            
            kfree(query);
            break;
        }
        
        prev = query;
        query = query->next;
    }
} 