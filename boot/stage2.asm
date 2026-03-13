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
    sti
; a20 est maintenant activé, on peut accéder à toute la mémoire au-delà de 1mo

; fat_start = reservedsector = 32

; (lba c'est juste le numéro de secteur) = 1056
; cylindre = lba / (secteurs_par_piste * têtes)
; tête     = (lba / secteurs_par_piste) % têtes
; secteur  = (lba % secteurs_par_piste) + 1

; valeurs du disque
; lba = 1056                ; secteur du répertoire racine
; secteurs_par_piste = 63
; têtes = 255

; calcul secteurs par cylindre
; secteurs_par_cylindre = secteurs_par_piste * têtes
; secteurs_par_cylindre = 63 * 255
; secteurs_par_cylindre = 16065

; calcul cylindre
; cylindre = lba / secteurs_par_cylindre
; cylindre = 1056 / 16065
; cylindre = 0

; calcul tête
; tête = (lba / secteurs_par_piste) % têtes
; tête = (1056 / 63) % 255
; tête = 16 % 255
; tête = 16

; calcul secteur
; secteur = (lba % secteurs_par_piste) + 1
; secteur = (1056 % 63) + 1
; secteur = 48 + 1
; secteur = 49

; résultat chs final
; cylindre = 0
; tête = 16
; secteur = 49

    ; ds est à 0 ici, on peut lire directement vers 0x9000
    mov ah, 0x02        ; fonction 0x02 : lire des secteurs - EN PREMIER avant tout mov ax
    mov al, 1           ; lire 1 secteur
    mov ch, 0           ; ch = cylindre 0 
    mov cl, 49          ; secteur 49
    mov dh, 16          ; tête 16
    mov dl, 0x80        ; premier disque dur
    mov bx, 0x0900
    mov es, bx          ; es:bx = 0x9000:0x0000 = adresse physique 0x90000
    xor bx, bx          ; offset à 0
    int 0x13            ; lit le secteur du répertoire racine
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
    cmp al, 0xe5        ; vérifie que le premier octet du secteur n'est pas 0xe5 (l'entrée est inutilisé)
    je next_entry
    mov al, [si + 11]   ; offset 11 = octet d'attributs de l'entrée (pas 10 ; offset 10 = dernier octet du nom)
    cmp al, 0x0f        ; 0x0f = attribut lfn (long file name), entrée à ignorer car pas au format 8.3
    je next_entry
    mov di, kernel_name ; adresse du nom de fichier attendu
    mov [current_entry], si ; sauvegarde le début de l'entrée actuelle pour pouvoir y revenir plus tard
    push si             ; sauvegarde si sur la pile - read_name va l'incrémenter, il faut pouvoir revenir
    mov cx, 11          ; kernel  bin

read_name: 
    mov al, [si]        ; lire un octet du secteur
    mov bl, [cs:di]        ; llire un octet du nom attendu (forcer cs car kernel_name est dans stage2)
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
    jmp check_entry     ; retour au début de la boucle - surtout pas read_cluster_loop qui remettrait si à 0x9000

dir_end:                ; on a parcouru tout le répertoire sans trouver kernel  bin
    cli
    hlt                 ; arrêt machine - le kernel est introuvable, rien à faire de plus

kernel_found:
    mov si, [current_entry] ; revenir au début de l'entrée du kernel trouvée
    xor ebx, ebx
    mov bx, [si + 0x14]     ; cluster haut (16 bits)
    shl ebx, 16             ; décale vers le haut - foutu spec de fat que j'avais pas vu et qui m'a bloqué un peu mais j'ai trouvé ça
    mov bx, [si + 0x1a]     ; cluster bas (16 bits)
    ; ebx contient maintenant le numéro de cluster complet
    ; pov j'utilise enfin un registre 32 bits pour la premiere fois woohoo

    ; note: chs hardcodé (63 secteurs/piste, 255 têtes) - fonctionne sur qemu mais peut etre pas sur vrai hardware
    ; todo: passer à lba étendu (int 0x13 / 0x42) avant les tests hardware réels (phase 13)

    mov eax, ebx         ; mettre le numéro de cluster ebx dans eax
    shl eax, 2           ; multiplie par 4 car chaque entrée fat32 fait 4 octets
    xor edx, edx         
    mov ecx, 512         ; diviseur : 512 octets par secteur
    div ecx              ; eax = secteur fat, edx = offset dans ce secteur
    add eax, 32          ; on ajoute fat_start à eax pour avoir le lba réel

    push edx             ; sauvegarde l'offset avant les divisions chs qui vont écraser edx

    ; conversion lba -> chs
    ; c = lba / (heads * sectors_per_track)
    ; h = (lba / sectors_per_track) % heads
    ; s = (lba % sectors_per_track) + 1

    mov ecx, 63          ; sectors per track
    xor edx, edx
    div ecx

    mov bl, dl            ; reste = secteur - 1
    inc bl                ; secteur

    mov ecx, 255
    xor edx, edx
    div ecx               ; eax = cylindre, edx = head

    mov ch, al            ; cylindre
    mov dh, dl            ; tete
    mov cl, bl            ; secteur

    ; lire le secteur fat
    ; IMPORTANT : ah/al EN PREMIER avant tout mov ax qui écraserait ah
    mov ah, 0x02
    mov al, 1
    mov dl, 0x80
    push ax               ; sauvegarder ah/al avant que mov bx,0x8000 les écrase (le bug qui ma pris du temp mdr)
    mov bx, 0x8000
    mov es, bx            ; buffer = 0x8000:0x0000 = 0x80000
    pop ax                ; remettre ah=0x02, al=1
    xor bx, bx
    int 0x13
    jc disk_error

    pop esi               ; récupère l'offset sauvegardé avant les divisions chs

follow_chain:
    mov eax, [es:esi]       
    ; 0fffffff car il garde les 28 bits et supprime les 4 bits hauts
    ; and pour masquer
    and eax, 0x0fffffff     ; lire 4 octets (entrée fat car les 4 bits de haut sont réservés au fat system)
    cmp eax, 0x0ffffff8
    jae load_clusters       ; si c'est le dernier cluster : on le charge
    mov ebx, eax            ; sinon prochain cluster
    jmp kernel_found        ; recalculer le chs pour ce cluster et relire la fat

load_clusters:
    ; ebx = premier cluster du kernel
    ; lba = 32 + 2*8 + (cluster - 2) * 8   (sectors_per_cluster = 8 sur qemu par défaut)
    ; charger à 0x2000:0x0000 = 0x20000
    mov eax, ebx
    sub eax, 2
    mov ecx, 8              ; sectors_per_cluster 
    mul ecx               
    add eax, 1056           ; data_start = 32 + 2*512

    ; conversion lba -> chs avant int 0x13 
    mov ecx, 63
    xor edx, edx
    div ecx
    mov bl, dl
    inc bl
    mov ecx, 255
    xor edx, edx
    div ecx
    mov ch, al
    mov dh, dl
    mov cl, bl

    ; lire 8 secteurs (1 cluster) vers 0x2000:0x0000 = 0x20000
    ; IMPORTANT : ah/al EN PREMIER avant tout mov qui écraserait ah
    mov ah, 0x02
    mov al, 8
    mov dl, 0x80
    push ax
    mov bx, 0x2000
    mov es, bx              ; es:bx = 0x2000:0x0000 = 0x20000
    pop ax
    xor bx, bx
    int 0x13
    jc disk_error

    ; on remet ds à 0 AVANT lgdt - sinon lgdt lit gdtd au mauvais endroit (ds=0x9000 + offset = adresse fausse)
    xor ax, ax
    mov ds, ax

    ; maintenant on bascule en mode protégé avant de parser l'elf
    lgdt [gdtd]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:protected_mode     ; far jump qui flush le pipeline et active le mode protégé

disk_error: 
    cli 
    hlt


; le format de fat 32 est de 11 octets : le nom du fichier et l'extenssion donc on dois les remplir
kernel_name db "kernel  bin"
current_entry dw 0              ; pour sauvegarder le début de l'entrée

; on fait la gdt
; le cpu lit une entrée dans la gdt qui contient :
; base address
; limit
; type (code / data)
; privilege level
; flags
;
; donc la gdt est indispensable

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
; 16 bits : taille
; 32 bits : adresse
gdtd:
    dw gdt_end - gdt_start - 1
    dd gdt_start

; 10011010b     le code créer se segment
; 1    -> présent
; 00   -> ring 0
; 1    -> code/data (descriptor type)
; 1    -> executable (code)
; 0    -> non-conforming
; 1    -> readable
; 0    -> accessed (mis à 1 par le cpu automatiquement)

[bits 32]
; on est enfin en 32 bits wouh !

protected_mode:

; recharger les segments avec le sélecteur data ring 0 (0x10)
    mov ax, 0x10    ; entrée data ring 0
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ; vaut mieux pas faire d'accès mémoire selon intel donc azy

    mov esp, 0x90000    ; sinon en mode protégé la pile a une valeure aléatoire du mode réel


; maintenant faut parser le header du kernel.bin qui est en elf alors que l'extention .bin m'a induit en erreur et j'ai passer 6h de debug pour ca rraahhh
; le header nous dit ou se situ le code

; algo : 
; 1. esi = 0x20000  (début du fichier elf en mémoire)
; 2. lire e_entry  à [esi + 0x18]  -> sauvegarder dans edi
; 3. lire e_phoff  à [esi + 0x1c]  -> ebp = esi + e_phoff (pointeur courant dans la table des program headers)
; 4. lire e_phnum  à [esi + 0x2c]  -> compteur de boucle dans ecx

;  5. boucle pour chaque segment :
;     lire p_type   à [ebp + 0x00]
;      si p_type != 1 -> segment suivant (skip)
;      
;      lire p_offset à [ebp + 0x04]
;      lire p_vaddr  à [ebp + 0x08]
;      lire p_filesz à [ebp + 0x10]
;      
;      copier p_filesz octets de (0x20000 + p_offset) vers p_vaddr
;      attention : rep movsb écrase ecx, il faut le sauvegarder/restaurer autour
;      
;      ebp += 32  (segment suivant, chaque program header fait 0x20 octets)
;      boucle

; 6. sauter sur e_entry (dans edi)

    mov esi, 0x20000            ; début du fichier ELF en mémoire

    ; vérification magic number ELF (optionnel mais utile pour débugger)
    ; [esi+0] = 0x7f 'E' 'L' 'F'

    mov eax, [esi + 0x18]       ; e_entry : adresse d'entrée du kernel
    mov edi, eax                ; on sauvegarde e_entry dans edi (on l'utilisera pour le jump final)

    mov eax, [esi + 0x1c]       ; e_phoff : offset de la table des program headers
    add eax, esi                ; ebp = 0x20000 + e_phoff (adresse absolue du premier program header)
    mov ebp, eax                ; on garde le pointeur courant dans ebp

    movzx ecx, word [esi + 0x2c] ; e_phnum : nombre de segments (movzx car champ 16 bits -> 32 bits)

parse_ph_loop:
    cmp ecx, 0
    je done_parsing             ; plus de segments à traiter, on saute vers le kernel

    mov eax, [ebp + 0x00]       ; p_type
    cmp eax, 1                  ; PT_LOAD = 1, seul type qui nous intéresse (les autres sont des métadonnées)
    jne skip_segment            ; si ce n'est pas PT_LOAD, on skip

    ; c'est un segment PT_LOAD : il faut le copier de (0x20000 + p_offset) vers p_vaddr
    mov eax, [ebp + 0x04]       ; p_offset : offset du segment dans le fichier ELF
    add eax, 0x20000            ; adresse source = base ELF en mémoire + p_offset
    push esi                    ; sauvegarde esi (base ELF) car on va le réutiliser pour rep movsb
    mov esi, eax                ; esi = adresse source pour la copie

    push edi                    ; sauvegarde e_entry (edi) car on en a besoin pour le jump final
    mov edi, [ebp + 0x08]       ; edi = p_vaddr (adresse destination en mémoire)

    push ecx                    ; IMPORTANT : rep movsb écrase ecx, on sauvegarde le compteur de segments
    mov ecx, [ebp + 0x10]       ; p_filesz : nombre d'octets à copier
    rep movsb                   ; copie ecx octets de [esi] vers [edi] (direction = forward, df=0)
    pop ecx                     ; restaure le compteur de segments

    pop edi                     ; restaure e_entry dans edi
    pop esi                     ; restaure esi (base ELF)

skip_segment:
    add ebp, 0x20               ; chaque program header fait 32 octets (0x20), on passe au suivant
    dec ecx
    jmp parse_ph_loop

done_parsing:
    jmp edi                     ; saute vers e_entry -> point d'entrée du kernel, bonne chance à lui