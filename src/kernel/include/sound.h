#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>
#include "io.h"

// Sound buffer constants
#define MAX_SOUND_BUFFERS 16
#define MAX_SOUND_CHANNELS 2
#define MAX_SAMPLE_RATE 48000
#define DEFAULT_BUFFER_SIZE 4096

// Sound buffer states
#define BUFFER_STATE_FREE     0
#define BUFFER_STATE_STOPPED  1
#define BUFFER_STATE_PLAYING  2
#define BUFFER_STATE_PAUSED   3

// Sound formats
#define SOUND_FORMAT_PCM8     0
#define SOUND_FORMAT_PCM16    1

// PC Speaker ports
#define PIT_CHANNEL2 0x42
#define PIT_CONTROL  0x43
#define SPEAKER_PORT 0x61

// Sound callback function type
typedef void (*sound_callback_t)(void* buffer, uint32_t size);

// Sound buffer structure
typedef struct {
    uint8_t* data;
    uint32_t size;
    uint32_t position;
    uint8_t format;
    uint8_t channels;
    uint32_t sample_rate;
    uint8_t state;
    sound_callback_t callback;
} sound_buffer_t;

// Sound device structure
typedef struct sound_device {
    const char* name;
    uint32_t capabilities;
    int (*init)(void);
    int (*cleanup)(void);
    int (*play)(uint32_t buffer);
    int (*stop)(uint32_t buffer);
    int (*set_volume)(uint32_t buffer, uint8_t volume);
    struct sound_device* next;
} sound_device_t;

// Sound system initialization
void sound_init(void);

// Sound device management
int sound_device_register(sound_device_t* device);
int sound_device_unregister(sound_device_t* device);

// Sound buffer management
int sound_buffer_create(uint32_t size, uint8_t format, uint8_t channels, uint32_t sample_rate);
void sound_buffer_destroy(uint32_t buffer);
int sound_buffer_write(uint32_t buffer, const void* data, uint32_t size, uint32_t offset);
int sound_buffer_read(uint32_t buffer, void* data, uint32_t size, uint32_t offset);
void sound_buffer_set_callback(uint32_t buffer, sound_callback_t callback);

// Sound playback control
int sound_play(uint32_t buffer);
int sound_pause(uint32_t buffer);
int sound_stop(uint32_t buffer);
int sound_set_volume(uint32_t buffer, uint8_t volume);
uint8_t sound_get_volume(uint32_t buffer);

// Sound mixing
void sound_mix_buffers(void* output, uint32_t frames);
uint32_t sound_get_frame_size(uint8_t format, uint8_t channels);

// Legacy PC speaker functions
void play_sound(uint32_t frequency);
void stop_sound(void);
void beep(void);
void timer_wait(uint32_t ticks);

// Sound driver state
extern uint8_t sound_enabled;

#endif // SOUND_H
