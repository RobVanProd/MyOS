#ifndef SOUND_BUFFER_H
#define SOUND_BUFFER_H

#include <stdint.h>

// Sound buffer callback function type
typedef void (*sound_buffer_callback_t)(void* buffer, uint32_t length, void* user_data);

// Sound buffer functions
void sound_buffer_init(void);
void sound_buffer_set_callback(uint8_t channel, sound_buffer_callback_t callback, void* user_data);
void sound_buffer_clear_callback(uint8_t channel);
void sound_update(void);

// Sound buffer configuration
#define SOUND_BUFFER_SIZE 4096
#define SOUND_CHANNELS 2
#define SOUND_SAMPLE_RATE 44100
#define SOUND_BITS_PER_SAMPLE 16

// Sound buffer state
extern uint8_t* sound_buffer;
extern uint32_t sound_buffer_position;
extern sound_buffer_callback_t sound_callbacks[SOUND_CHANNELS];
extern void* sound_callback_data[SOUND_CHANNELS];

#endif // SOUND_BUFFER_H
