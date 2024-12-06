 #include "fs.h"
#include "kheap.h"
#include "terminal.h"
#include "string.h"

#define MAX_FILES 256
#define MAX_FILENAME 256

static file_t files[MAX_FILES];

void fs_init(void) {
    for (int i = 0; i < MAX_FILES; i++) {
        files[i].used = false;
    }
}

static int find_unused_fd(void) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (!files[i].used) {
            return i;
        }
    }
    return -1;  // No free file descriptors
}

int fs_open(const char* filename, uint8_t flags) {
    int fd = find_unused_fd();
    if (fd < 0) {
        return -1;  // No free file descriptors
    }

    files[fd].used = true;
    files[fd].position = 0;
    files[fd].size = 0;
    strncpy(files[fd].name, filename, MAX_FILENAME - 1);
    files[fd].name[MAX_FILENAME - 1] = '\0';

    if (flags & FS_OPEN_CREATE) {
        files[fd].data = kmalloc(MAX_FILE_SIZE);
        if (!files[fd].data) {
            files[fd].used = false;
            return -1;
        }
    }

    return fd;
}

int fs_write(int fd, const void* buffer, uint32_t size) {
    if (fd < 0 || fd >= MAX_FILES || !files[fd].used || !files[fd].data) {
        return -1;
    }

    if (files[fd].position + size > MAX_FILE_SIZE) {
        size = MAX_FILE_SIZE - files[fd].position;
    }

    memcpy((uint8_t*)files[fd].data + files[fd].position, buffer, size);
    files[fd].position += size;

    if (files[fd].position > files[fd].size) {
        files[fd].size = files[fd].position;
    }

    return size;
}

int fs_read(int fd, void* buffer, uint32_t size) {
    if (fd < 0 || fd >= MAX_FILES || !files[fd].used || !files[fd].data) {
        return -1;
    }

    if (files[fd].position + size > files[fd].size) {
        size = files[fd].size - files[fd].position;
    }

    memcpy(buffer, (uint8_t*)files[fd].data + files[fd].position, size);
    files[fd].position += size;

    return size;
}

int fs_close(int fd) {
    if (fd < 0 || fd >= MAX_FILES || !files[fd].used) {
        return -1;
    }

    if (files[fd].data) {
        kfree(files[fd].data);
        files[fd].data = NULL;
    }

    files[fd].used = false;
    return 0;
}

int fs_seek(int fd, uint32_t offset) {
    if (fd < 0 || fd >= MAX_FILES || !files[fd].used) {
        return -1;
    }

    if (offset > files[fd].size) {
        offset = files[fd].size;
    }

    files[fd].position = offset;
    return 0;
}

int fs_tell(int fd) {
    if (fd < 0 || fd >= MAX_FILES || !files[fd].used) {
        return -1;
    }

    return files[fd].position;
}

int fs_eof(int fd) {
    if (fd < 0 || fd >= MAX_FILES || !files[fd].used) {
        return -1;
    }

    return files[fd].position >= files[fd].size;
}

int fs_create(const char* filename) {
    return fs_open(filename, FS_OPEN_CREATE | FS_OPEN_WRITE);
}

int fs_delete(const char* filename) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used && strcmp(files[i].name, filename) == 0) {
            return fs_close(i);
        }
    }
    return -1;
}

int fs_stat(const char* filename, uint32_t* size) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used && strcmp(files[i].name, filename) == 0) {
            if (size) {
                *size = files[i].size;
            }
            return 0;
        }
    }
    return -1;
}

int fs_exists(const char* filename) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used && strcmp(files[i].name, filename) == 0) {
            return 1;
        }
    }
    return 0;
}