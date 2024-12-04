#ifndef DNS_H
#define DNS_H

#include <stdint.h>
#include "netstack.h"

// DNS header structure
typedef struct {
    uint16_t id;          // Query ID
    uint16_t flags;       // Flags
    uint16_t qdcount;     // Question count
    uint16_t ancount;     // Answer count
    uint16_t nscount;     // Authority count
    uint16_t arcount;     // Additional count
} __attribute__((packed)) dns_header_t;

// DNS question structure
typedef struct {
    uint16_t qtype;       // Query type
    uint16_t qclass;      // Query class
} __attribute__((packed)) dns_question_t;

// DNS resource record structure
typedef struct {
    uint16_t type;        // Record type
    uint16_t class;       // Record class
    uint32_t ttl;         // Time to live
    uint16_t rdlength;    // Data length
    uint8_t rdata[];      // Record data
} __attribute__((packed)) dns_record_t;

// DNS record types
#define DNS_TYPE_A     1  // IPv4 address
#define DNS_TYPE_NS    2  // Name server
#define DNS_TYPE_CNAME 5  // Canonical name
#define DNS_TYPE_SOA   6  // Start of authority
#define DNS_TYPE_PTR   12 // Pointer record
#define DNS_TYPE_MX    15 // Mail exchange
#define DNS_TYPE_TXT   16 // Text record
#define DNS_TYPE_AAAA  28 // IPv6 address

// DNS classes
#define DNS_CLASS_IN   1  // Internet

// DNS flags
#define DNS_FLAG_QR    0x8000  // Query/Response
#define DNS_FLAG_AA    0x0400  // Authoritative answer
#define DNS_FLAG_TC    0x0200  // Truncated
#define DNS_FLAG_RD    0x0100  // Recursion desired
#define DNS_FLAG_RA    0x0080  // Recursion available
#define DNS_FLAG_RCODE 0x000F  // Response code

// DNS response codes
#define DNS_RCODE_OK       0  // No error
#define DNS_RCODE_FORMAT   1  // Format error
#define DNS_RCODE_SERVER   2  // Server failure
#define DNS_RCODE_NAME     3  // Name error
#define DNS_RCODE_NOTIMPL  4  // Not implemented
#define DNS_RCODE_REFUSED  5  // Query refused

// DNS client structure
typedef struct {
    uint16_t next_id;
    uint32_t server_ip;
    net_interface_t* interface;
} dns_client_t;

// DNS query callback
typedef void (*dns_callback_t)(const char* name, uint32_t ip);

// DNS client functions
void dns_init(net_interface_t* interface, uint32_t server_ip);
void dns_cleanup(void);
int dns_resolve(const char* hostname, dns_callback_t callback);
void dns_handle_packet(const void* data, size_t length);

// DNS utility functions
int dns_encode_name(uint8_t* buffer, const char* name);
int dns_decode_name(const uint8_t* data, size_t length, char* name, size_t max_length);

#endif 