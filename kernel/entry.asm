[BITS 64]  
[global _start]
[extern kmain]

section .bss
align 16
kernel_stack_bottom:
    resb 16384              
kernel_stack_top:

section .text
_start:
    mov rsp, kernel_stack_top
    and rsp, ~0xF
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    xor eax, eax            ; FS et GS à 0 pour l'instant
    mov fs, ax
    mov gs, ax
    xor rbp, rbp

    call kmain

    ; Sécurité : si kmain retourne quand même
    cli
.halt:
    hlt
    jmp .halt
