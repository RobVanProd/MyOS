#include "sound.h"
#include "memory.h"
#include "string.h"
#include "kheap.h"

// Sound globals
static sound_device_t* current_device = NULL;
static sound_buffer_t buffers[MAX_SOUND_BUFFERS] = {0};
static uint8_t buffer_volumes[MAX_SOUND_BUFFERS] = {255}; // Default full volume

// Initialize sound system
void sound_init(void) {
    // Initialize buffer states
    for (int i = 0; i < MAX_SOUND_BUFFERS; i++) {
        buffers[i].state = BUFFER_STATE_FREE;
        buffers[i].data = NULL;
        buffers[i].callback = NULL;
        buffer_volumes[i] = 255;
    }
}

// Update sound system
void sound_update(void) {
    if (!current_device) return;

    // Process active buffers
    for (int i = 0; i < MAX_SOUND_BUFFERS; i++) {
        if (buffers[i].state == BUFFER_STATE_PLAYING) {
            // Update buffer position
            uint32_t frame_size = sound_get_frame_size(buffers[i].format, buffers[i].channels);
            buffers[i].position += frame_size;

            // Check for buffer end
            if (buffers[i].position >= buffers[i].size) {
                if (buffers[i].callback) {
                    // Call buffer callback
                    buffers[i].callback(buffers[i].data, buffers[i].size);
                }
                // Reset or stop buffer
                buffers[i].position = 0;
            }
        }
    }

    // Mix and output audio
    uint8_t mix_buffer[DEFAULT_BUFFER_SIZE];
    sound_mix_buffers(mix_buffer, DEFAULT_BUFFER_SIZE / 4); // Assuming stereo 16-bit
}

// Register a sound device
int sound_device_register(sound_device_t* device) {
    if (!device) return -1;
    
    // If no current device, make this the current
    if (!current_device) {
        current_device = device;
    }
    
    return 0;
}

// Unregister a sound device
int sound_device_unregister(sound_device_t* device) {
    if (!device) return -1;
    
    if (current_device == device) {
        current_device = NULL;
    }
    
    return 0;
}

// Create a sound buffer
int sound_buffer_create(uint32_t size, uint8_t format, uint8_t channels, uint32_t sample_rate) {
    // Find free buffer
    int buffer_id = -1;
    for (int i = 0; i < MAX_SOUND_BUFFERS; i++) {
        if (buffers[i].state == BUFFER_STATE_FREE) {
            buffer_id = i;
            break;
        }
    }
    
    if (buffer_id < 0) {
        return -1;  // No free buffers
    }
    
    // Allocate buffer memory
    buffers[buffer_id].data = kmalloc(size);
    if (!buffers[buffer_id].data) {
        return -1;  // Memory allocation failed
    }
    
    // Initialize buffer properties
    buffers[buffer_id].size = size;
    buffers[buffer_id].position = 0;
    buffers[buffer_id].format = format;
    buffers[buffer_id].channels = channels;
    buffers[buffer_id].sample_rate = sample_rate;
    buffers[buffer_id].state = BUFFER_STATE_STOPPED;
    buffers[buffer_id].callback = NULL;
    
    return buffer_id;
}

// Destroy a sound buffer
void sound_buffer_destroy(uint32_t buffer) {
    if (buffer >= MAX_SOUND_BUFFERS) return;
    if (buffers[buffer].state == BUFFER_STATE_FREE) return;
    
    // Stop if playing
    if (buffers[buffer].state == BUFFER_STATE_PLAYING) {
        sound_stop(buffer);
    }
    
    // Free buffer memory
    if (buffers[buffer].data) {
        kfree(buffers[buffer].data);
        buffers[buffer].data = NULL;
    }
    
    // Reset buffer state
    buffers[buffer].state = BUFFER_STATE_FREE;
    buffers[buffer].callback = NULL;
}

// Write data to buffer
int sound_buffer_write(uint32_t buffer, const void* data, uint32_t size, uint32_t offset) {
    if (buffer >= MAX_SOUND_BUFFERS) return -1;
    if (buffers[buffer].state == BUFFER_STATE_FREE) return -1;
    if (!data || !size) return -1;
    
    // Check if write would exceed buffer size
    if (offset + size > buffers[buffer].size) {
        return -1;
    }
    
    // Copy data to buffer
    memcpy((uint8_t*)buffers[buffer].data + offset, data, size);
    return size;
}

// Read data from buffer
int sound_buffer_read(uint32_t buffer, void* data, uint32_t size, uint32_t offset) {
    if (buffer >= MAX_SOUND_BUFFERS) return -1;
    if (buffers[buffer].state == BUFFER_STATE_FREE) return -1;
    if (!data || !size) return -1;
    
    // Check if read would exceed buffer size
    if (offset + size > buffers[buffer].size) {
        return -1;
    }
    
    // Copy data from buffer
    memcpy(data, (uint8_t*)buffers[buffer].data + offset, size);
    return size;
}

// Set buffer callback
void sound_buffer_set_callback(uint32_t buffer, sound_callback_t callback) {
    if (buffer >= MAX_SOUND_BUFFERS) return;
    if (buffers[buffer].state == BUFFER_STATE_FREE) return;
    
    buffers[buffer].callback = callback;
}

// Start playing a buffer
int sound_play(uint32_t buffer) {
    if (buffer >= MAX_SOUND_BUFFERS) return -1;
    if (buffers[buffer].state == BUFFER_STATE_FREE) return -1;
    
    buffers[buffer].state = BUFFER_STATE_PLAYING;
    return 0;
}

// Pause buffer playback
int sound_pause(uint32_t buffer) {
    if (buffer >= MAX_SOUND_BUFFERS) return -1;
    if (buffers[buffer].state != BUFFER_STATE_PLAYING) return -1;
    
    buffers[buffer].state = BUFFER_STATE_PAUSED;
    return 0;
}

// Stop buffer playback
int sound_stop(uint32_t buffer) {
    if (buffer >= MAX_SOUND_BUFFERS) return -1;
    if (buffers[buffer].state == BUFFER_STATE_FREE) return -1;
    
    buffers[buffer].state = BUFFER_STATE_STOPPED;
    buffers[buffer].position = 0;
    return 0;
}

// Set buffer volume
int sound_set_volume(uint32_t buffer, uint8_t volume) {
    if (buffer >= MAX_SOUND_BUFFERS) return -1;
    if (buffers[buffer].state == BUFFER_STATE_FREE) return -1;
    
    buffer_volumes[buffer] = volume;
    return 0;
}

// Get buffer volume
uint8_t sound_get_volume(uint32_t buffer) {
    if (buffer >= MAX_SOUND_BUFFERS || buffers[buffer].state == BUFFER_STATE_FREE) {
        return 0;
    }
    return buffer_volumes[buffer];
}

// Mix active sound buffers
void sound_mix_buffers(void* output, uint32_t frames) {
    int16_t* out = (int16_t*)output;
    memset(out, 0, frames * sizeof(int16_t) * 2); // Stereo output
    
    for (int i = 0; i < MAX_SOUND_BUFFERS; i++) {
        if (buffers[i].state != BUFFER_STATE_PLAYING) continue;
        
        uint32_t mix_frames = frames;
        uint32_t available_frames = (buffers[i].size - buffers[i].position) / 
            sound_get_frame_size(buffers[i].format, buffers[i].channels);
            
        if (mix_frames > available_frames) {
            mix_frames = available_frames;
        }
        
        if (mix_frames == 0) {
            // Buffer finished playing
            if (buffers[i].callback) {
                buffers[i].callback(buffers[i].data, buffers[i].size);
            }
            sound_stop(i);
            continue;
        }
        
        // Mix based on format
        if (buffers[i].format == SOUND_FORMAT_PCM8) {
            uint8_t* src = (uint8_t*)buffers[i].data + buffers[i].position;
            for (uint32_t j = 0; j < mix_frames; j++) {
                int16_t sample = ((int16_t)(*src++) - 128) << 8;
                sample = (sample * buffer_volumes[i]) >> 8;
                
                out[j * 2] += sample;     // Left
                out[j * 2 + 1] += sample; // Right
            }
        }
        else if (buffers[i].format == SOUND_FORMAT_PCM16) {
            int16_t* src = (int16_t*)((uint8_t*)buffers[i].data + buffers[i].position);
            for (uint32_t j = 0; j < mix_frames; j++) {
                int16_t sample = *src++;
                sample = (sample * buffer_volumes[i]) >> 8;
                
                out[j * 2] += sample;     // Left
                out[j * 2 + 1] += sample; // Right
            }
        }
        
        // Update buffer position
        buffers[i].position += mix_frames * sound_get_frame_size(buffers[i].format, buffers[i].channels);
    }
}

// Get frame size in bytes
uint32_t sound_get_frame_size(uint8_t format, uint8_t channels) {
    uint32_t bytes_per_sample;
    
    switch (format) {
        case SOUND_FORMAT_PCM8:
            bytes_per_sample = 1;
            break;
        case SOUND_FORMAT_PCM16:
            bytes_per_sample = 2;
            break;
        default:
            return 0;
    }
    
    return bytes_per_sample * channels;
}