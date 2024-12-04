#ifndef FIREWALL_H
#define FIREWALL_H

#include <stdint.h>
#include "netstack.h"

// Firewall rule actions
#define FW_ACTION_ACCEPT 1
#define FW_ACTION_DROP   2
#define FW_ACTION_REJECT 3

// Firewall rule directions
#define FW_DIR_IN    1
#define FW_DIR_OUT   2
#define FW_DIR_BOTH  3

// Firewall rule protocols
#define FW_PROTO_ANY  0
#define FW_PROTO_ICMP 1
#define FW_PROTO_TCP  6
#define FW_PROTO_UDP  17

// Firewall rule structure
typedef struct fw_rule {
    uint8_t action;        // Rule action
    uint8_t direction;     // Rule direction
    uint8_t protocol;      // Protocol
    uint32_t src_ip;       // Source IP
    uint32_t src_mask;     // Source netmask
    uint32_t dst_ip;       // Destination IP
    uint32_t dst_mask;     // Destination netmask
    uint16_t src_port;     // Source port
    uint16_t dst_port;     // Destination port
    uint32_t priority;     // Rule priority
    struct fw_rule* next;  // Next rule in chain
} fw_rule_t;

// Firewall statistics
typedef struct {
    uint64_t packets_accepted;
    uint64_t packets_dropped;
    uint64_t packets_rejected;
    uint64_t bytes_accepted;
    uint64_t bytes_dropped;
    uint64_t bytes_rejected;
} fw_stats_t;

// Initialize firewall
void firewall_init(void);
void firewall_cleanup(void);

// Rule management
int firewall_add_rule(uint8_t action, uint8_t direction, uint8_t protocol,
                     uint32_t src_ip, uint32_t src_mask,
                     uint32_t dst_ip, uint32_t dst_mask,
                     uint16_t src_port, uint16_t dst_port,
                     uint32_t priority);
int firewall_remove_rule(uint32_t priority);
void firewall_clear_rules(void);

// Packet filtering
int firewall_filter_packet(const void* data, size_t length, int direction);

// Statistics
void firewall_get_stats(fw_stats_t* stats);
void firewall_reset_stats(void);

// Rule management utilities
void firewall_dump_rules(void);
int firewall_load_rules(const char* filename);
int firewall_save_rules(const char* filename);

#endif 