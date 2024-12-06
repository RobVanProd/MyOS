ENTRY(_start)

SECTIONS
{
    . = 0x100000;    /* Load the kernel at 1MB */

    .text ALIGN(4K) : 
    {
        *(.multiboot)
        *(.text*)
    }

    .rodata ALIGN(4K) :
    {
        *(.rodata*)
    }

    .data ALIGN(4K) :
    {
        *(.data*)
    }

    .bss ALIGN(4K) :
    {
        *(COMMON)
        *(.bss*)
        *(.stack)
    }

    end = .;    /* Define the 'end' symbol here */
}
