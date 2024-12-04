#include "nettest.h"
#include "../memory.h"
#include <string.h>
#include <stdio.h>

// Test state
static struct {
    uint8_t running;
    uint8_t stop_requested;
    test_config_t config;
    test_result_t result;
    uint32_t start_time;
    uint32_t last_time;
} test_state;

// Initialize network testing
void nettest_init(void) {
    memset(&test_state, 0, sizeof(test_state));
}

// Cleanup network testing
void nettest_cleanup(void) {
    nettest_stop();
}

// Get current timestamp in milliseconds
static uint32_t get_timestamp(void) {
    // TODO: Implement proper timestamp
    static uint32_t timestamp = 0;
    return timestamp++;
}

// Send ICMP echo request
static int send_ping(uint32_t target_ip, uint16_t seq, uint32_t size) {
    uint8_t packet[1500];
    icmp_header_t* icmp = (icmp_header_t*)packet;
    
    // Fill ICMP header
    icmp->type = 8; // Echo request
    icmp->code = 0;
    icmp->checksum = 0;
    icmp->rest = htonl((get_timestamp() << 16) | seq);
    
    // Fill data
    uint8_t* data = packet + sizeof(icmp_header_t);
    for (uint32_t i = 0; i < size; i++) {
        data[i] = i & 0xFF;
    }
    
    // Calculate checksum
    icmp->checksum = netstack_checksum(packet, sizeof(icmp_header_t) + size);
    
    // Send packet
    return netstack_send_packet(packet, sizeof(icmp_header_t) + size);
}

// Handle ICMP echo reply
static void handle_ping_reply(const void* data, size_t length) {
    const icmp_header_t* icmp = (const icmp_header_t*)data;
    
    if (icmp->type == 0 && icmp->code == 0) { // Echo reply
        uint32_t timestamp = ntohl(icmp->rest) >> 16;
        uint16_t seq = ntohl(icmp->rest) & 0xFFFF;
        uint32_t latency = get_timestamp() - timestamp;
        
        // Update statistics
        test_state.result.packets_received++;
        test_state.result.bytes_received += length;
        
        if (latency < test_state.result.min_latency || 
            test_state.result.min_latency == 0) {
            test_state.result.min_latency = latency;
        }
        if (latency > test_state.result.max_latency) {
            test_state.result.max_latency = latency;
        }
        test_state.result.avg_latency = 
            (test_state.result.avg_latency * (test_state.result.packets_received - 1) +
             latency) / test_state.result.packets_received;
    }
}

// Run ping test
static int run_ping_test(void) {
    uint32_t count = test_state.config.packet_count;
    uint32_t interval = test_state.config.interval;
    
    for (uint32_t i = 0; i < count && !test_state.stop_requested; i++) {
        // Send ping
        if (send_ping(test_state.config.target_ip, i, test_state.config.packet_size) < 0) {
            test_state.result.errors++;
            continue;
        }
        
        test_state.result.packets_sent++;
        test_state.result.bytes_sent += test_state.config.packet_size;
        
        // Wait for reply
        uint32_t timeout = get_timestamp() + 1000; // 1 second timeout
        while (get_timestamp() < timeout && !test_state.stop_requested) {
            // TODO: Handle incoming packets
        }
        
        // Update progress
        if (test_state.config.progress_callback) {
            test_state.config.progress_callback((i + 1) * 100 / count);
        }
        
        // Wait for interval
        while (get_timestamp() < test_state.last_time + interval &&
               !test_state.stop_requested);
        test_state.last_time = get_timestamp();
    }
    
    // Calculate packet loss
    test_state.result.packet_loss = 
        ((test_state.result.packets_sent - test_state.result.packets_received) * 100) /
        test_state.result.packets_sent;
    
    return 0;
}

// Run traceroute test
static int run_traceroute_test(void) {
    uint8_t ttl = 1;
    uint32_t max_hops = 30;
    
    while (ttl <= max_hops && !test_state.stop_requested) {
        // TODO: Send probe with current TTL
        
        // Wait for reply or timeout
        uint32_t timeout = get_timestamp() + 1000;
        while (get_timestamp() < timeout && !test_state.stop_requested) {
            // TODO: Handle incoming packets
        }
        
        // Update progress
        if (test_state.config.progress_callback) {
            test_state.config.progress_callback(ttl * 100 / max_hops);
        }
        
        ttl++;
    }
    
    return 0;
}

// Run bandwidth test
static int run_bandwidth_test(void) {
    uint32_t duration = test_state.config.duration;
    uint32_t end_time = test_state.start_time + duration;
    
    while (get_timestamp() < end_time && !test_state.stop_requested) {
        // Send packet
        uint8_t packet[1500];
        memset(packet, 0, test_state.config.packet_size);
        
        if (netstack_send_packet(packet, test_state.config.packet_size) < 0) {
            test_state.result.errors++;
            continue;
        }
        
        test_state.result.packets_sent++;
        test_state.result.bytes_sent += test_state.config.packet_size;
        
        // Update progress
        if (test_state.config.progress_callback) {
            uint32_t elapsed = get_timestamp() - test_state.start_time;
            test_state.config.progress_callback(elapsed * 100 / duration);
        }
    }
    
    // Calculate bandwidth
    uint32_t elapsed = get_timestamp() - test_state.start_time;
    if (elapsed > 0) {
        test_state.result.bandwidth = 
            (test_state.result.bytes_sent * 8) / (elapsed / 1000); // bits per second
    }
    
    return 0;
}

// Run latency test
static int run_latency_test(void) {
    return run_ping_test(); // Similar to ping test
}

// Run packet loss test
static int run_packet_loss_test(void) {
    return run_ping_test(); // Similar to ping test
}

// Run network test
int nettest_run(test_config_t* config) {
    if (!config || test_state.running) return -1;
    
    // Initialize test
    memcpy(&test_state.config, config, sizeof(test_config_t));
    memset(&test_state.result, 0, sizeof(test_result_t));
    test_state.result.min_latency = 0xFFFFFFFF;
    test_state.stop_requested = 0;
    test_state.running = 1;
    
    test_state.start_time = get_timestamp();
    test_state.last_time = test_state.start_time;
    
    // Run test
    int result = -1;
    switch (config->test_type) {
        case TEST_PING:
            result = run_ping_test();
            break;
            
        case TEST_TRACEROUTE:
            result = run_traceroute_test();
            break;
            
        case TEST_BANDWIDTH:
            result = run_bandwidth_test();
            break;
            
        case TEST_LATENCY:
            result = run_latency_test();
            break;
            
        case TEST_PACKET_LOSS:
            result = run_packet_loss_test();
            break;
    }
    
    // Call result callback
    if (config->result_callback) {
        config->result_callback(&test_state.result);
    }
    
    test_state.running = 0;
    return result;
}

// Stop network test
void nettest_stop(void) {
    test_state.stop_requested = 1;
}

// Check if test is running
int nettest_is_running(void) {
    return test_state.running;
}

// Run ping test
int nettest_ping(uint32_t target_ip, uint32_t count, uint32_t interval,
                void (*callback)(const test_result_t* result)) {
    test_config_t config = {
        .test_type = TEST_PING,
        .target_ip = target_ip,
        .packet_count = count,
        .interval = interval,
        .packet_size = 64,
        .result_callback = callback
    };
    return nettest_run(&config);
}

// Run traceroute test
int nettest_traceroute(uint32_t target_ip,
                      void (*callback)(uint32_t hop, uint32_t ip, uint32_t latency)) {
    test_config_t config = {
        .test_type = TEST_TRACEROUTE,
        .target_ip = target_ip,
        .packet_size = 64
    };
    return nettest_run(&config);
}

// Run bandwidth test
int nettest_bandwidth(uint32_t target_ip, uint16_t target_port,
                     uint32_t duration, uint32_t packet_size,
                     void (*callback)(const test_result_t* result)) {
    test_config_t config = {
        .test_type = TEST_BANDWIDTH,
        .target_ip = target_ip,
        .target_port = target_port,
        .duration = duration,
        .packet_size = packet_size,
        .result_callback = callback
    };
    return nettest_run(&config);
}

// Run latency test
int nettest_latency(uint32_t target_ip, uint16_t target_port,
                   uint32_t count, uint32_t interval,
                   void (*callback)(const test_result_t* result)) {
    test_config_t config = {
        .test_type = TEST_LATENCY,
        .target_ip = target_ip,
        .target_port = target_port,
        .packet_count = count,
        .interval = interval,
        .packet_size = 64,
        .result_callback = callback
    };
    return nettest_run(&config);
}

// Run packet loss test
int nettest_packet_loss(uint32_t target_ip, uint32_t count,
                       uint32_t interval, uint32_t packet_size,
                       void (*callback)(const test_result_t* result)) {
    test_config_t config = {
        .test_type = TEST_PACKET_LOSS,
        .target_ip = target_ip,
        .packet_count = count,
        .interval = interval,
        .packet_size = packet_size,
        .result_callback = callback
    };
    return nettest_run(&config);
}

// Format test result
void nettest_format_result(const test_result_t* result, char* buffer, size_t size) {
    snprintf(buffer, size,
             "Test Results:\n"
             "  Packets: sent=%u, received=%u, lost=%u (%.1f%% loss)\n"
             "  Bytes: sent=%u, received=%u\n"
             "  Latency: min=%ums, avg=%ums, max=%ums\n"
             "  Bandwidth: %u bits/sec\n"
             "  Errors: %u\n",
             result->packets_sent,
             result->packets_received,
             result->packets_sent - result->packets_received,
             result->packet_loss,
             result->bytes_sent,
             result->bytes_received,
             result->min_latency,
             result->avg_latency,
             result->max_latency,
             result->bandwidth,
             result->errors);
}

// Dump packet contents
void nettest_dump_packet(const void* data, size_t length) {
    const uint8_t* bytes = (const uint8_t*)data;
    
    printf("Packet dump (%zu bytes):\n", length);
    for (size_t i = 0; i < length; i += 16) {
        printf("%04zx: ", i);
        
        // Print hex values
        for (size_t j = 0; j < 16; j++) {
            if (i + j < length) {
                printf("%02x ", bytes[i + j]);
            } else {
                printf("   ");
            }
        }
        
        printf(" ");
        
        // Print ASCII values
        for (size_t j = 0; j < 16; j++) {
            if (i + j < length) {
                uint8_t c = bytes[i + j];
                printf("%c", (c >= 32 && c <= 126) ? c : '.');
            }
        }
        
        printf("\n");
    }
} 