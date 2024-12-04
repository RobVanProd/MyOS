# MyOS - A Simple x86 Operating System

A bare metal x86 operating system built from scratch for learning purposes. This project implements fundamental OS concepts including interrupt handling, memory management, keyboard input, file systems, and process management.

## Features

### Currently Implemented
- VGA text mode driver with color support and scrolling
- Global Descriptor Table (GDT) for memory segmentation
- Interrupt Descriptor Table (IDT) for handling CPU exceptions

### In Progress
- Programmable Interrupt Controller (PIC) initialization
- Keyboard driver
- Memory paging
- Basic file system
- Process management

## Building

### Prerequisites
- NASM (Netwide Assembler)
- i686-elf cross-compiler
- QEMU for emulation

### Build Instructions
```bash
# Build the kernel
make

# Run in QEMU
make run
```

## Project Structure
```
src/
├── boot/              # Boot-related code
│   └── multiboot.asm  # Multiboot header and entry point
├── kernel/            # Kernel source files
│   ├── kernel.c       # Main kernel file
│   ├── terminal.c     # VGA text mode driver
│   ├── gdt.c         # Global Descriptor Table
│   └── idt.c         # Interrupt Descriptor Table
└── include/          # Header files
```

## Memory Layout
- Kernel is loaded at 1MB (0x100000)
- GDT setup with:
  - Null descriptor
  - Kernel code segment (0x08)
  - Kernel data segment (0x10)
  - User code segment (0x18)
  - User data segment (0x20)

## Contributing
This is a learning project. Feel free to submit issues or pull requests if you find bugs or have suggestions for improvements.

## License
This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments
- OSDev.org Wiki and Community
- "Operating Systems: Three Easy Pieces" by Remzi H. Arpaci-Dusseau
- James Molloy's Kernel Development Tutorials 