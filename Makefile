# Compiler/Assembler/Linker flags
CC = "C:/MinGW/bin/gcc.exe"
AS = nasm
CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -m32 -I./src/include -fno-stack-protector -nostdinc -fno-builtin
ASFLAGS = -f elf32
LDFLAGS = -ffreestanding -O2 -nostdlib -m32 -Wl,--build-id=none

# Source files
BOOT_SRC = src/boot/multiboot.asm
ASM_SRCS = src/kernel/interrupt.asm src/kernel/gdt.asm
KERNEL_SRCS = src/kernel/kernel.c \
              src/kernel/string.c \
              src/kernel/terminal.c \
              src/kernel/keyboard.c \
              src/kernel/memory.c \
              src/kernel/process.c \
              src/kernel/fs.c \
              src/kernel/mouse.c \
              src/kernel/sound.c \
              src/kernel/hal.c \
              src/kernel/driver.c \
              src/kernel/pci.c \
              src/kernel/net/netstack.c \
              src/drivers/storage/ata.c \
              src/drivers/network/rtl8139.c \
              src/apps/notepad.c \
              src/apps/calculator.c

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
	"C:/MinGW/bin/gcc.exe" -T linker.ld -o $@ $(LDFLAGS) $(OBJS)

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

%.o: %.asm
	$(AS) $< -o $@ $(ASFLAGS)

clean:
	rm -f $(KERNEL) $(OBJS)