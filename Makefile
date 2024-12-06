# Compiler/Assembler/Linker flags
CC = $(subst /,\,$(CURDIR))\cross-compiler\bin\i686-elf-gcc.exe
AS = $(subst /,\,$(CURDIR))\nasm\nasm-2.16.01\nasm.exe
CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -m32 \
         -I$(subst /,\,$(CURDIR))/src/kernel/include \
         -I$(subst /,\,$(CURDIR))/cross-compiler/lib/gcc/i686-elf/7.1.0/include \
         -I$(subst /,\,$(CURDIR))/cross-compiler/i686-elf/include \
         -fno-stack-protector -nostdinc -fno-builtin
ASFLAGS = -f elf32
LDFLAGS = -ffreestanding -O2 -nostdlib -m32 -Wl,--build-id=none

# Source files
BOOT_SRC = src/boot/multiboot.asm
ASM_SRCS = src/kernel/interrupt_asm.asm src/kernel/gdt_asm.asm
KERNEL_SRCS = src/kernel/kernel.c \
              src/kernel/string.c \
              src/kernel/terminal.c \
              src/kernel/keyboard.c \
              src/kernel/memory.c \
              src/kernel/kheap.c \
              src/kernel/process.c \
              src/kernel/test_process.c \
              src/kernel/fs.c \
              src/kernel/mouse.c \
              src/kernel/pic.c \
              src/kernel/sound.c \
              src/kernel/hal.c \
              src/kernel/driver.c \
              src/kernel/pci.c \
              src/kernel/interrupt.c \
              src/kernel/net/netstack.c \
              src/kernel/graphics.c \
              src/kernel/signal.c \
              src/kernel/acpi.c \
              src/kernel/timer.c \
              src/kernel/idt.c \
              src/kernel/cursor.c \
              src/kernel/tss.c \
              src/kernel/command.c \
              src/kernel/shell.c \
              src/drivers/storage/ata.c \
              src/drivers/network/rtl8139.c \
              src/kernel/window.c \
              src/apps/notepad.c \
              src/apps/calculator.c \
              src/kernel/gdt.c

# Object files
BOOT_OBJ = $(BOOT_SRC:.asm=.o)
ASM_OBJS = $(ASM_SRCS:.asm=.o)
KERNEL_OBJS = $(KERNEL_SRCS:.c=.o)
OBJS = $(BOOT_OBJ) $(ASM_OBJS) $(KERNEL_OBJS)

# Output files
KERNEL = myos.bin
ISO = myos.iso

# Create necessary directories
$(shell mkdir -p src/kernel/net src/drivers/storage src/drivers/network 2>NUL)

.PHONY: all clean run iso

all: $(KERNEL)

$(KERNEL): $(OBJS)
	$(CC) -T linker.ld -o $(KERNEL) $(LDFLAGS) $(OBJS)

%.o: %.c
	@if not exist $(dir $@) mkdir $(dir $@)
	$(CC) -c $< -o $@ $(CFLAGS)

%.o: %.asm
	@if not exist $(dir $@) mkdir $(dir $@)
	$(AS) $< -o $@ $(ASFLAGS)

iso: $(KERNEL)
	@if not exist isodir\boot\grub mkdir isodir\boot\grub
	@copy $(KERNEL) isodir\boot\ /Y
	@echo menuentry "MyOS" { > isodir\boot\grub\grub.cfg
	@echo     multiboot /boot/myos.bin >> isodir\boot\grub\grub.cfg
	@echo } >> isodir\boot\grub\grub.cfg
	grub-mkrescue -o $(ISO) isodir

run: iso
	qemu-system-i386 -cdrom $(ISO)

clean:
	@del /F /Q $(subst /,\,$(OBJS)) $(KERNEL) $(ISO) 2>NUL
	@if exist isodir rmdir /S /Q isodir