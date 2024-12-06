#ifndef FS_H
#define FS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Maximum number of open files
#define MAX_FILES 256
#define MAX_FILENAME 256
#define MAX_FILE_SIZE 1048576  // 1MB

// File flags
#define FS_OPEN_READ    0x01
#define FS_OPEN_WRITE   0x02
#define FS_OPEN_CREATE  0x04
#define FS_OPEN_APPEND  0x08

// File types
#define FS_FILE        0x01
#define FS_DIRECTORY   0x02
#define FS_CHARDEVICE  0x03
#define FS_BLOCKDEVICE 0x04
#define FS_PIPE        0x05
#define FS_SYMLINK     0x06
#define FS_MOUNTPOINT  0x08

// File system structures
typedef struct fs_node {
    char name[128];           // Filename
    uint32_t mask;           // Permission mask
    uint32_t uid;            // User id
    uint32_t gid;            // Group id
    uint32_t flags;          // Includes the node type
    uint32_t inode;          // Device-specific - provides a way for a filesystem to identify files
    uint32_t length;         // Size of the file, in bytes
    uint32_t impl;           // Implementation-dependent number
    
    // File operations
    uint32_t (*read)(struct fs_node*, uint32_t, uint32_t, uint8_t*);
    uint32_t (*write)(struct fs_node*, uint32_t, uint32_t, uint8_t*);
    void (*open)(struct fs_node*);
    void (*close)(struct fs_node*);
    struct dirent* (*readdir)(struct fs_node*, uint32_t);
    struct fs_node* (*finddir)(struct fs_node*, char* name);
} fs_node_t;

// Directory entry structure
struct dirent {
    char name[128];          // Filename
    uint32_t ino;           // Inode number
};

// File structure
typedef struct {
    char name[MAX_FILENAME];
    uint8_t type;
    uint32_t size;
    uint32_t position;
    void* data;
    bool used;
} file_t;

// File system functions
void fs_init(void);
fs_node_t* fs_root;         // The root of the filesystem

// Standard file operations
uint32_t read_fs(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer);
uint32_t write_fs(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer);
void open_fs(fs_node_t* node);
void close_fs(fs_node_t* node);
struct dirent* readdir_fs(fs_node_t* node, uint32_t index);
fs_node_t* finddir_fs(fs_node_t* node, char* name);

// New file system functions
int fs_open(const char* filename, uint8_t flags);
int fs_close(int fd);
int fs_read(int fd, void* buffer, uint32_t size);
int fs_write(int fd, const void* buffer, uint32_t size);
int fs_seek(int fd, uint32_t offset);
int fs_tell(int fd);
int fs_eof(int fd);
int fs_create(const char* filename);
int fs_delete(const char* filename);
int fs_stat(const char* filename, uint32_t* size);
int fs_exists(const char* filename);

#endif // FS_H
