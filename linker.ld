ENTRY(_start)

SECTIONS {
    . = 0x100000;
    .text : {
        build/entry.o
        *(.text)
    }
    .data : { *(.data) }
    .bss  : { *(.bss)  }
}