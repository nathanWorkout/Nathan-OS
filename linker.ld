ENTRY(_start)

SECTIONS {
    . = 0x100000;

    kernel_start = .;

    .text : {
        build/entry.o
        *(.text)
    }
    .data : { *(.data) }
    .bss  : { *(.bss)  }

    kernel_end = .;
}
