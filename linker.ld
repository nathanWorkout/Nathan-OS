ENTRY(_start)
KERNEL_VIRT_BASE = 0xFFFFFFFF80000000;
KERNEL_PHYS_BASE = 0x00100000;
SECTIONS
{
    . = KERNEL_VIRT_BASE + KERNEL_PHYS_BASE;
    kernel_virt_start = .;
    kernel_phys_start = . - KERNEL_VIRT_BASE;
    _text_start = .;
    .text ALIGN(4096) :
    {
        *(.text)
        *(.text.*)
    }
    _text_end = .;
    _rodata_start = .;
    .rodata ALIGN(4096) :
    {
        *(.rodata)
        *(.rodata.*)
    }
    _rodata_end = .;
    _data_start = .;
    .data ALIGN(4096) :
    {
        *(.data)
        *(.data.*)
    }
    _data_end = .;
    _bss_start = .;
    .bss ALIGN(4096) :
    {
        *(.bss)
        *(.bss.*)
        *(COMMON)
    }
    _bss_end = .;
    kernel_virt_end = .;
    kernel_phys_end = . - KERNEL_VIRT_BASE;
}