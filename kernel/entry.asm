[BITS 64]
[global _start]
[global kernel_stack_top]
[extern kmain]

section .bss
align 16
kernel_stack_bottom:
    resb 16384
kernel_stack_top:

section .text
_start:
    mov ax, 0x10
    mov ss, ax
    mov ds, ax
    mov es, ax
    xor ax, ax
    mov fs, ax
    mov gs, ax

    mov rsp, kernel_stack_top
    and rsp, ~0xF

    xor rbp, rbp

    call kmain

    cli
.halt:
    hlt
    jmp .halt