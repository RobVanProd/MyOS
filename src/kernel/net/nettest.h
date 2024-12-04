#ifndef NETTEST_H
#define NETTEST_H

#include <stdint.h>
#include "netstack.h"

// Test types
#define TEST_PING        1
#define TEST_TRACEROUTE  2
#define TEST_BANDWIDTH   3
#define TEST_LATENCY     4
#define TEST_PACKET_LOSS 5

// Test flags
#define TEST_FLAG_VERBOSE   0x01
#define TEST_FLAG_CONTINUOUS 0x02
#define TEST_FLAG_RAW_DATA  0x04

// Test result structure
typedef struct {
    uint32_t packets_sent;
    uint32_t packets_received;
    uint32_t bytes_sent;
    uint32_t bytes_received;
    uint32_t min_latency;
    uint32_t max_latency;
    uint32_t avg_latency;
    uint32_t bandwidth;
    uint32_t packet_loss;
    uint32_t errors;
} test_result_t;

// Test configuration structure
typedef struct {
    uint8_t test_type;
    uint8_t flags;
    uint32_t target_ip;
    uint16_t target_port;
    uint32_t duration;
    uint32_t interval;
    uint32_t packet_size;
    uint32_t packet_count;
    void (*progress_callback)(int percent);
    void (*result_callback)(const test_result_t* result);
} test_config_t;

// Initialize network testing
void nettest_init(void);
void nettest_cleanup(void);

// Test execution
int nettest_run(test_config_t* config);
void nettest_stop(void);
int nettest_is_running(void);

// Ping test
int nettest_ping(uint32_t target_ip, uint32_t count, uint32_t interval,
                void (*callback)(const test_result_t* result));

// Traceroute test
int nettest_traceroute(uint32_t target_ip,
                      void (*callback)(uint32_t hop, uint32_t ip, uint32_t latency));

// Bandwidth test
int nettest_bandwidth(uint32_t target_ip, uint16_t target_port,
                     uint32_t duration, uint32_t packet_size,
                     void (*callback)(const test_result_t* result));

// Latency test
int nettest_latency(uint32_t target_ip, uint16_t target_port,
                   uint32_t count, uint32_t interval,
                   void (*callback)(const test_result_t* result));

// Packet loss test
int nettest_packet_loss(uint32_t target_ip, uint32_t count,
                       uint32_t interval, uint32_t packet_size,
                       void (*callback)(const test_result_t* result));

// Test utilities
void nettest_format_result(const test_result_t* result, char* buffer, size_t size);
void nettest_dump_packet(const void* data, size_t length);

#endif 