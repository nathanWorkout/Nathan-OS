org 0x7c00
bits 16         

start:
    jmp short main
    nop

; Maintenant : le configuration du Bios Parameter Block (BPB) pour que le stage 2 puisse lire le système de fichiers FAT32
; Le BPB est une structure de données qui décrit les caractéristiques du système de fichiers FAT32 (FAT = File Alocation Table)
OEM_NAME:            db "NathanOS"   ; 8 octets pour l'OEM Name
BYTE_BY_SECTOR:      dw 512          ; 1 secteur = 512 octets (1000 secteurs × 512 octets = 512000 octets = 512 KB donc c utile pour calculer la taille d'un fichier)
SECTOR_PER_CLUSTER:  db 8            ; FAT32 stocke les fichiers clusters par clusters donc 1 cluster = plusieurs secteurs -> sector per clusters veut donc dire combien il y a de secteurs par cluster
RESERVED_SECTOR:     dw 32           ; Champ du boot sector des systèmes de fichier FAT, il dit combien de secteurs sont au début du disque pour le système de fichiers (il contient le boot sector et les info FAT)
NUMBER_OF_FAT:       db 2            ; Combien de copies de FAT existe sur le disque dur (La 2eme copie pour la sécurité si la 1ere est corrompue)
ROOT_ENTRY:          dw 0            ; Nombre d'entrée max dans la racine du dossier (0 pour FAT32 car la racine est un cluster comme les autres et donc le nombre d'entrées n'est plus limité)
TOTAL_SECTOR:        dw 0            ; Nombre total de secteurs sur le disque dur en FAT16 (0 pour FAT32, la vraie valeur est dans TOTAL_SECTORS à 0x20)
MEDIA_DESCRIPTOR:    db 0xF8         ; Pour savoir quel type de disque il gère (dans notre cas c'est un disque dur ou SSD)
SECTOR_PER_FAT16:    dw 0            ; Nombre de secteurs par FAT en FAT16 (0 pour FAT32, la vraie valeur est dans FAT_SIZE à 0x24)
SECTOR_PER_TRACK:    dw 63           ; Ce champ dans le BPB indique combien il y a de secteurs sur la piste du disque dur (une ligne circulaire sur la surface) il sert surtout pour les anciens disques durs, mais on le remplit quand même pour être conforme au standard FAT32
NUMBER_OF_HEAD:      dw 255          ; Ce champ dans le BPB indique combien il y a de têtes de lecture/écriture sur le disque dur (une par surface) il sert a calculer la taille du disque : Taille disque=Cylinders x Heads x Sectors per Track x Bytes per Sector
HIDDEN_SECTOR:       dd 0            ; Indique combien de secteurs cachés sur le disque ne font pas partie de cette partition (utile pour savoir quand lire car on skip les secteurs cachés)
TOTAL_SECTORS:       dd 524288       ; Nombre total de secteurs par partition (524288 * 512 = 256Mo)

; Champs étendus FAT32
; Juste pour me rappeller mais depuis le BPB, le repertoire racine commence au cluster 2 
FAT_SIZE:            dd 512          ; Nombre de secteurs utilisés pour chaque FAT
EXTENDED_FLAGS:      dw 0            ; Champ de 16 bits dans le BPB FAT32, il indique quelle FAT est active et si il y en a plusieurs ou réservé des bits
FS_VERSION:          dw 0            ; Version du système de fichiers FAT32 (0x0000 pour la version 0.0)
ROOT_CLUSTER:        dd 2            ; Indique le numéro du premier cluster du répertoire racine (toujours 2 par convention)
FSINFO_SECTOR:       dw 1            ; Indique le numéro de secteur de FSInfo (contient l'espace libre et le prochain cluster libre)
BACKUP_BOOT_SECTOR:  dw 6            ; Indique le numéro de secteur du backup boot sector (une copie de secours du boot sector pour la sécurité)
RESERVED:            times 12 db 0   ; 12 octets de données réservés (généralement mis à 0 dans les FAT32 modernes)
DRIVE_NUMBER:        db 0x80         ; Numéro de lecteur (0x00 pour le lecteur de disquette, 0x80 pour le premier disque dur, 0x81 pour le second disque dur, etc.)
RESERVED1:           db 0            ; Octet réservé pour usage futur / Windows NT, généralement mis à 0
BOOT_SIGNATURE:      db 0x29         ; Signature d'extension de boot (0x29 indique que les champs suivants sont valides)
VOLUME_ID:           dd 0x12345678   ; Identifiant de volume (généralement un nombre aléatoire pour identifier de manière unique la partition)
VOLUME_LABEL:        db "NATHAN OS  "; 11 octets pour le label du volume (généralement le nom de la partition)
FS_TYPE:             db "FAT32   "   ; 8 octets pour le type de système de fichiers


main:
; Nous sommes au début dans le secteur 0 du disque dur : on appelle ça le MBR (Master Boot Record) ou stage 1 du bootloader
; Celui-ci fait 512 octets et doit se terminer par les deux octets 0xAA55 pour être reconnu comme un bootloader valide par le BIOS
; Or, 512 octets ne suffit pas pour contenir un kernel complet, c'est pourquoi nous devons acceder au 2eme secteur ou stage 2 du bootloader

    cli                 ; Désactive les interruptions pendant l'initialisation des segments
    xor ax, ax          ; Met ax à 0
    mov ds, ax          ; Data Segment à 0
    mov es, ax          ; Extra Segment à 0

    mov [boot_drive], dl ; Sauvegarde le numéro de disque fourni par le BIOS

    mov ax, 0x7000      ; Cette adresse car elle est libre en mémoire et ne risque pas d'écraser le bootloader
    mov ss, ax          ; Stack Segment à 0x7000
    mov sp, 0xFFFF      ; Stack Pointer à 0xFFFF, pile de 64Ko propre et alignée sur 2
    sti                 ; Réactive les interruptions

    ; on prépare d'abord tous les registres CHS et ah/al AVANT de toucher es
    ; sinon mov ax, segment écrase ah qui contient la fonction 0x02
    mov ah, 0x02        ; Fonction 0x02 : Lire des secteurs - EN PREMIER avant tout mov ax
    mov al, 0x02        ; Lire 2 secteurs (marge de sécurité si stage2 grossit)
    mov ch, 0x00        ; CH = cylindre 0 (numéro de piste du disque)
    mov cl, 0x03        ; secteur 3 en CHS = LBA 2
    mov dh, 0x00        ; Tête 0 (surface du plateau du disque dur)
    mov dl, [boot_drive] ; Utilise le vrai numéro de disque fourni par le BIOS

    ; maintenant seulement on configure es:bx (destination du int 0x13)
    ; 0x07E0 * 16 = 0x7E00, juste après le bootloader en mémoire
    mov bx, 0x07E0
    mov es, bx          ; es = 0x07E0
    xor bx, bx          ; Offset à 0 -> es:bx = 0x7E00:0x0000 = adresse physique 0x7E00

    int 0x13            ; Lit les secteurs de stage2 depuis le disque
    jc boot_error       ; Si erreur de lecture (carry flag), on s'arrête

    ; =====================================================
    ; FAR JUMP VERS STAGE2
    ; =====================================================
    ; BUG CORRIGÉ : on saute vers cs=0x0000 / offset=0x7E00
    ; Si on utilisait cs=0x07E0 / offset=0x0000, cs vaudrait 0x07E0 dans stage2
    ; et tous les accès [cs:label] seraient décalés : 0x07E0*16 + offset_nasm
    ; alors que NASM a assemblé stage2 avec org 0x7e00 (base 0x0000)
    ; -> les labels kernel_name, dap, boot_drive etc. pointeraient au mauvais endroit
    ; Avec cs=0 / offset=0x7E00 : cs*16 + offset = 0 + 0x7E00 = adresse physique correcte
    db 0xEA             ; opcode far jump
    dw 0x7E00           ; offset 0x7E00  <- CORRIGÉ (était 0x0000)
    dw 0x0000           ; segment cs=0   <- CORRIGÉ (était 0x07E0)

boot_error:
    cli
    hlt

boot_drive db 0

times 510 - ($ - $$) db 0 
dw 0xAA55