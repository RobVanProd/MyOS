# MyOS - A Modern Operating System

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)]()
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)]()

A feature-rich operating system built from scratch, focusing on modern functionality, user experience, and system performance.

## 🚀 Quick Start

```bash
# Clone the repository
git clone https://github.com/yourusername/myos.git
cd myos

# Build the OS
make

# Run in QEMU
make run
```

## ✨ Features

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

## 📚 Documentation

### Core System Documentation
- [System Architecture](docs/core/system.md) - Overall system design and component interactions
- [Memory Management](docs/core/memory.md) - Memory allocation, virtual memory, and protection mechanisms
- [Process Management](docs/core/process.md) - Process scheduling, IPC, and synchronization primitives

### API Reference
- [Command System](docs/api/command.md) - Command registration and execution API
- [System Calls](docs/api/syscalls.md) - Available system calls and their usage

### User Guides
- [Shell Interface](docs/user/shell.md) - Using the command-line interface and built-in commands

## 🛠️ Building and Running

### Prerequisites
- [i686-elf cross-compiler](https://wiki.osdev.org/GCC_Cross-Compiler)
- [NASM](https://www.nasm.us/) assembler
- [GNU Make](https://www.gnu.org/software/make/)
- [QEMU](https://www.qemu.org/) for testing

### Build Instructions
1. Ensure all prerequisites are installed
2. Clone and build:
   ```bash
   git clone https://github.com/yourusername/myos.git
   cd myos
   make
   ```
3. Run in QEMU:
   ```bash
   make run
   ```

## 💻 System Requirements

| Component | Minimum Requirement |
|-----------|-------------------|
| Architecture | x86 |
| RAM | 32MB |
| Display | VGA-compatible |
| Input | PS/2 keyboard and mouse |
| Optional | Sound card, Network card |

## 📁 Project Structure
```
myos/
├── src/              # Source code
│   ├── boot/         # Boot loader and initialization
│   ├── kernel/       # Core kernel components
│   │   ├── memory/   # Memory management
│   │   ├── process/  # Process management
│   │   ├── fs/       # File system
│   │   ├── network/  # Network stack
│   │   ├── sound/    # Audio system
│   │   └── graphics/ # GUI system
│   └── apps/         # User applications
│       ├── notepad/  # Text editor
│       └── calc/     # Calculator
├── docs/             # Documentation
│   ├── core/         # Core system docs
│   ├── api/          # API reference
│   └── user/         # User guides
├── tests/            # Test suites
└── tools/            # Development tools
```

## 🎉 Recent Updates
- Added comprehensive command system with extensible architecture
- Implemented shell application with command history and line editing
- Created detailed system documentation and API references
- Enhanced memory management with improved error handling
- Added process management with priority-based scheduling

## 🤝 Contributing

We welcome contributions! Here's how you can help:

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

Please read our [Contributing Guidelines](CONTRIBUTING.md) for details.

## 📝 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- Thanks to all contributors who have helped build MyOS
- Special thanks to the OSDev community for their valuable resources
- Inspired by various open-source operating systems

## 📫 Contact & Support

- Report bugs by [opening an issue](https://github.com/yourusername/myos/issues)
- Join our [Discord community](https://discord.gg/yourdiscord)
- Follow updates on [Twitter](https://twitter.com/yourusername)

---
<div align="center">
Made with ❤️ by the MyOS Team
</div>