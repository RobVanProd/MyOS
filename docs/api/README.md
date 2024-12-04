# MyOS API Reference

This section provides comprehensive documentation for all MyOS APIs. Each subsystem's API is documented with function signatures, parameters, return values, and usage examples.

## Core APIs

### Memory Management API
- [Memory Allocation](memory.md#allocation)
- [Virtual Memory](memory.md#virtual)
- [Memory Mapping](memory.md#mapping)
- [Memory Protection](memory.md#protection)

### Process Management API
- [Process Creation](process.md#creation)
- [Process Control](process.md#control)
- [Scheduling](process.md#scheduling)
- [IPC Mechanisms](process.md#ipc)

### File System API
- [File Operations](filesystem.md#operations)
- [Directory Management](filesystem.md#directories)
- [File Permissions](filesystem.md#permissions)
- [File System Control](filesystem.md#control)

## System Services

### Network API
- [Socket Interface](network.md#sockets)
- [TCP Protocol](network.md#tcp)
- [UDP Protocol](network.md#udp)
- [Network Configuration](network.md#config)

### Graphics API
- [Window Management](graphics.md#windows)
- [Drawing Functions](graphics.md#drawing)
- [Event Handling](graphics.md#events)
- [GUI Components](graphics.md#components)

### Sound API
- [Audio Playback](sound.md#playback)
- [Sound Buffers](sound.md#buffers)
- [Audio Formats](sound.md#formats)
- [Mixer Controls](sound.md#mixer)

## API Usage Examples

### Memory Management
```c
// Allocate memory
void* buffer = kmalloc(1024);
if (!buffer) {
    handle_error();
}

// Map memory
int result = mmap(buffer, 1024, PROT_READ | PROT_WRITE);
if (result < 0) {
    handle_error();
}

// Free memory
kfree(buffer);
```

### Process Management
```c
// Create process
process_t* proc = process_create("myapp", entry_point, PRIORITY_NORMAL);
if (!proc) {
    handle_error();
}

// Set process priority
process_set_priority(proc, PRIORITY_HIGH);

// Start process
process_start(proc);
```

### File System
```c
// Open file
int fd = fs_open("/path/to/file", FS_OPEN_READ);
if (fd < 0) {
    handle_error();
}

// Read data
char buffer[1024];
int bytes_read = fs_read(fd, buffer, sizeof(buffer));

// Close file
fs_close(fd);
```

### Network
```c
// Create socket
int sock = socket_create(SOCK_STREAM);
if (sock < 0) {
    handle_error();
}

// Connect
int result = socket_connect(sock, "192.168.1.1", 80);
if (result < 0) {
    handle_error();
}

// Send data
socket_send(sock, data, length);
```

### Graphics
```c
// Create window
window_t* win = create_window(100, 100, 800, 600, "My Window");
if (!win) {
    handle_error();
}

// Draw on window
draw_rectangle(win, 10, 10, 100, 50, COLOR_BLUE);
draw_text(win, 20, 20, "Hello, World!");

// Handle events
win->on_click = handle_click;
win->on_key = handle_key;
```

### Sound
```c
// Create sound buffer
int buffer = sound_buffer_create(SOUND_FORMAT_PCM16, SOUND_CHANNEL_STEREO);
if (buffer < 0) {
    handle_error();
}

// Load and play sound
sound_buffer_write(buffer, sound_data, data_size);
sound_play(buffer);
```

## API Conventions

### Return Values
- Negative values indicate errors
- Zero typically indicates success
- Positive values indicate success with additional information

### Error Handling
```c
if (result < 0) {
    switch (result) {
        case ERR_NOMEM:
            // Handle out of memory
            break;
        case ERR_INVAL:
            // Handle invalid parameter
            break;
        default:
            // Handle unknown error
            break;
    }
}
```

### Memory Management
- Caller is responsible for freeing allocated memory
- Use appropriate deallocation functions
- Check return values for allocation failures

### Thread Safety
- Functions marked with `thread_safe` are safe to call from any context
- Functions marked with `non_thread_safe` must be called from appropriate context
- Use synchronization primitives when necessary

## API Versioning

### Version Format
- Major.Minor.Patch (e.g., 1.2.3)
- Major: Breaking changes
- Minor: New features, backward compatible
- Patch: Bug fixes

### Compatibility
- APIs maintain backward compatibility within major versions
- Deprecated functions marked with `deprecated`
- Migration guides provided for major version changes

## Contributing to APIs

See our [API Development Guide](../development.md#api-development) for:
- API design principles
- Documentation standards
- Testing requirements
- Review process

## Support

For API support:
1. Check the relevant documentation section
2. Search [existing issues](https://github.com/yourusername/myos/issues)
3. Open a [new issue](https://github.com/yourusername/myos/issues/new) 