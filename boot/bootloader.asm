org 0x0000     
bits 16         

start:
    jmp main

main:
; 0X07C0 car le CPU fait automatiquement 0x07C0 * 16 + 0x0000 (pour le offset) = 0x7C00, l'adresse de début du bootloader

    cli                 ; Désactive les interruptions pendant l'initialisation des segments
    xor ax, ax          ; Met ax à 0
    mov ds, ax          ; Data Segment à 0
    mov es, ax          ; Extra Segment à 0

    mov ax, 0x7000      ; Cette adresse car elle est libre en mémoire et ne risque pas d'écraser le bootloader
    mov ss, ax          ; Stack Segment à 0x7000
    mov sp, 0xFFFF      ; Stack Pointer à la fin de la pile (0x7000 + 0xFFFF = 0x7FFF, juste avant le début
    sti                 ; Réactive les interruptions


sector2:
    mov ah, 0x02        ; Fonction 0x02 : Lire des secteurs
    mov al, 0x01        ; Lire 1 secteur
    mov ch, 0x00        ; CH = cylindre 0 (numéro de piste du disque, le numéro est utile pour trouver le secteur à lire(passionant))
    mov cl, 0x02        ; Secteur 2
    mov dh, 0x00        ; Tête 0(surface du plateau du disque dur)
    mov dl, 0x80        ; Disque dur 0
    mov ax, 0x7E0       ; Adresse de destination en mémoire pour le secteur lu (0x7E00, juste après le bootloader)
    mov es, ax          ; On passe l'adresse a ax car es est un registre de segment contrairement a ax quii est général
    xor bx, bx          ; Initialise l'offset dans la destination (es) sinon il pourra mettre une valeure aléatoire (ont met bx a l'intérieur de es pour que le secteur lu soit stocké à l'adresse es:bx, soit 0x7E00:0x0000 = 0x7E00 et bx par convention)
    int 0x13            ; On passe enfin dans le secteur 2 du disque dur
    jmp 0x7E00          ; Saut vers le secteur 2 lu (0x7E00)




done:
    cli                 ; Désactive les interruptions
    hlt                 ; Met le CPU en pause jusqu'à la prochaine interruption (qui n'arrivera jamais)


messageChangeMode db "Le bootloader a lu le secteur 2 du disque dur !", 0
times 510 - ($ - $$) db 0 
dw 0xAA55            