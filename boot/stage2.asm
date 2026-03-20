org 0x7e00
bits 16

; en x86, il y a 2 façons de parler au matériel :
; 1 -> la mémoire ram (mov)
; 2 -> les ports i/o : bus séparé de la ram, 65536 ports numérotés de 0x0000 à 0xffff, chaque périphérique a ses propres ports
; on choisit le registre al car le port 0x92 est un port 8 bits (al = moitié basse de ax, 8 bits)
; activation de la ligne a20 via le port 0x92
; sans a20 le bit 21 des adresses est bloqué à 0, impossible d'accéder au delà de 1mo de mémoire

    ; sauvegarder dl EN TOUT PREMIER avant que quoi que ce soit ne l'écrase
    ; dl contient le numéro de disque transmis par le BIOS -> stage1 -> stage2 (convention)
    mov [cs:boot_drive], dl    ; BUG CORRIGÉ : préfixe cs: obligatoire ici
                               ; à ce stade ds peut valoir n'importe quoi (le BIOS ne le garantit pas)
                               ; or boot_drive est dans le segment de code (org 0x7e00)
                               ; cs est toujours correct car c'est lui qui pointe stage2
                               ; sans cs: on risque d'écrire dans ds:boot_drive = adresse aléatoire

    ; on remet ds à 0 au début du stage2 car le far jump depuis stage1 a rechargé cs
    ; mais ds peut valoir n'importe quoi selon le BIOS
    ; BUG CORRIGÉ : cs est maintenant 0x0000 grâce au far jump corrigé dans stage1
    ; (cs=0x0000, offset=0x7E00 au lieu de cs=0x07E0, offset=0x0000)
    ; avec cs=0 : [cs:label] = [0*16 + label_nasm] = adresse physique correcte
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
    jnc a20_ok          ; si pas d'erreur, a20 est activé

a20_fail:
    mov word [0x5000], 0xDEAD   ; DEBUG : a20_fail
    cli
    hlt

a20_ok:
    mov word [0x5002], 0xBEEF   ; DEBUG : a20_ok atteint
    sti                 ; réactive les interruptions pour que int 0x13 fonctionne

; =====================================================
; TEST SUPPORT LBA ÉTENDU (int 0x13 / ah=0x41)
; =====================================================
; avant d'utiliser int 0x13/0x42, on vérifie que le BIOS supporte les extensions LBA
; ah=0x41 : fonction de détection, bx=0x55AA magic requis
; si supporté : carry=0
; si non supporté : carry=1 -> on s'arrête

    mov ah, 0x41
    mov bx, 0x55AA
    mov dl, [boot_drive]    ; ds=0 ici donc [boot_drive] = [0x0000 + offset_nasm] = correct
    int 0x13
    mov word [0x5004], 0xBEEF   ; DEBUG : retour int 0x13 lba check
    mov [0x5006], ax            ; DEBUG : ax après lba check
    jc lba_not_supported        ; carry=1 -> extensions non supportées
    jmp lba_supported           ; carry=0 -> LBA ok (on ignore bx, QEMU ne le met pas à 0xAA55)

lba_not_supported:
    mov word [0x5008], 0xDEAD   ; DEBUG : lba non supporté
    cli
    hlt                         ; LBA étendu non supporté, arrêt

lba_supported:
    mov word [0x500A], 0xBEEF   ; DEBUG : lba supporté

; =====================================================
; LECTURE DU RÉPERTOIRE RACINE (int 0x13 / ah=0x42)
; =====================================================
; On utilise l'adressage LBA étendu qui utilise juste le numéro de secteur direct
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

    mov word [dap],        0x0010   ; taille du DAP = 16 octets
    mov word [dap + 0x02], 0x0001   ; lire 1 secteur
    mov word [dap + 0x04], 0x9000   ; offset destination = 0x9000
    mov word [dap + 0x06], 0x0000   ; segment destination = 0x0000
    mov dword [dap + 0x08], 1056    ; LBA bas : secteur 1056 (répertoire racine)
    mov dword [dap + 0x0C], 0       ; LBA haut : 0 (disque < 2To)

    ; DEBUG : snapshot du DAP avant int 0x13 racine -> 0x5100
    mov word [0x5100], 0xBEEF
    mov ax, [dap]
    mov [0x5110], ax
    mov ax, [dap + 0x02]
    mov [0x5112], ax
    mov ax, [dap + 0x04]
    mov [0x5114], ax
    mov ax, [dap + 0x06]
    mov [0x5116], ax
    mov eax, [dap + 0x08]
    mov [0x5118], eax
    mov eax, [dap + 0x0C]
    mov [0x511C], eax

    mov ah, 0x42        ; fonction 0x42 : lire via LBA étendu
    mov dl, [boot_drive]
    mov si, dap         ; ds:si pointe sur le DAP (ds=0, si=adresse nasm du dap -> correct)
    int 0x13

    mov [0x5102], ax                ; ax après int 0x13 racine
    mov word [0x5104], 0xDEAD
    jc disk_error
    mov word [0x5106], 0x1234       ; carry=0, lecture ok

    ; on pointe ds sur 0x9000 pour lire les entrées du répertoire
    xor ax, ax
    mov ds, ax
    mov si, 0x9000

check_entry:
    mov al, [si]
    cmp al, 0x00        ; premier octet = 0 -> fin du répertoire, plus aucune entrée après
    je dir_end
    cmp al, 0xe5        ; premier octet = 0xe5 -> entrée supprimée, on l'ignore
    je next_entry
    mov al, [si + 11]   ; offset 11 = octet d'attributs de l'entrée
    cmp al, 0x0f        ; 0x0f = attribut LFN (Long File Name), entrée à ignorer car pas au format 8.3
    je next_entry


    ; ds=0x9000 ici donc on DOIT utiliser cs: pour accéder à nos variables
    mov [cs:current_entry], si  ; sauvegarde le début de l'entrée actuelle

    push si             ; sauvegarde si avant la boucle de comparaison

    mov bx, 0           ; bx = index dans kernel_name (0 à 10)
    mov cx, 11          ; "KERNEL  BIN" = 11 octets au format 8.3

read_name:
    mov al, [si]                        ; lire un octet du répertoire (ds=0x9000 -> correct)
    mov dl, [cs:kernel_name + bx]       ; BUG CORRIGÉ : cs: pour lire kernel_name dans le code stage2
                                        ; sans cs: on lirait ds:kernel_name = 0x9000*16 + offset
                                        ; = dans le secteur racine FAT32, pas dans notre chaîne
    cmp al, dl
    jne name_mismatch   ; les octets diffèrent : ce n'est pas le bon fichier
    inc si
    inc bx
    loop read_name      ; répéter pour les 11 octets
    pop si              ; les 11 octets correspondent : dépile proprement
    jmp kernel_found

name_mismatch:
    pop si              ; restaure si avant de passer à l'entrée suivante

next_entry:
    add si, 32          ; chaque entrée de répertoire fait 32 octets
    jmp check_entry

dir_end:
    cli
    hlt                 ; kernel.bin introuvable, arrêt

kernel_found:
    mov si, [cs:current_entry]  ; revenir au début de l'entrée du kernel (cs: car ds=0x9000)
    xor ebx, ebx
    mov bx, [si + 0x14]     ; cluster haut (16 bits, offset 0x14 dans l'entrée FAT32)
    shl ebx, 16             ; décale vers le haut pour former les 32 bits
    mov bx, [si + 0x1a]     ; cluster bas (16 bits, offset 0x1a dans l'entrée FAT32)
    ; ebx contient maintenant le numéro de cluster complet sur 32 bits

    ; destination de chargement : 0x20000 en mémoire physique
    ; on utilise un pointeur 32 bits stocké en mémoire car on est encore en 16 bits
    ; on ne peut pas utiliser de registre 32 bits comme pointeur directement en mode réel
    mov dword [cs:load_dest], 0x20000   ; BUG CORRIGÉ : cs: car ds=0x9000 encore ici

; =====================================================
; BOUCLE DE CHARGEMENT : SUIT LA CHAÎNE FAT ET CHARGE CHAQUE CLUSTER
; =====================================================
; on reste EN MODE RÉEL pendant toute cette boucle
; int 0x13 ne fonctionne plus une fois en mode protégé
; algo :
;   1. calculer le secteur FAT du cluster courant (ebx)
;   2. lire le secteur FAT -> buffer 0x10000
;   3. lire l'entrée FAT à l'offset calculé
;   4. calculer le LBA du cluster courant et charger ses 8 secteurs -> [load_dest]
;   5. avancer load_dest de 8*512 = 4096 octets
;   6. si l'entrée FAT >= 0x0FFFFFF8 -> dernier cluster, on arrête
;   7. sinon ebx = entrée FAT (prochain cluster), retour en 1

load_next_cluster:

    ; étape 1 : calculer secteur FAT et offset pour le cluster ebx
    ; offset dans la FAT (en octets) = cluster * 4 (chaque entrée FAT32 = 4 octets)
    ; secteur FAT = fat_start + (offset / 512)
    ; offset dans le secteur = offset % 512
    ; fat_start = 32 (reserved_sectors du BPB)

    ; ds peut valoir 0x9000 ici, on remet à 0 pour accéder au DAP
    xor cx, cx
    mov ds, cx          ; ds=0 : toutes les variables de stage2 sont maintenant accessibles sans préfixe

    mov eax, ebx        ; numéro de cluster courant
    shl eax, 2          ; multiplie par 4 : offset en octets dans la FAT
    xor edx, edx
    mov ecx, 512        ; 512 octets par secteur
    div ecx             ; eax = secteur relatif dans la FAT, edx = offset dans ce secteur
    add eax, 32         ; ajoute fat_start -> LBA réel du secteur FAT

    push edx            ; sauvegarde l'offset dans le secteur FAT (on en aura besoin après la lecture)

    ; étape 2 : lire le secteur FAT
    ; IMPORTANT : buffer à 0x1000:0x0000 = adresse physique 0x10000
    ; on évite 0x0800:0x0000 = 0x8000 qui chevauche le code stage2 (chargé à 0x7e00, ~600 octets)
    mov dword [dap + 0x08], eax     ; LBA du secteur FAT
    mov dword [dap + 0x0C], 0
    mov word  [dap + 0x02], 1       ; 1 secteur
    mov word  [dap + 0x04], 0x0000  ; offset destination = 0
    mov word  [dap + 0x06], 0x1000  ; segment 0x1000 -> adresse physique 0x10000

    mov ah, 0x42
    mov dl, [boot_drive]
    mov si, dap
    int 0x13
    jc disk_error

    ; étape 3 : lire l'entrée FAT à l'offset edx
    ; le secteur FAT est maintenant à 0x10000, on utilise es=0x1000
    pop esi             ; récupère l'offset dans le secteur FAT
    mov ax, 0x1000      ; segment 0x1000 -> adresse physique 0x10000
    mov es, ax
    mov eax, [es:esi]   ; lit les 4 octets de l'entrée FAT32
    and eax, 0x0fffffff ; masque les 4 bits hauts réservés par FAT32

    ; étape 4 : charger les données du cluster courant
    ; lba = data_start + (cluster - 2) * sectors_per_cluster
    ; data_start = reserved + fat_count * fat_size = 32 + 2*512 = 1056
    ; sectors_per_cluster = 8

    push eax            ; sauvegarde l'entrée FAT (prochain cluster ou marqueur fin)

    mov eax, ebx        ; cluster courant
    sub eax, 2          ; les clusters commencent à 2 en FAT32
    mov ecx, 8          ; sectors_per_cluster = 8
    mul ecx
    add eax, 1056       ; data_start

    ; charger 8 secteurs vers [load_dest]
    ; load_dest contient une adresse 32 bits (ex: 0x20000, 0x21000, 0x22000...)
    ; en mode réel on ne peut pas mettre une adresse 32 bits dans es directement
    ; on décompose : segment = load_dest >> 4, offset = load_dest & 0xF
    ; pour nos adresses (multiples de 0x1000) : segment = load_dest >> 4, offset = 0

    mov ebx, [load_dest]    ; adresse physique destination (ds=0 ici -> accès correct)
    shr ebx, 4              ; convertit en segment (adresse / 16)

    xor cx, cx
    mov ds, cx              ; ds=0 pour accéder au DAP

    mov dword [dap + 0x08], eax     ; LBA du premier secteur du cluster
    mov dword [dap + 0x0C], 0
    mov word  [dap + 0x02], 8       ; 8 secteurs = 1 cluster = 4096 octets
    mov word  [dap + 0x04], 0x0000  ; offset = 0
    mov [dap + 0x06], bx            ; segment destination calculé

    mov ah, 0x42
    mov dl, [boot_drive]
    mov si, dap
    int 0x13
    jc disk_error

    ; étape 5 : avancer le pointeur de destination
    add dword [load_dest], 0x1000   ; 8 secteurs * 512 = 4096 = 0x1000 octets

    ; étape 6 et 7 : vérifier si c'est le dernier cluster
    pop eax             ; récupère l'entrée FAT sauvegardée
    cmp eax, 0x0ffffff8
    jae done_loading    ; >= 0x0ffffff8 -> dernier cluster, tout le kernel est chargé

    mov ebx, eax        ; sinon : ebx = prochain cluster
    jmp load_next_cluster

done_loading:

    ; =====================================================
    ; BASCULE EN MODE PROTÉGÉ 32 BITS
    ; =====================================================
    ; TOUTES les lectures disque sont finies ici
    ; int 0x13 ne fonctionnerait plus après cette bascule

    cli                 ; désactive les interruptions avant la bascule (obligatoire)

    xor ax, ax
    mov ds, ax          ; ds=0 AVANT lgdt, sinon lgdt lit gdtd au mauvais endroit

    lgdt [gdtd]         ; charge la GDT dans le registre GDTR du CPU

    mov eax, cr0
    or eax, 1           ; bit PE (Protection Enable) = 1 -> active le mode protégé
    mov cr0, eax

    jmp 0x08:protected_mode ; far jump qui flush le pipeline et recharge cs avec le sélecteur 0x08

disk_error:
    cli
    hlt

; =====================================================
; DONNÉES 16 BITS
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
current_entry dw 0              ; sauvegarde le début de l'entrée courante du répertoire
load_dest dd 0x20000            ; adresse physique courante de chargement du kernel (avance cluster par cluster)
boot_drive db 0                 ; numéro de disque transmis par le BIOS, sauvegardé en tout premier

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
; structure d'un ELF32 header (les offsets qui nous intéressent) :
;   0x00 : magic  (4 octets : 0x7f 'E' 'L' 'F')
;   0x18 : e_entry  (4 octets : adresse d'entrée du kernel)
;   0x1c : e_phoff  (4 octets : offset de la table des program headers)
;   0x2c : e_phnum  (2 octets : nombre de programme headers)
;
; structure d'un program header ELF32 (32 octets) :
;   0x00 : p_type   (4 octets : type du segment, PT_LOAD = 1)
;   0x04 : p_offset (4 octets : offset du segment dans le fichier ELF)
;   0x08 : p_vaddr  (4 octets : adresse virtuelle de destination)
;   0x0c : p_paddr  (4 octets : adresse physique, ignorée ici)
;   0x10 : p_filesz (4 octets : taille du segment dans le fichier)
;   0x14 : p_memsz  (4 octets : taille en mémoire, peut être > filesz pour le BSS)
;
; algo :
; 1. esi = 0x20000  (début du fichier ELF en mémoire)
; 2. lire e_entry  à [esi + 0x18] -> sauvegarder dans [entry_point]
; 3. lire e_phoff  à [esi + 0x1c] -> ebp = esi + e_phoff
; 4. lire e_phnum  à [esi + 0x2c] -> compteur dans ecx
; 5. boucle pour chaque segment :
;    lire p_type   à [ebp + 0x00]
;    si p_type != 1 (PT_LOAD) -> skip
;    lire p_offset à [ebp + 0x04]
;    lire p_vaddr  à [ebp + 0x08]
;    lire p_filesz à [ebp + 0x10]
;    copier p_filesz octets de (0x20000 + p_offset) vers p_vaddr
;    zéroiser (p_memsz - p_filesz) octets après (BSS)
;    ebp += 0x20 (taille d'un program header = 32 octets)
; 6. jmp [entry_point]

    mov esi, 0x20000            ; début du fichier ELF en mémoire

    mov eax, [esi + 0x18]       ; e_entry : adresse d'entrée du kernel
    mov [entry_point], eax      ; sauvegarde dans une variable mémoire
                                  ; on n'utilise plus edi pour ça -> évite les conflits avec rep movsb

    mov eax, [esi + 0x1c]       ; e_phoff : offset de la table des program headers
    add eax, esi                ; adresse absolue du premier program header
    mov ebp, eax                ; ebp = pointeur sur le program header courant

    movzx ecx, word [esi + 0x2c] ; e_phnum : nombre de segments (champ 16 bits -> 32 bits)

parse_ph_loop:
    cmp ecx, 0
    je done_parsing             ; plus de segments -> on saute vers le kernel

    mov eax, [ebp + 0x00]       ; p_type
    cmp eax, 1                  ; PT_LOAD = 1, seul type à copier
    jne skip_segment

    ; segment PT_LOAD : copier de (0x20000 + p_offset) vers p_vaddr
    push ecx                    ; sauvegarde le compteur de boucle (rep movsb écrase ecx)
    push ebp                    ; sauvegarde le pointeur sur le program header courant
    push esi                    ; sauvegarde la base ELF (rep movsb écrase esi)

    mov eax, [ebp + 0x04]       ; p_offset : offset du segment dans le fichier ELF
    add eax, 0x20000            ; adresse source = base ELF + p_offset
    mov esi, eax                ; esi = source pour rep movsb

    mov edi, [ebp + 0x08]       ; p_vaddr : adresse destination pour rep movsb

    mov ecx, [ebp + 0x10]       ; p_filesz : nombre d'octets à copier depuis le fichier
    rep movsb                   ; copie ecx octets de [esi] vers [edi], incrémente esi/edi

    ; zéroiser la partie BSS : (p_memsz - p_filesz) octets après la copie
    ; edi pointe déjà juste après la zone copiée grâce à rep movsb
    ; si p_memsz == p_filesz, ecx sera 0 et rep stosb ne fait rien
    pop esi                     ; restaure base ELF AVANT de lire [ebp + 0x14]
    pop ebp                     ; restaure pointeur program header
    push ebp                    ; sauvegarde à nouveau pour le pop final
    push esi

    mov ecx, [ebp + 0x14]       ; p_memsz
    sub ecx, [ebp + 0x10]       ; p_memsz - p_filesz = taille du BSS
    xor al, al                  ; valeur à écrire = 0
    rep stosb                   ; zéroiser ecx octets à partir de [edi]

    pop esi                     ; restaure base ELF
    pop ebp                     ; restaure pointeur program header
    pop ecx                     ; restaure compteur de boucle

skip_segment:
    add ebp, 0x20               ; program header suivant (taille fixe = 32 octets)
    dec ecx
    jmp parse_ph_loop

done_parsing:
    jmp [entry_point]           ; saute vers e_entry -> le kernel prend la main


; =====================================================
; DONNÉES MODE PROTÉGÉ
; =====================================================
entry_point dd 0                ; adresse d'entrée du kernel, lue depuis l'ELF header