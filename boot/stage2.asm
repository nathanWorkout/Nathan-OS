org 0x7e00
bits 16

; en x86, il y a 2 façons de parler au matériel :
; 1 -> la mémoire ram (mov)
; 2 -> les ports i/o : bus séparé de la ram, 65536 ports numérotés de 0x0000 à 0xffff, chaque périphérique a ses propres ports
; on choisit le registre al car le port 0x92 est un port 8 bits (al = moitié basse de ax, 8 bits)
; activation de la ligne a20 via le port 0x92 
; sans a20 le bit 21 des adresses est bloqué à 0, impossible d'accéder au delà de 1mo de mémoire

    ; on remet ds à 0 au début du stage2 car le far jump depuis stage1 a rechargé cs
    ; mais ds peut valoir n'importe quoi selon le BIOS
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7000      ; pile temporaire propre pendant le stage2

    in al, 0x92         ; lit l'octet actuel du port 0x92 (system control port a)
    or al, 0b00000010   ; met le bit 1 à 1 pour activer a20
    and al, 0b11111110  ; met le bit 0 à 0 pour éviter le reset machine
    out 0x92, al        ; renvoie l'octet modifié au port 0x92

    ; on teste si a20 est bien activé via un test mémoire
    ; sans a20 : 0x100000 et 0x000000 pointent au même endroit physique (bit 21 ignoré)
    ; avec a20 : 0x100000 et 0x000000 sont deux adresses séparées
    ; en mode réel on ne peut pas adresser 0x100000 directement, on utilise fs:0x0010 avec fs=0xffff
    ; (0xffff << 4) + 0x0010 = 0xffff0 + 0x10 = 0x100000

    mov di, 0x0500              ; adresse safe hors IVT (IVT = 0x0000-0x03FF)
    mov byte [di], 0x55         ; écrit 0x55 à 0x0500
    mov ax, 0xffff
    mov fs, ax
    mov al, [fs:0x0510]         ; lit ce qui est à 0x100500 (même décalage)
    cmp al, 0x55                ; si pareil -> a20 inactif, sinon -> a20 actif
    jne a20_ok                  ; a20 actif, on continue

    mov ax, 0x2401      ; fonction pour activer le port a20 via le bios 
    int 0x15            ; interruption pour activer a20 via le bios (alternative au port 0x92)
    jnc a20_ok          ; si pas d'erreur, a20 est activé, sinon échec (c'est la différence avec jne la)

a20_fail:
    cli
    hlt

a20_ok:
    sti                 ; réactive les interruptions pour que int 0x13 fonctionne
; a20 est maintenant activé, on peut accéder à toute la mémoire au-delà de 1mo

; =====================================================
; LECTURE LBA ÉTENDU (int 0x13 / ah=0x42)
; =====================================================
; On abandonne le CHS hardcodé (trop fragile, géométrie variable selon QEMU/hardware)
; et on passe à l'adressage LBA étendu qui utilise juste le numéro de secteur direct
;
; Le DAP (Disk Address Packet) est une structure de 16 octets en mémoire :
;   offset 0x00 : taille du DAP (0x10 = 16 octets)
;   offset 0x01 : réservé (0x00)
;   offset 0x02 : nombre de secteurs à lire (word)
;   offset 0x04 : offset du buffer destination (word)
;   offset 0x06 : segment du buffer destination (word)
;   offset 0x08 : numéro de secteur LBA (qword, 64 bits)
;
; LBA du répertoire racine = fat_start + (fat_count * fat_size)
; fat_start = reserved_sectors = 32
; fat_count = 2 (deux copies de la FAT)
; fat_size  = 512 secteurs par FAT
; lba_root  = 32 + 2 * 512 = 1056

    ; on remplit le DAP pour lire le répertoire racine (LBA 1056)
    ; le DAP est stocké à 0x7000 (zone libre, pile est à ss:sp=0x0000:0x7000 donc on descend)
    ; on le met à 0x0600 pour éviter tout conflit avec la pile à 0x7000

    mov word [dap],        0x0010   ; taille du DAP = 16 octets
    mov word [dap + 0x02], 0x0001   ; lire 1 secteur
    mov word [dap + 0x04], 0x0000   ; offset destination = 0x0000
    mov word [dap + 0x06], 0x0900   ; segment destination = 0x0900 -> adresse physique 0x9000
    mov dword [dap + 0x08], 1056    ; LBA bas : secteur 1056 (répertoire racine)
    mov dword [dap + 0x0C], 0       ; LBA haut : 0 (disque < 2To)

    mov ah, 0x42        ; fonction 0x42 : lire via LBA étendu
    mov dl, 0x80        ; premier disque dur
    mov si, dap         ; ds:si pointe sur le DAP
    int 0x13
    jc disk_error

    ; on pointe ds sur 0x9000 pour lire les entrées du répertoire
    ; ATTENTION : ds sera remis à 0 avant lgdt plus bas
    mov ax, 0x9000
    mov ds, ax
    mov si, 0x0000      ; si pointe sur la première entrée du répertoire racine

check_entry:            ; début de la boucle - apres la lecture disque, on ne relit pas à chaque tour
    mov al, [si]    
    cmp al, 0x00        ; vérifie que le premier octet du secteur n'est pas 0 (indique un secteur vide)
    je dir_end          ; fin du répertoire, le kernel n'existe pas sur le disque
    cmp al, 0xe5        ; vérifie que le premier octet du secteur n'est pas 0xe5 (l'entrée est inutilisée)
    je next_entry
    mov al, [si + 11]   ; offset 11 = octet d'attributs de l'entrée (pas 10 ; offset 10 = dernier octet du nom)
    cmp al, 0x0f        ; 0x0f = attribut lfn (long file name), entrée à ignorer car pas au format 8.3
    je next_entry
    mov di, kernel_name ; adresse du nom de fichier attendu
    mov [cs:current_entry], si  ; sauvegarde le début de l'entrée actuelle (cs: car ds=0x9000)
    push si             ; sauvegarde si sur la pile - read_name va l'incrémenter, il faut pouvoir revenir
    mov cx, 11          ; kernel  bin = 11 octets au format 8.3

read_name: 
    mov al, [si]        ; lire un octet du secteur (ds=0x9000, correct)
    mov bl, [cs:di]     ; lire un octet du nom attendu (cs: car kernel_name est dans stage2, pas à 0x9000)
    cmp al, bl
    jne name_mismatch   ; les octets diffèrent : ce n'est pas le bon fichier
    inc si              ; passer à l'octet suivant du secteur
    inc di              ; passer à l'octet suivant du nom attendu
    loop read_name      ; répéter pour les 11 octets du nom
    pop si              ; les 11 octets correspondent : on restaure si (current_entry suffit mais on dépile proprement)
    jmp kernel_found    ; si le nom correspond, on a trouvé le kernel

name_mismatch:
    pop si              ; restaure si avant de passer à l'entrée suivante (dépile ce qu'on a empilé avant read_name)

next_entry: 
    add si, 32          ; 32 car chaque entrée de répertoire fait 32 octets, on passe à la prochaine entrée
    jmp check_entry     ; retour au début de la boucle

dir_end:                ; on a parcouru tout le répertoire sans trouver kernel.bin
    cli
    hlt                 ; arrêt machine - le kernel est introuvable, rien à faire de plus

kernel_found:
    mov si, [cs:current_entry]  ; revenir au début de l'entrée du kernel trouvée (cs: car ds=0x9000)
    xor ebx, ebx
    mov bx, [si + 0x14]     ; cluster haut (16 bits)
    shl ebx, 16             ; décale vers le haut
    mov bx, [si + 0x1a]     ; cluster bas (16 bits)
    ; ebx contient maintenant le numéro de cluster complet sur 32 bits

    ; =====================================================
    ; LECTURE DE LA FAT POUR SUIVRE LA CHAÎNE DE CLUSTERS
    ; =====================================================
    ; chaque entrée FAT32 fait 4 octets
    ; offset dans la FAT = cluster * 4
    ; secteur FAT = fat_start + (offset / 512)
    ; offset dans ce secteur = offset % 512
    ;
    ; fat_start = 32 (reserved_sectors)

    mov eax, ebx         ; numéro de cluster
    shl eax, 2           ; multiplie par 4 car chaque entrée fat32 fait 4 octets
    xor edx, edx         
    mov ecx, 512         ; diviseur : 512 octets par secteur
    div ecx              ; eax = secteur relatif dans la FAT, edx = offset dans ce secteur
    add eax, 32          ; ajoute fat_start pour avoir le LBA réel

    push edx             ; sauvegarde l'offset dans le secteur FAT

    ; lire le secteur FAT via LBA étendu
    mov dword [dap + 0x08], eax     ; LBA du secteur FAT
    mov dword [dap + 0x0C], 0
    mov word  [dap + 0x02], 1       ; 1 secteur
    mov word  [dap + 0x04], 0x0000  ; offset destination
    mov word  [dap + 0x06], 0x0800  ; segment 0x0800 -> adresse physique 0x8000

    xor ax, ax
    mov ds, ax          ; ds=0 pour que ds:si pointe correctement sur le DAP
    mov ah, 0x42
    mov dl, 0x80
    mov si, dap
    int 0x13
    jc disk_error

    pop esi             ; récupère l'offset dans le secteur FAT

follow_chain:
    ; lire l'entrée FAT à l'offset esi dans le buffer 0x8000
    mov ax, 0x0800
    mov es, ax
    mov eax, [es:esi]
    and eax, 0x0fffffff     ; masque les 4 bits hauts réservés par FAT32
    cmp eax, 0x0ffffff8
    jae load_clusters       ; >= 0x0ffffff8 = dernier cluster -> on charge
    mov ebx, eax            ; sinon ebx = prochain cluster, on suit la chaîne
    jmp kernel_found        ; recalcule le secteur FAT pour ce cluster

load_clusters:
    ; =====================================================
    ; CHARGEMENT DU KERNEL EN MÉMOIRE
    ; =====================================================
    ; ebx = premier cluster du kernel
    ; lba = data_start + (cluster - 2) * sectors_per_cluster
    ; data_start = reserved + fat_count * fat_size = 32 + 2*512 = 1056
    ; sectors_per_cluster = 8 (valeur par défaut QEMU avec -F 32)
    ; destination : 0x2000:0x0000 = adresse physique 0x20000

    mov eax, ebx
    sub eax, 2              ; les clusters commencent à 2 en FAT32
    mov ecx, 8              ; sectors_per_cluster = 8
    mul ecx               
    add eax, 1056           ; data_start = 32 + 2*512

    ; lire 8 secteurs (1 cluster) via LBA étendu vers 0x20000
    mov dword [dap + 0x08], eax     ; LBA du premier secteur du cluster
    mov dword [dap + 0x0C], 0
    mov word  [dap + 0x02], 8       ; 8 secteurs = 1 cluster
    mov word  [dap + 0x04], 0x0000  ; offset destination = 0
    mov word  [dap + 0x06], 0x2000  ; segment 0x2000 -> adresse physique 0x20000

    xor ax, ax
    mov ds, ax
    mov ah, 0x42
    mov dl, 0x80
    mov si, dap
    int 0x13
    jc disk_error

    ; on remet ds à 0 AVANT lgdt - sinon lgdt lit gdtd au mauvais endroit
    xor ax, ax
    mov ds, ax

    ; =====================================================
    ; BASCULE EN MODE PROTÉGÉ 32 BITS
    ; =====================================================
    lgdt [gdtd]         ; charge la GDT
    mov eax, cr0
    or eax, 1           ; bit PE (Protection Enable) = 1
    mov cr0, eax
    jmp 0x08:protected_mode     ; far jump qui flush le pipeline et active le mode protégé

disk_error: 
    cli 
    hlt

; =====================================================
; DONNÉES
; =====================================================

; DAP (Disk Address Packet) pour int 0x13/0x42
; structure de 16 octets utilisée par le LBA étendu
dap:
    db 0x10         ; taille du DAP
    db 0x00         ; réservé
    dw 0x0000       ; nombre de secteurs
    dw 0x0000       ; offset destination
    dw 0x0000       ; segment destination
    dd 0x00000000   ; LBA bas
    dd 0x00000000   ; LBA haut

; le format FAT32 utilise des noms de 11 octets : 8 pour le nom + 3 pour l'extension
kernel_name db "KERNEL  BIN"    ; en majuscules : FAT32 stocke les noms en majuscules
current_entry dw 0              ; sauvegarde le début de l'entrée courante

; =====================================================
; GDT (Global Descriptor Table)
; =====================================================
; le cpu lit une entrée dans la gdt qui contient :
; base address, limit, type (code/data), privilege level, flags
; la gdt est indispensable pour le mode protégé

gdt_start:

gdt_null: dq 0x0000000000000000 ; entrée nulle obligatoire (le cpu plante si on l'oublie)

; 0x08 - ring 0 (segment code kernel)
gdt_code: 
    dw 0xffff        ; limit low
    dw 0x0000        ; base low
    db 0x00          ; base middle
    db 10011010b     ; access byte (présent, ring 0, code, exécutable, readable)
    db 11001111b     ; flags (32 bits, granularité 4ko) + limit high
    db 0x00          ; base high

; 0x10 - ring 0 (segment data kernel)
gdt_data:
    dw 0xffff
    dw 0x0000
    db 0x00
    db 10010010b     ; access byte (présent, ring 0, data, writable)
    db 11001111b
    db 0x00

; 0x18 - ring 3 (segment code userland)
gdt_user_code:
    dw 0xffff        ; limit low
    dw 0x0000        ; base low
    db 0x00          ; base middle
    db 11111010b     ; access byte (ring 3 : bits 5-6 = 11)
    db 11001111b     ; flags + limit high
    db 0x00          ; base high

; 0x20 - ring 3 (segment data userland)
gdt_user_data:
    dw 0xffff
    dw 0x0000
    db 0x00
    db 11110010b     ; access byte (ring 3 : bits 5-6 = 11)
    db 11001111b     ; flags + limit high
    db 0x00          ; base high

gdt_end:

; le cpu attend :
; 16 bits : taille - 1
; 32 bits : adresse linéaire de la GDT
gdtd:
    dw gdt_end - gdt_start - 1
    dd gdt_start

; 10011010b     décomposition de l'access byte code ring 0
; 1    -> présent
; 00   -> ring 0
; 1    -> code/data (descriptor type)
; 1    -> executable (code)
; 0    -> non-conforming
; 1    -> readable
; 0    -> accessed (mis à 1 par le cpu automatiquement)

[bits 32]
; on est enfin en 32 bits !

protected_mode:

; recharger les segments avec le sélecteur data ring 0 (0x10)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov esp, 0x90000    ; pile en mode protégé à une adresse propre

; =====================================================
; PARSE ELF ET JUMP VERS LE KERNEL
; =====================================================
; kernel.bin est un ELF32 chargé à 0x20000
; on lit les program headers pour copier chaque segment PT_LOAD
; à son adresse virtuelle (p_vaddr), puis on saute sur e_entry
;
; algo :
; 1. esi = 0x20000  (début du fichier elf en mémoire)
; 2. lire e_entry  à [esi + 0x18]  -> sauvegarder dans edi
; 3. lire e_phoff  à [esi + 0x1c]  -> ebp = esi + e_phoff
; 4. lire e_phnum  à [esi + 0x2c]  -> compteur de boucle dans ecx
; 5. boucle pour chaque segment :
;    lire p_type à [ebp + 0x00]
;    si p_type != 1 (PT_LOAD) -> skip
;    lire p_offset à [ebp + 0x04]
;    lire p_vaddr  à [ebp + 0x08]
;    lire p_filesz à [ebp + 0x10]
;    copier p_filesz octets de (0x20000 + p_offset) vers p_vaddr
;    ebp += 0x20 (taille d'un program header)
; 6. jmp edi (e_entry)

    mov esi, 0x20000            ; début du fichier ELF en mémoire

    mov eax, [esi + 0x18]       ; e_entry : adresse d'entrée du kernel
    mov edi, eax                ; sauvegarde e_entry dans edi pour le jump final

    mov eax, [esi + 0x1c]       ; e_phoff : offset de la table des program headers
    add eax, esi                ; adresse absolue du premier program header
    mov ebp, eax

    movzx ecx, word [esi + 0x2c] ; e_phnum : nombre de segments (champ 16 bits -> 32 bits)

parse_ph_loop:
    cmp ecx, 0
    je done_parsing             ; plus de segments -> on saute vers le kernel

    mov eax, [ebp + 0x00]       ; p_type
    cmp eax, 1                  ; PT_LOAD = 1, seul type à copier (les autres sont des métadonnées)
    jne skip_segment

    ; segment PT_LOAD : copier de (0x20000 + p_offset) vers p_vaddr
    mov eax, [ebp + 0x04]       ; p_offset : offset du segment dans le fichier ELF
    add eax, 0x20000            ; adresse source = base ELF + p_offset
    push esi                    ; sauvegarde esi (base ELF)
    mov esi, eax

    push edi                    ; sauvegarde e_entry
    mov edi, [ebp + 0x08]       ; p_vaddr : adresse destination

    push ecx                    ; rep movsb écrase ecx -> sauvegarde
    mov ecx, [ebp + 0x10]       ; p_filesz : nombre d'octets à copier
    rep movsb                   ; copie ecx octets de [esi] vers [edi]
    pop ecx

    pop edi                     ; restaure e_entry
    pop esi                     ; restaure base ELF

skip_segment:
    add ebp, 0x20               ; program header suivant (taille = 32 octets)
    dec ecx
    jmp parse_ph_loop

done_parsing:
    jmp edi                     ; saute vers e_entry -> le kernel prend la main