[bits 32]
[global _start]
global kernel_stack_top
[extern kmain]

section .bss
align 16
kernel_stack_bottom:
    resb 16384          ; 16ko
kernel_stack_top:

section .text
_start:
    mov esp, kernel_stack_top   ; initialise la pile avant tout appel C
    xor ebp, ebp             
    call kmain
    cli
    hlt
