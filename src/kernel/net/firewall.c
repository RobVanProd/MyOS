#include "firewall.h"
#include "../memory.h"
#include <string.h>

// Firewall rule chain
static fw_rule_t* rule_chain = NULL;

// Firewall statistics
static fw_stats_t fw_stats;

// Initialize firewall
void firewall_init(void) {
    rule_chain = NULL;
    memset(&fw_stats, 0, sizeof(fw_stats_t));
}

// Cleanup firewall
void firewall_cleanup(void) {
    firewall_clear_rules();
}

// Add firewall rule
int firewall_add_rule(uint8_t action, uint8_t direction, uint8_t protocol,
                     uint32_t src_ip, uint32_t src_mask,
                     uint32_t dst_ip, uint32_t dst_mask,
                     uint16_t src_port, uint16_t dst_port,
                     uint32_t priority) {
    // Create new rule
    fw_rule_t* rule = kmalloc(sizeof(fw_rule_t));
    if (!rule) return -1;
    
    rule->action = action;
    rule->direction = direction;
    rule->protocol = protocol;
    rule->src_ip = src_ip;
    rule->src_mask = src_mask;
    rule->dst_ip = dst_ip;
    rule->dst_mask = dst_mask;
    rule->src_port = src_port;
    rule->dst_port = dst_port;
    rule->priority = priority;
    
    // Insert rule in priority order
    if (!rule_chain || rule_chain->priority > priority) {
        rule->next = rule_chain;
        rule_chain = rule;
    } else {
        fw_rule_t* current = rule_chain;
        while (current->next && current->next->priority <= priority) {
            current = current->next;
        }
        rule->next = current->next;
        current->next = rule;
    }
    
    return 0;
}

// Remove firewall rule
int firewall_remove_rule(uint32_t priority) {
    fw_rule_t* prev = NULL;
    fw_rule_t* rule = rule_chain;
    
    while (rule) {
        if (rule->priority == priority) {
            if (prev) {
                prev->next = rule->next;
            } else {
                rule_chain = rule->next;
            }
            kfree(rule);
            return 0;
        }
        prev = rule;
        rule = rule->next;
    }
    
    return -1;
}

// Clear all firewall rules
void firewall_clear_rules(void) {
    fw_rule_t* rule = rule_chain;
    while (rule) {
        fw_rule_t* next = rule->next;
        kfree(rule);
        rule = next;
    }
    rule_chain = NULL;
}

// Check if IP matches rule
static int ip_matches_rule(uint32_t ip, uint32_t rule_ip, uint32_t rule_mask) {
    return (ip & rule_mask) == (rule_ip & rule_mask);
}

// Filter packet through firewall
int firewall_filter_packet(const void* data, size_t length, int direction) {
    if (length < sizeof(ethernet_header_t)) return FW_ACTION_DROP;
    
    const ethernet_header_t* eth = (const ethernet_header_t*)data;
    uint16_t ethertype = ntohs(eth->ethertype);
    
    // Skip non-IP packets
    if (ethertype != ETH_TYPE_IP) return FW_ACTION_ACCEPT;
    
    // Check IP header
    const uint8_t* ip_data = (const uint8_t*)data + sizeof(ethernet_header_t);
    size_t ip_length = length - sizeof(ethernet_header_t);
    
    if (ip_length < sizeof(ipv4_header_t)) return FW_ACTION_DROP;
    
    const ipv4_header_t* ip = (const ipv4_header_t*)ip_data;
    uint32_t src_ip = ntohl(ip->src_ip);
    uint32_t dst_ip = ntohl(ip->dest_ip);
    uint8_t protocol = ip->protocol;
    
    // Get port numbers for TCP/UDP
    uint16_t src_port = 0;
    uint16_t dst_port = 0;
    
    if (protocol == IP_PROTO_TCP) {
        if (ip_length < sizeof(ipv4_header_t) + sizeof(tcp_header_t)) {
            return FW_ACTION_DROP;
        }
        const tcp_header_t* tcp = (const tcp_header_t*)(ip_data + sizeof(ipv4_header_t));
        src_port = ntohs(tcp->src_port);
        dst_port = ntohs(tcp->dest_port);
    } else if (protocol == IP_PROTO_UDP) {
        if (ip_length < sizeof(ipv4_header_t) + sizeof(udp_header_t)) {
            return FW_ACTION_DROP;
        }
        const udp_header_t* udp = (const udp_header_t*)(ip_data + sizeof(ipv4_header_t));
        src_port = ntohs(udp->src_port);
        dst_port = ntohs(udp->dest_port);
    }
    
    // Check rules
    fw_rule_t* rule = rule_chain;
    while (rule) {
        // Check direction
        if (rule->direction == direction || rule->direction == FW_DIR_BOTH) {
            // Check protocol
            if (rule->protocol == FW_PROTO_ANY || rule->protocol == protocol) {
                // Check IP addresses
                if (ip_matches_rule(src_ip, rule->src_ip, rule->src_mask) &&
                    ip_matches_rule(dst_ip, rule->dst_ip, rule->dst_mask)) {
                    // Check ports for TCP/UDP
                    if (protocol == IP_PROTO_TCP || protocol == IP_PROTO_UDP) {
                        if ((rule->src_port == 0 || rule->src_port == src_port) &&
                            (rule->dst_port == 0 || rule->dst_port == dst_port)) {
                            // Rule matches
                            switch (rule->action) {
                                case FW_ACTION_ACCEPT:
                                    fw_stats.packets_accepted++;
                                    fw_stats.bytes_accepted += length;
                                    return FW_ACTION_ACCEPT;
                                    
                                case FW_ACTION_DROP:
                                    fw_stats.packets_dropped++;
                                    fw_stats.bytes_dropped += length;
                                    return FW_ACTION_DROP;
                                    
                                case FW_ACTION_REJECT:
                                    fw_stats.packets_rejected++;
                                    fw_stats.bytes_rejected += length;
                                    return FW_ACTION_REJECT;
                            }
                        }
                    } else {
                        // Rule matches for non-TCP/UDP
                        switch (rule->action) {
                            case FW_ACTION_ACCEPT:
                                fw_stats.packets_accepted++;
                                fw_stats.bytes_accepted += length;
                                return FW_ACTION_ACCEPT;
                                
                            case FW_ACTION_DROP:
                                fw_stats.packets_dropped++;
                                fw_stats.bytes_dropped += length;
                                return FW_ACTION_DROP;
                                
                            case FW_ACTION_REJECT:
                                fw_stats.packets_rejected++;
                                fw_stats.bytes_rejected += length;
                                return FW_ACTION_REJECT;
                        }
                    }
                }
            }
        }
        rule = rule->next;
    }
    
    // Default action: accept
    fw_stats.packets_accepted++;
    fw_stats.bytes_accepted += length;
    return FW_ACTION_ACCEPT;
}

// Get firewall statistics
void firewall_get_stats(fw_stats_t* stats) {
    memcpy(stats, &fw_stats, sizeof(fw_stats_t));
}

// Reset firewall statistics
void firewall_reset_stats(void) {
    memset(&fw_stats, 0, sizeof(fw_stats_t));
}

// Dump firewall rules
void firewall_dump_rules(void) {
    fw_rule_t* rule = rule_chain;
    int count = 0;
    
    printf("Firewall Rules:\n");
    printf("Priority  Action   Direction  Protocol  Source IP/Mask          Destination IP/Mask      Ports\n");
    printf("---------------------------------------------------------------------------------\n");
    
    while (rule) {
        char src_ip[32], dst_ip[32];
        char src_mask[32], dst_mask[32];
        
        netstack_format_ip(src_ip, rule->src_ip);
        netstack_format_ip(src_mask, rule->src_mask);
        netstack_format_ip(dst_ip, rule->dst_ip);
        netstack_format_ip(dst_mask, rule->dst_mask);
        
        printf("%-8d  %-7s  %-9s  %-8s  %-15s/%-15s  %-15s/%-15s  %d->%d\n",
               rule->priority,
               rule->action == FW_ACTION_ACCEPT ? "ACCEPT" :
               rule->action == FW_ACTION_DROP ? "DROP" : "REJECT",
               rule->direction == FW_DIR_IN ? "IN" :
               rule->direction == FW_DIR_OUT ? "OUT" : "BOTH",
               rule->protocol == FW_PROTO_ANY ? "ANY" :
               rule->protocol == FW_PROTO_ICMP ? "ICMP" :
               rule->protocol == FW_PROTO_TCP ? "TCP" :
               rule->protocol == FW_PROTO_UDP ? "UDP" : "???",
               src_ip, src_mask,
               dst_ip, dst_mask,
               rule->src_port, rule->dst_port);
        
        rule = rule->next;
        count++;
    }
    
    printf("---------------------------------------------------------------------------------\n");
    printf("Total rules: %d\n", count);
}

// Load firewall rules from file
int firewall_load_rules(const char* filename) {
    // TODO: Implement rule loading from file
    return -1;
}

// Save firewall rules to file
int firewall_save_rules(const char* filename) {
    // TODO: Implement rule saving to file
    return -1;
} 