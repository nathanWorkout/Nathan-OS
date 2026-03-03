org 0x0000      ; L'ffset part de 0, ds = 0x07C0 se chargera de pointer à 0x7C00 (0x07C0 * 16 + 0x00000 = 0x7C00)
bits 16         ; Mode réel, 16 bits

start:
    jmp main

main:
; 0X07C0 car le CPU fait automatiquement 0x07C0 * 16 + 0x0000 (pour le offset) = 0x7C00, l'adresse de début du bootloader

    mov ax, 0x07C0      ; On stocke l'adresse de début du bootloader dans ax
    mov ds, ax          ; On stocke cette adresse dans le segment 0x07C0 (car le CPU utilise des segments pour accéder à la mémoire)    
    mov si, message     ; Charge l'adresse du message dans si

print_loop:
    lodsb               ; Lit le caractère pointé par si dans al et incrémente si (merveille de l'assembleur)
    or al, al           ; Teste si al est nul (fin de la chaîne)
    jz done             ; Si c'est le cas, on saute à done
    mov ah, 0x0E
    int 0x10            
    jmp print_loop

done:
    cli                 ; Désactive les interruptions
    hlt                 ; Met le CPU en pause jusqu'à la prochaine interruption (qui n'arrivera jamais)

message db 'Hello, World!', 0
times 510 - ($ - $$) db 0 
dw 0xAA55               ; Signature de fin de secteur pour les bootloaders (doit être à la fin du secteur de 512 octets)