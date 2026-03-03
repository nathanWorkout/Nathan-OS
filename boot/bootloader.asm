org 0x0000
bits 16

start:
    jmp main

main:
    mov ax, 0x07C0
    mov ds, ax
    mov si, message

print_loop:
    lodsb
    or al, al
    jz done
    mov ah, 0x0E
    int 0x10
    jmp print_loop

done:
    cli
    hlt

message db 'Hello, World!', 0
times 510 - ($ - $$) db 0
dw 0xAA55