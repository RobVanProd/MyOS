ENTRY(kernel_main)
OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386)

SECTIONS
{
    . = 0x100000;    /* Load kernel at 1MB */

    .text ALIGN(4K) : {
        *(.multiboot)  /* Multiboot header first */
        *(.text)       /* Code */
    }

    .rodata ALIGN(4K) : {
        *(.rodata)     /* Read-only data */
    }

    .data ALIGN(4K) : {
        *(.data)       /* Initialized data */
    }

    .bss ALIGN(4K) : {
        *(COMMON)      /* Common symbols */
        *(.bss)        /* Uninitialized data */
    }

    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
        *(.note.gnu.build-id)
    }
}
