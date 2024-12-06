 #include <fs.h>
#include <memory.h>
#include <string.h>

#define MAX_FILES 256
#define MAX_FILE_SIZE 1024*1024  // 1MB max file size

typedef struct {
    char name[256];
    uint8_t* data;
    size_t size;
    int used;
} file_t;

static file_t files[MAX_FILES];
static int initialized = 0;

void fs_init(void) {
    if (!initialized) {
        memset(files, 0, sizeof(files));
        initialized = 1;
    }
}

int fs_open(const char* path, int flags) {
    // Find free file slot
    int fd;
    for (fd = 0; fd < MAX_FILES; fd++) {
        if (!files[fd].used) {
            break;
        }
    }
    
    if (fd >= MAX_FILES) {
        return -1;  // No free slots
    }
    
    // Initialize file
    strncpy(files[fd].name, path, 255);
    files[fd].name[255] = '\0';
    files[fd].size = 0;
    files[fd].used = 1;
    
    if (flags & FS_OPEN_CREATE) {
        files[fd].data = kmalloc(MAX_FILE_SIZE);
        if (!files[fd].data) {
            files[fd].used = 0;
            return -1;
        }
    }
    
    return fd;
}

int fs_write(int fd, const void* buffer, size_t size) {
    if (fd < 0 || fd >= MAX_FILES || !files[fd].used) {
        return -1;
    }
    
    if (files[fd].size + size > MAX_FILE_SIZE) {
        return -1;
    }
    
    memcpy(files[fd].data + files[fd].size, buffer, size);
    files[fd].size += size;
    return size;
}

int fs_read(int fd, void* buffer, size_t size) {
    if (fd < 0 || fd >= MAX_FILES || !files[fd].used) {
        return -1;
    }
    
    size_t to_read = size;
    if (to_read > files[fd].size) {
        to_read = files[fd].size;
    }
    
    memcpy(buffer, files[fd].data, to_read);
    return to_read;
}

int fs_close(int fd) {
    if (fd >= 0 && fd < MAX_FILES && files[fd].used) {
        kfree(files[fd].data);
        files[fd].used = 0;
        return 0;
    }
    return -1;
}