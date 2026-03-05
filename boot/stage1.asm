org 0x0000     
bits 16         

start:
    jmp main

main:
; Nous sommes au début dans le secteur 0 du disque dur : on appelle ça le MBR (Master Boot Record) ou stage 1 du bootloader
; Celui-ci fait 512 octets et doit se terminer par les deux octets 0xAA55 pour être reconnu comme un bootloader valide par le BIOS
; Or, 512 octets ne suffit pas pour contenir un kernel complet, c'est pourquoi nous devons acceder au 2eme secteur ou stage 2 du bootloader

    cli                 ; Désactive les interruptions pendant l'initialisation des segments
    xor ax, ax          ; Met ax à 0
    mov ds, ax          ; Data Segment à 0
    mov es, ax          ; Extra Segment à 0

    mov [boot_drive], dl        ; Sauvegarde le numéro de disque fourni par le BIOS

    mov ax, 0x7000      ; Cette adresse car elle est libre en mémoire et ne risque pas d'écraser le bootloader
    mov ss, ax          ; Stack Segment à 0x7000
    mov sp, 0x0000      ; Stack Pointer à 0 → wrappe à 0xFFFF, pile de 64Ko propre et alignée sur 2
    sti                 ; Réactive les interruptions

    mov ax, 0x07E0      ; Adresse de destination en mémoire pour le secteur lu (0x7E00, juste après le bootloader)
    mov es, ax          ; On passe l'adresse a ax car es est un registre de segment contrairement a ax qui est général
    xor bx, bx          ; Initialise l'offset dans la destination (es:bx = 0x7E00:0x0000 = 0x7E00)
    mov ah, 0x02        ; Fonction 0x02 : Lire des secteurs
    mov al, 0x01        ; Lire 1 secteur
    mov ch, 0x00        ; CH = cylindre 0 (numéro de piste du disque)
    mov cl, 0x02        ; Secteur 2
    mov dh, 0x00        ; Tête 0 (surface du plateau du disque dur)
    mov dl, [boot_drive]        ; Utilise le vrai numéro de disque fourni par le BIOS
    int 0x13            ; Lit le secteur 2 du disque dur
    jmp 0x07E0:0x0000   ; Far jump vers le stage 2 (recharge cs correctement)

boot_drive db 0

times 510 - ($ - $$) db 0 
dw 0xAA55