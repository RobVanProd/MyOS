#include "sound.h"
#include "memory.h"
#include "string.h"

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
        buffers[i].user_data = NULL;
        buffer_volumes[i] = 255;
    }
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
int sound_buffer_create(uint8_t format, uint8_t channels, uint32_t sample_rate, uint32_t size) {
    // Find free buffer
    int buffer_id = -1;
    for (int i = 0; i < MAX_SOUND_BUFFERS; i++) {
        if (buffers[i].state == BUFFER_STATE_FREE) {
            buffer_id = i;
            break;
        }
    }
    if (buffer_id == -1) return -1;
    
    // Allocate buffer data
    buffers[buffer_id].data = kmalloc(size);
    if (!buffers[buffer_id].data) return -1;
    
    // Initialize buffer
    buffers[buffer_id].size = size;
    buffers[buffer_id].position = 0;
    buffers[buffer_id].format = format;
    buffers[buffer_id].channels = channels;
    buffers[buffer_id].sample_rate = sample_rate;
    buffers[buffer_id].state = BUFFER_STATE_STOPPED;
    
    return buffer_id;
}

// Destroy a sound buffer
void sound_buffer_destroy(int buffer) {
    if (buffer < 0 || buffer >= MAX_SOUND_BUFFERS) return;
    if (buffers[buffer].state == BUFFER_STATE_FREE) return;
    
    // Stop playback if needed
    if (buffers[buffer].state == BUFFER_STATE_PLAYING) {
        sound_stop(buffer);
    }
    
    // Free buffer data
    if (buffers[buffer].data) {
        kfree(buffers[buffer].data);
    }
    
    // Reset buffer state
    buffers[buffer].state = BUFFER_STATE_FREE;
    buffers[buffer].data = NULL;
    buffers[buffer].callback = NULL;
    buffers[buffer].user_data = NULL;
}

// Write data to buffer
int sound_buffer_write(int buffer, const void* data, uint32_t size) {
    if (buffer < 0 || buffer >= MAX_SOUND_BUFFERS) return -1;
    if (buffers[buffer].state == BUFFER_STATE_FREE) return -1;
    if (!data) return -1;
    
    // Check if size exceeds buffer size
    if (size > buffers[buffer].size) {
        size = buffers[buffer].size;
    }
    
    // Copy data
    memcpy(buffers[buffer].data, data, size);
    return size;
}

// Read data from buffer
int sound_buffer_read(int buffer, void* data, uint32_t size) {
    if (buffer < 0 || buffer >= MAX_SOUND_BUFFERS) return -1;
    if (buffers[buffer].state == BUFFER_STATE_FREE) return -1;
    if (!data) return -1;
    
    // Check if size exceeds remaining data
    uint32_t remaining = buffers[buffer].size - buffers[buffer].position;
    if (size > remaining) {
        size = remaining;
    }
    
    // Copy data
    memcpy(data, buffers[buffer].data + buffers[buffer].position, size);
    buffers[buffer].position += size;
    
    return size;
}

// Set buffer callback
void sound_buffer_set_callback(int buffer, void (*callback)(void*), void* user_data) {
    if (buffer < 0 || buffer >= MAX_SOUND_BUFFERS) return;
    if (buffers[buffer].state == BUFFER_STATE_FREE) return;
    
    buffers[buffer].callback = callback;
    buffers[buffer].user_data = user_data;
}

// Start playing a buffer
int sound_play(int buffer) {
    if (buffer < 0 || buffer >= MAX_SOUND_BUFFERS) return -1;
    if (buffers[buffer].state == BUFFER_STATE_FREE) return -1;
    
    buffers[buffer].state = BUFFER_STATE_PLAYING;
    return 0;
}

// Pause buffer playback
int sound_pause(int buffer) {
    if (buffer < 0 || buffer >= MAX_SOUND_BUFFERS) return -1;
    if (buffers[buffer].state != BUFFER_STATE_PLAYING) return -1;
    
    buffers[buffer].state = BUFFER_STATE_PAUSED;
    return 0;
}

// Stop buffer playback
int sound_stop(int buffer) {
    if (buffer < 0 || buffer >= MAX_SOUND_BUFFERS) return -1;
    if (buffers[buffer].state == BUFFER_STATE_FREE) return -1;
    
    buffers[buffer].state = BUFFER_STATE_STOPPED;
    buffers[buffer].position = 0;
    return 0;
}

// Set buffer volume
int sound_set_volume(int buffer, uint8_t volume) {
    if (buffer < 0 || buffer >= MAX_SOUND_BUFFERS) return -1;
    if (buffers[buffer].state == BUFFER_STATE_FREE) return -1;
    
    buffer_volumes[buffer] = volume;
    return 0;
}

// Get buffer volume
uint8_t sound_get_volume(int buffer) {
    if (buffer < 0 || buffer >= MAX_SOUND_BUFFERS) return 0;
    if (buffers[buffer].state == BUFFER_STATE_FREE) return 0;
    
    return buffer_volumes[buffer];
}

// Mix active sound buffers
void sound_mix_buffers(int16_t* output, uint32_t frames) {
    if (!output) return;
    
    // Clear output buffer
    memset(output, 0, frames * sizeof(int16_t) * 2); // Stereo output
    
    // Mix each active buffer
    for (int i = 0; i < MAX_SOUND_BUFFERS; i++) {
        if (buffers[i].state != BUFFER_STATE_PLAYING) continue;
        
        uint32_t remaining = buffers[i].size - buffers[i].position;
        uint32_t mix_frames = frames;
        if (mix_frames > remaining) {
            mix_frames = remaining;
        }
        
        // Mix based on format
        if (buffers[i].format == SOUND_FORMAT_PCM8) {
            uint8_t* src = (uint8_t*)buffers[i].data + buffers[i].position;
            float volume = buffer_volumes[i] / 255.0f;
            
            for (uint32_t j = 0; j < mix_frames; j++) {
                int16_t sample = ((int16_t)src[j] - 128) << 8;
                sample = (int16_t)(sample * volume);
                
                output[j * 2] += sample;     // Left
                output[j * 2 + 1] += sample; // Right
            }
        }
        else if (buffers[i].format == SOUND_FORMAT_PCM16) {
            int16_t* src = (int16_t*)(buffers[i].data + buffers[i].position);
            float volume = buffer_volumes[i] / 255.0f;
            
            for (uint32_t j = 0; j < mix_frames; j++) {
                int16_t sample = (int16_t)(src[j] * volume);
                
                output[j * 2] += sample;     // Left
                output[j * 2 + 1] += sample; // Right
            }
        }
        
        // Update position
        buffers[i].position += mix_frames * sound_get_frame_size(buffers[i].format, buffers[i].channels);
        
        // Check for end of buffer
        if (buffers[i].position >= buffers[i].size) {
            buffers[i].position = 0;
            
            // Call callback if set
            if (buffers[i].callback) {
                buffers[i].callback(buffers[i].user_data);
            }
        }
    }
}

// Update sound system
void sound_update(void) {
    // This should be called regularly to update sound playback
    // Typically called from a timer interrupt
    
    // TODO: Implement actual sound output
    // For now, we just maintain buffer states
}

// Utility functions
uint32_t sound_bytes_to_frames(uint8_t format, uint8_t channels, uint32_t bytes) {
    uint32_t frame_size = sound_get_frame_size(format, channels);
    if (frame_size == 0) return 0;
    return bytes / frame_size;
}

uint32_t sound_frames_to_bytes(uint8_t format, uint8_t channels, uint32_t frames) {
    return frames * sound_get_frame_size(format, channels);
}

uint32_t sound_get_frame_size(uint8_t format, uint8_t channels) {
    uint32_t sample_size;
    
    switch (format) {
        case SOUND_FORMAT_PCM8:
            sample_size = 1;
            break;
        case SOUND_FORMAT_PCM16:
            sample_size = 2;
            break;
        default:
            return 0;
    }
    
    return sample_size * channels;
}