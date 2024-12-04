# File System API

## Overview

The File System API provides interfaces for file and directory operations in MyOS. It implements a hierarchical file system with support for regular files, directories, symbolic links, and device files.

## File System Constants

### File System Limits
```c
#define FS_MAX_FILENAME_LENGTH 256
#define FS_MAX_PATH_LENGTH 4096
#define FS_MAX_FILES 1024
#define FS_MAX_OPEN_FILES 32
#define FS_BLOCK_SIZE 4096
```

### File Types
```c
#define FS_TYPE_FILE 1
#define FS_TYPE_DIRECTORY 2
#define FS_TYPE_SYMLINK 3
#define FS_TYPE_DEVICE 4
```

### File Permissions
```c
#define FS_PERM_READ    0x04
#define FS_PERM_WRITE   0x02
#define FS_PERM_EXECUTE 0x01
```

### File Open Modes
```c
#define FS_OPEN_READ    0x01
#define FS_OPEN_WRITE   0x02
#define FS_OPEN_CREATE  0x04
#define FS_OPEN_APPEND  0x08
#define FS_OPEN_TRUNC   0x10
```

### Seek Modes
```c
#define FS_SEEK_SET 0
#define FS_SEEK_CUR 1
#define FS_SEEK_END 2
```

## File System Structures

### Inode Structure
```c
typedef struct {
    uint32_t inode;
    uint32_t size;
    uint8_t type;
    uint8_t permissions;
    uint32_t links;
    uint32_t blocks;
    uint32_t direct_blocks[12];
    uint32_t indirect_block;
    uint32_t double_indirect_block;
    uint32_t access_time;
    uint32_t modify_time;
    uint32_t create_time;
} inode_t;
```

### Directory Entry
```c
typedef struct {
    char name[FS_MAX_FILENAME_LENGTH];
    uint32_t inode;
} dir_entry_t;
```

### Superblock
```c
typedef struct {
    uint32_t magic;
    uint32_t blocks;
    uint32_t inodes;
    uint32_t free_blocks;
    uint32_t free_inodes;
    uint32_t block_size;
    uint32_t inode_blocks;
    uint32_t data_blocks;
} superblock_t;
```

### File Handle
```c
typedef struct {
    inode_t* inode;
    uint32_t offset;
    uint8_t mode;
    uint8_t flags;
} file_handle_t;
```

## File System Operations

### Initialization

```c
void fs_init(void);
int fs_format(void);
int fs_mount(const char* device);
int fs_unmount(void);
```

#### fs_init()
Initializes the file system.

#### fs_format()
Formats the file system.

Returns: 0 on success, -1 on error

#### fs_mount()
Mounts a file system from a device.

Parameters:
- `device`: Device path

Returns: 0 on success, -1 on error

#### fs_unmount()
Unmounts the current file system.

Returns: 0 on success, -1 on error

### File Operations

```c
int fs_open(const char* path, uint8_t mode);
int fs_close(int fd);
int fs_read(int fd, void* buffer, uint32_t size);
int fs_write(int fd, const void* buffer, uint32_t size);
int fs_seek(int fd, int32_t offset, uint8_t whence);
int fs_tell(int fd);
int fs_truncate(int fd, uint32_t size);
```

#### fs_open()
Opens a file.

Parameters:
- `path`: File path
- `mode`: Open mode flags

Returns: File descriptor or -1 on error

#### fs_close()
Closes a file.

Parameters:
- `fd`: File descriptor

Returns: 0 on success, -1 on error

#### fs_read()
Reads from a file.

Parameters:
- `fd`: File descriptor
- `buffer`: Buffer to read into
- `size`: Number of bytes to read

Returns: Bytes read or -1 on error

#### fs_write()
Writes to a file.

Parameters:
- `fd`: File descriptor
- `buffer`: Buffer to write from
- `size`: Number of bytes to write

Returns: Bytes written or -1 on error

#### fs_seek()
Changes file position.

Parameters:
- `fd`: File descriptor
- `offset`: Offset from whence
- `whence`: Seek mode

Returns: New position or -1 on error

#### fs_tell()
Gets current file position.

Parameters:
- `fd`: File descriptor

Returns: Current position or -1 on error

#### fs_truncate()
Changes file size.

Parameters:
- `fd`: File descriptor
- `size`: New size

Returns: 0 on success, -1 on error

### Directory Operations

```c
int fs_mkdir(const char* path);
int fs_rmdir(const char* path);
int fs_opendir(const char* path);
int fs_readdir(int dd, dir_entry_t* entry);
int fs_closedir(int dd);
```

#### fs_mkdir()
Creates a directory.

Parameters:
- `path`: Directory path

Returns: 0 on success, -1 on error

#### fs_rmdir()
Removes a directory.

Parameters:
- `path`: Directory path

Returns: 0 on success, -1 on error

#### fs_opendir()
Opens a directory for reading.

Parameters:
- `path`: Directory path

Returns: Directory descriptor or -1 on error

#### fs_readdir()
Reads next directory entry.

Parameters:
- `dd`: Directory descriptor
- `entry`: Entry buffer

Returns: 1 if entry read, 0 if end, -1 on error

#### fs_closedir()
Closes a directory.

Parameters:
- `dd`: Directory descriptor

Returns: 0 on success, -1 on error

### File System Utilities

```c
int fs_stat(const char* path, inode_t* stat);
int fs_link(const char* oldpath, const char* newpath);
int fs_unlink(const char* path);
int fs_symlink(const char* target, const char* linkpath);
int fs_readlink(const char* path, char* buffer, uint32_t size);
int fs_chmod(const char* path, uint8_t mode);
int fs_rename(const char* oldpath, const char* newpath);
```

#### fs_stat()
Gets file status.

Parameters:
- `path`: File path
- `stat`: Status buffer

Returns: 0 on success, -1 on error

#### fs_link()
Creates a hard link.

Parameters:
- `oldpath`: Existing file
- `newpath`: New link

Returns: 0 on success, -1 on error

#### fs_unlink()
Removes a file link.

Parameters:
- `path`: File path

Returns: 0 on success, -1 on error

#### fs_symlink()
Creates a symbolic link.

Parameters:
- `target`: Link target
- `linkpath`: Link path

Returns: 0 on success, -1 on error

#### fs_readlink()
Reads a symbolic link.

Parameters:
- `path`: Link path
- `buffer`: Buffer for target
- `size`: Buffer size

Returns: Bytes read or -1 on error

#### fs_chmod()
Changes file permissions.

Parameters:
- `path`: File path
- `mode`: New permissions

Returns: 0 on success, -1 on error

#### fs_rename()
Renames a file.

Parameters:
- `oldpath`: Old path
- `newpath`: New path

Returns: 0 on success, -1 on error

## Examples

### Basic File Operations
```c
// Create and write to file
int fd = fs_open("/test.txt", FS_OPEN_CREATE | FS_OPEN_WRITE);
if (fd >= 0) {
    const char* data = "Hello, File System!";
    fs_write(fd, data, strlen(data));
    fs_close(fd);
}

// Read from file
fd = fs_open("/test.txt", FS_OPEN_READ);
if (fd >= 0) {
    char buffer[100];
    int bytes = fs_read(fd, buffer, sizeof(buffer));
    if (bytes > 0) {
        buffer[bytes] = '\0';
        printf("Read: %s\n", buffer);
    }
    fs_close(fd);
}
```

### Directory Operations
```c
// Create directory
fs_mkdir("/mydir");

// List directory contents
int dd = fs_opendir("/mydir");
if (dd >= 0) {
    dir_entry_t entry;
    while (fs_readdir(dd, &entry) > 0) {
        printf("Found: %s\n", entry.name);
    }
    fs_closedir(dd);
}
```

### Symbolic Links
```c
// Create symbolic link
fs_symlink("/original.txt", "/link.txt");

// Read through link
char target[FS_MAX_PATH_LENGTH];
if (fs_readlink("/link.txt", target, sizeof(target)) > 0) {
    printf("Link target: %s\n", target);
}
```

## Best Practices

1. Always check return values from file operations
2. Close files and directories when done
3. Use appropriate permissions for security
4. Handle error conditions gracefully
5. Use proper path sanitization
6. Implement proper file locking
7. Regular fsck maintenance

## See Also

- [Process Management API](process.md)
- [Memory Management API](memory.md)
- [File System Documentation](../../core/filesystem/filesystem.md)
- [File Operations Documentation](../../core/filesystem/operations.md) 