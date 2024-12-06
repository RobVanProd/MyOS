 #ifndef FS_H
#define FS_H

#include <stdint.h>

// File system flags
#define FS_OPEN_READ    0x01
#define FS_OPEN_WRITE   0x02
#define FS_OPEN_CREATE  0x04
#define FS_OPEN_APPEND  0x08

// File system error codes
#define FS_ERROR_NOT_FOUND      -1
#define FS_ERROR_EXISTS         -2
#define FS_ERROR_INVALID_PATH   -3
#define FS_ERROR_NO_SPACE       -4
#define FS_ERROR_READ_ONLY      -5
#define FS_ERROR_IO            -6

// File system functions
void fs_init(void);
int fs_open(const char* path, int flags);
int fs_close(int fd);
int fs_read(int fd, void* buffer, size_t size);
int fs_write(int fd, const void* buffer, size_t size);
int fs_seek(int fd, int offset, int whence);
int fs_tell(int fd);
int fs_remove(const char* path);
int fs_rename(const char* oldpath, const char* newpath);

// Directory operations
typedef struct {
    char name[256];
    uint32_t size;
    uint32_t flags;
    uint32_t created;
    uint32_t modified;
} fs_dirent_t;

int fs_opendir(const char* path);
int fs_readdir(int dd, fs_dirent_t* entry);
int fs_closedir(int dd);
int fs_mkdir(const char* path);
int fs_rmdir(const char* path);

#endif