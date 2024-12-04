# Compiler/Assembler/Linker flags
CC = i686-elf-gcc
AS = nasm
CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra
ASFLAGS = -f elf32
LDFLAGS = -ffreestanding -O2 -nostdlib

# Source files
BOOT_SRC = src/boot/multiboot.asm
ASM_SRCS = src/kernel/interrupt.asm src/kernel/gdt.asm
KERNEL_SRCS = src/kernel/kernel.c \
              src/kernel/terminal.c \
              src/kernel/gdt.c \
              src/kernel/idt.c \
              src/kernel/pic.c \
              src/kernel/keyboard.c

# Object files
BOOT_OBJ = $(BOOT_SRC:.asm=.o)
ASM_OBJS = $(ASM_SRCS:.asm=.o)
KERNEL_OBJS = $(KERNEL_SRCS:.c=.o)
OBJS = $(BOOT_OBJ) $(ASM_OBJS) $(KERNEL_OBJS)

# Output kernel binary
KERNEL = myos.bin

.PHONY: all clean

all: $(KERNEL)

$(KERNEL): $(OBJS)
	$(CC) -T src/linker.ld -o $@ $(LDFLAGS) $(OBJS) -lgcc

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

%.o: %.asm
	$(AS) $(ASFLAGS) $< -o $@

clean:
	rm -f $(KERNEL) $(OBJS)

run: $(KERNEL)
	qemu-system-i386 -kernel $(KERNEL) 