#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>

// Sound formats
#define SOUND_FORMAT_PCM8  1
#define SOUND_FORMAT_PCM16 2

// Sound channels
#define SOUND_CHANNEL_MONO   1
#define SOUND_CHANNEL_STEREO 2

// Sample rates
#define SOUND_RATE_8000  8000
#define SOUND_RATE_11025 11025
#define SOUND_RATE_22050 22050
#define SOUND_RATE_44100 44100

// Maximum number of sound buffers
#define MAX_SOUND_BUFFERS 16

// Sound buffer states
#define BUFFER_STATE_FREE     0
#define BUFFER_STATE_PLAYING  1
#define BUFFER_STATE_PAUSED   2
#define BUFFER_STATE_STOPPED  3

// Sound buffer structure
typedef struct {
    uint8_t* data;
    uint32_t size;
    uint32_t position;
    uint8_t format;
    uint8_t channels;
    uint32_t sample_rate;
    uint8_t state;
    void (*callback)(void*);
    void* user_data;
} sound_buffer_t;

// Sound device structure
typedef struct {
    char name[32];
    uint8_t formats;
    uint8_t channels;
    uint32_t rates;
    void* driver_data;
} sound_device_t;

// Sound functions
void sound_init(void);
int sound_device_register(sound_device_t* device);
int sound_device_unregister(sound_device_t* device);

// Buffer management
int sound_buffer_create(uint8_t format, uint8_t channels, uint32_t sample_rate, uint32_t size);
void sound_buffer_destroy(int buffer);
int sound_buffer_write(int buffer, const void* data, uint32_t size);
int sound_buffer_read(int buffer, void* data, uint32_t size);
void sound_buffer_set_callback(int buffer, void (*callback)(void*), void* user_data);

// Playback control
int sound_play(int buffer);
int sound_pause(int buffer);
int sound_stop(int buffer);
int sound_rewind(int buffer);
int sound_set_position(int buffer, uint32_t position);
uint32_t sound_get_position(int buffer);

// Volume control
int sound_set_volume(int buffer, uint8_t volume);
uint8_t sound_get_volume(int buffer);

// Mixer functions
void sound_mix_buffers(int16_t* output, uint32_t frames);
void sound_update(void);

// Utility functions
uint32_t sound_bytes_to_frames(uint8_t format, uint8_t channels, uint32_t bytes);
uint32_t sound_frames_to_bytes(uint8_t format, uint8_t channels, uint32_t frames);
uint32_t sound_get_frame_size(uint8_t format, uint8_t channels);

#endif 