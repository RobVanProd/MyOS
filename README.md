# MyOS - A Modern Operating System

A feature-rich operating system built from scratch, focusing on modern functionality, user experience, and system performance.

## Features

### Core System
- **Memory Management**
  - Paging support
  - Virtual memory management
  - Heap allocation
  - Memory mapping capabilities
  - Protected memory spaces

- **Process Management**
  - Multi-tasking support
  - Process creation and destruction
  - Priority-based scheduling
  - Context switching
  - Inter-process communication
  - Process states (Ready, Running, Blocked, Zombie)

- **File System**
  - Inode-based architecture
  - Directory structure support
  - File operations (create, read, write, delete)
  - File permissions
  - Symbolic links
  - Maximum file size: 4GB
  - Support for up to 1024 files

### User Interface
- **Modern GUI System**
  - Window management
  - Double buffering for smooth graphics
  - Mouse and keyboard input
  - Multiple window support
  - Window stacking and focus management

- **Applications**
  1. **Notepad**
     - Text editing capabilities
     - File saving/loading
     - Cut, copy, paste functionality
     - Keyboard shortcuts
  
  2. **Calculator**
     - Basic arithmetic operations
     - Modern, responsive interface
     - Keyboard input support
     - Error handling
     - Decimal support
     - Backspace functionality

### Networking
- **Complete Network Stack**
  - TCP/IP protocol support
  - Socket interface
  - Support for multiple network interfaces
  - Protocols:
    - IP (v4)
    - TCP
    - UDP
    - ICMP
    - ARP
  - Maximum packet size: 1518 bytes
  - Network interface flags for various modes

### Audio System
- **Sound Support**
  - Multiple sound buffer management
  - Various audio formats:
    - PCM8
    - PCM16
  - Channel configurations:
    - Mono
    - Stereo
  - Sample rates:
    - 8000 Hz
    - 11025 Hz
    - 22050 Hz
    - 44100 Hz
  - Volume control
  - Sound mixing capabilities
  - Callback system for audio events

## Building and Running

### Prerequisites
- i686-elf cross-compiler
- NASM assembler
- GNU Make
- QEMU for testing

### Build Instructions
1. Clone the repository:
   ```bash
   git clone https://github.com/yourusername/myos.git
   cd myos
   ```

2. Build the OS:
   ```bash
   make
   ```

3. Run in QEMU:
   ```bash
   make run
   ```

## System Requirements
- x86 architecture
- Minimum 32MB RAM
- VGA-compatible display
- PS/2 keyboard and mouse
- (Optional) Sound card for audio support
- (Optional) Network card for networking features

## Project Structure
```
src/
├── boot/          # Boot loader and initialization
├── kernel/        # Core kernel components
│   ├── memory/    # Memory management
│   ├── process/   # Process management
│   ├── fs/        # File system
│   ├── network/   # Network stack
│   ├── sound/     # Audio system
│   └── graphics/  # GUI system
└── apps/          # User applications
    ├── notepad/   # Text editor
    └── calculator/# Calculator application
```

## Recent Updates
- Added complete network stack with TCP/IP support
- Implemented sound system with multiple format support
- Added calculator application with modern UI
- Enhanced file system with inode-based architecture
- Added process management with multi-tasking support
- Improved graphics system with double buffering
- Added mouse support with event system
- Updated kernel to support all new features

## Contributing
Contributions are welcome! Please read our contributing guidelines before submitting pull requests.

## License
This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments
- Thanks to all contributors who have helped build this OS
- Special thanks to the OSDev community for their valuable resources

## Contact
For questions, suggestions, or bug reports, please open an issue on GitHub.