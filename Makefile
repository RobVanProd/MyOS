# Compiler/Assembler/Linker flags
CC = $(subst /,\,$(CURDIR))\cross-compiler\bin\i686-elf-gcc.exe
AS = "C:\Program Files\NASM\nasm.exe"
CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -m32 -I$(subst /,\,$(CURDIR))\src\include -fno-stack-protector -nostdinc -fno-builtin
ASFLAGS = -f elf32
LDFLAGS = -ffreestanding -O2 -nostdlib -m32 -Wl,--build-id=none

# Source files
BOOT_SRC = $(subst /,\,$(CURDIR))\src\boot\multiboot.asm
ASM_SRCS = $(subst /,\,$(CURDIR))\src\kernel\interrupt.asm $(subst /,\,$(CURDIR))\src\kernel\gdt.asm
KERNEL_SRCS = $(subst /,\,$(CURDIR))\src\kernel\kernel.c \
              $(subst /,\,$(CURDIR))\src\kernel\string.c \
              $(subst /,\,$(CURDIR))\src\kernel\terminal.c \
              $(subst /,\,$(CURDIR))\src\kernel\keyboard.c \
              $(subst /,\,$(CURDIR))\src\kernel\memory.c \
              $(subst /,\,$(CURDIR))\src\kernel\kheap.c \
              $(subst /,\,$(CURDIR))\src\kernel\process.c \
              $(subst /,\,$(CURDIR))\src\kernel\fs.c \
              $(subst /,\,$(CURDIR))\src\kernel\mouse.c \
              $(subst /,\,$(CURDIR))\src\kernel\pic.c \
              $(subst /,\,$(CURDIR))\src\kernel\sound.c \
              $(subst /,\,$(CURDIR))\src\kernel\hal.c \
              $(subst /,\,$(CURDIR))\src\kernel\driver.c \
              $(subst /,\,$(CURDIR))\src\kernel\pci.c \
              $(subst /,\,$(CURDIR))\src\kernel\interrupt.c \
              $(subst /,\,$(CURDIR))\src\kernel\net\netstack.c \
              $(subst /,\,$(CURDIR))\src\drivers\storage\ata.c \
              $(subst /,\,$(CURDIR))\src\drivers\network\rtl8139.c \
              $(subst /,\,$(CURDIR))\src\apps\notepad.c \
              $(subst /,\,$(CURDIR))\src\apps\calculator.c

# Object files
BOOT_OBJ = $(BOOT_SRC:.asm=.o)
ASM_OBJS = $(ASM_SRCS:.asm=.o)
KERNEL_OBJS = $(KERNEL_SRCS:.c=.o)
OBJS = $(BOOT_OBJ) $(ASM_OBJS) $(KERNEL_OBJS)

# Output files
KERNEL = $(subst /,\,$(CURDIR))\myos.bin
ISO = $(subst /,\,$(CURDIR))\myos.iso

# Create necessary directories
$(shell mkdir $(subst /,\,$(CURDIR))\src\kernel\net $(subst /,\,$(CURDIR))\src\drivers\storage $(subst /,\,$(CURDIR))\src\drivers\network 2>NUL)

.PHONY: all clean run iso

all: $(KERNEL)

$(KERNEL): $(OBJS)
	$(CC) -T $(subst /,\,$(CURDIR))\linker.ld -o $@ $(LDFLAGS) $(OBJS)

%.o: %.c
	@mkdir $(dir $@) 2>NUL
	$(CC) -c $< -o $@ $(CFLAGS)

%.o: %.asm
	@mkdir $(dir $@) 2>NUL
	$(AS) $< -o $@ $(ASFLAGS)

iso: $(KERNEL)
	@if not exist $(subst /,\,$(CURDIR))\isodir\boot\grub mkdir $(subst /,\,$(CURDIR))\isodir\boot\grub
	@copy $(KERNEL) $(subst /,\,$(CURDIR))\isodir\boot\ /Y
	@echo menuentry "MyOS" { > $(subst /,\,$(CURDIR))\isodir\boot\grub\grub.cfg
	@echo     multiboot /boot/myos.bin >> $(subst /,\,$(CURDIR))\isodir\boot\grub\grub.cfg
	@echo } >> $(subst /,\,$(CURDIR))\isodir\boot\grub\grub.cfg
	grub-mkrescue -o $(ISO) $(subst /,\,$(CURDIR))\isodir

run: iso
	qemu-system-i386 -cdrom $(ISO)

clean:
	del /F /Q $(OBJS) $(KERNEL) $(ISO)
	rmdir /S /Q $(subst /,\,$(CURDIR))\isodir 2>NUL