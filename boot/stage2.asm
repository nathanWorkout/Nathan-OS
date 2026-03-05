org 0x7E00
bits 16

; En x86, il y a 2 façons de parler au matériel :
; 1 -> la mémoire RAM (mov)
; 2 -> les ports I/O : bus séparé de la RAM, 65536 ports numérotés de 0x0000 à 0xFFFF, chaque périphérique a ses propres ports
; On choisit le registre al car le port 0x92 est un port 8 bits (al = moitié basse de ax, 8 bits)
; Activation de la ligne A20 via le port 0x92 
; Sans A20 le bit 21 des adresses est bloqué à 0, impossible d'accéder au delà de 1Mo de mémoire

    in al, 0x92         ; Lit l'octet actuel du port 0x92 (System Control Port A)
    or al, 0b00000010   ; Met le bit 1 à 1 pour activer A20
    and al, 0b11111110  ; Met le bit 0 à 0 pour éviter le reset machine
    out 0x92, al        ; Renvoie l'octet modifié au port 0x92

    ; On teste si A20 est bien activé via un test mémoire
    ; Sans A20 : 0x100000 et 0x000000 pointent au même endroit physique (bit 21 ignoré)
    ; Avec A20 : 0x100000 et 0x000000 sont deux adresses séparées
    mov byte [0x000000], 0x55   ; Écrit 0x55 à l'adresse 0x000000
    mov al, [0x100000]          ; Lit ce qui est à 0x100000
    cmp al, 0x55                ; Si pareil → A20 inactif, sinon → A20 actif
    jne A20_OK                  ; A20 actif, on continue


    mov ax, 0x2401      ; Fonction pour activer le port A20 via le BIOS 
    int 0x15            ; Interruption pour activer A20 via le BIOS (alternative au port 0x92)
    jnc A20_OK            ; Si pas d'erreur, A20 est activé, sinon échec (c'est la différence avec jne la)

A20_FAIL:
    cli
    hlt

A20_OK:
    ; A20 est maintenant activé, on peut accéder à toute la mémoire au-delà de 1Mo

hang:
    cli                 ; Désactive les interruptions - temporaire, sera remplacé par la suite du bootloader
    hlt                 ; Stoppe le CPU
    jmp hang            ; Si le CPU se réveille (NMI), on reboucle