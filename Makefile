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
              src/kernel/net/ipv6.c \
              src/kernel/net/nat.c \
              src/kernel/net/dhcp.c \
              src/kernel/net/dns.c \
              src/kernel/net/firewall.c \
              src/kernel/net/nettest.c \
              src/kernel/net/ssl.c \
              src/kernel/net/http.c \
              src/kernel/net/ftp.c \
              src/kernel/net/smtp.c \
              src/kernel/net/pop3.c \
              src/kernel/net/imap.c \
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
	$(CC) -T src/linker.ld -o $@ $(LDFLAGS) $(OBJS) -lgcc

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

%.o: %.asm
	$(AS) $(ASFLAGS) $< -o $@

clean:
	rm -f $(KERNEL) $(OBJS)

run: $(KERNEL)
	qemu-system-i386 -kernel $(KERNEL) 