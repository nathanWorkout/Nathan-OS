ENTRY(_start) /* Entry point of the kernel */

SECTIONS
{
    . = 1M; /* Load the kernel at 1MB (0x100000) in x86 */

    .text :
    {
        *(.multiboot) /* Multiboot header first (required by GRUB) */
        *(.text)      /* Executable code */
    }

    .data :
    {
        *(.rodata) /* Read-only data (constant strings) */
        *(.data)   /* Initialized data */
    }

    .bss :
    {
        *(COMMON)  /* Uninitialized global variables */
        *(.bss)    /* Uninitialized data */
    }
}