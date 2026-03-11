org 0x7E00
bits 16

; En x86, il y a 2 façons de parler au matériel :
; 1 -> la mémoire RAM (mov)
; 2 -> les ports I/O : bus séparé de la RAM, 65536 ports numérotés de 0x0000 à 0xFFFF, chaque périphérique a ses propres ports
; On choisit le registre al car le port 0x92 est un port 8 bits (al = moitié basse de ax, 8 bits)
; Activation de la ligne A20 via le port 0x92 
; Sans A20 le bit 21 des adresses est bloqué à 0, impossible d'accéder au delà de 1Mo de mémoire

    mov ax, 0xB800
    mov es, ax
    mov word [es:0], 0x0741
    jmp $

    in al, 0x92         ; Lit l'octet actuel du port 0x92 (System Control Port A)
    or al, 0b00000010   ; Met le bit 1 à 1 pour activer A20
    and al, 0b11111110  ; Met le bit 0 à 0 pour éviter le reset machine
    out 0x92, al        ; Renvoie l'octet modifié au port 0x92

    ; On teste si A20 est bien activé via un test mémoire
    ; Sans A20 : 0x100000 et 0x000000 pointent au même endroit physique (bit 21 ignoré)
    ; Avec A20 : 0x100000 et 0x000000 sont deux adresses séparées
    ; En mode réel on ne peut pas adresser 0x100000 directement, on utilise fs:0x0010 avec fs=0xFFFF
    ; (0xFFFF << 4) + 0x0010 = 0xFFFF0 + 0x10 = 0x100000

    xor di, di                  ; di car c'est un segment et c'est le plus neutre des registres
    mov byte [di], 0x55         ; Écrit 0x55 à l'adresse 0x0000 (ds=0 donc ds:di = 0x00000)
    mov ax, 0xFFFF
    mov fs, ax
    mov al, [fs:0x0010]         ; Lit ce qui est à 0x100000 via fs:0x0010
    cmp al, 0x55                ; Si l'adresse est pareil -> A20 inactif, sinon -> A20 actif
    jne A20_OK                  ; A20 actif, on continue

    mov ax, 0x2401      ; Fonction pour activer le port A20 via le BIOS 
    int 0x15            ; Interruption pour activer A20 via le BIOS (alternative au port 0x92)
    jnc A20_OK          ; Si pas d'erreur, A20 est activé, sinon échec (c'est la différence avec jne la)

A20_FAIL:
    cli
    hlt

A20_OK:
; A20 est maintenant activé, on peut accéder à toute la mémoire au-delà de 1Mo

; FAT_start = ReservedSector = 32

; (LBA c'est juste le numéro de secteur) = 1056
; cylindre = LBA / (secteurs_par_piste * têtes)
; tête     = (LBA / secteurs_par_piste) % têtes
; secteur  = (LBA % secteurs_par_piste) + 1

; valeurs du disque
; LBA = 1056                ; secteur du répertoire racine
; secteurs_par_piste = 63
; têtes = 255

; calcul secteurs par cylindre
; secteurs_par_cylindre = secteurs_par_piste * têtes
; secteurs_par_cylindre = 63 * 255
; secteurs_par_cylindre = 16065

; calcul cylindre
; cylindre = LBA / secteurs_par_cylindre
; cylindre = 1056 / 16065
; cylindre = 0

; calcul tête
; tête = (LBA / secteurs_par_piste) % têtes
; tête = (1056 / 63) % 255
; tête = 16 % 255
; tête = 16

; calcul secteur
; secteur = (LBA % secteurs_par_piste) + 1
; secteur = (1056 % 63) + 1
; secteur = 48 + 1
; secteur = 49

; résultat CHS final
; cylindre = 0
; tête = 16
; secteur = 49

    mov ah, 0x02        ; Fonction 0x02 : Lire des secteurs
    mov al, 1           ; Lire 1 secteur
    mov ch, 0           ; CH = cylindre 0 
    mov cl, 49          ; Secteur 49
    mov dh, 16          ; Tête 16
    mov dl, 0x80        ; Premier disque dur (0)
    mov ax, 0x0900      
    mov es, ax          ; es:bx = adresse de destination pour le secteur lu (0x9000:0x0000 = 0x90000)
    xor bx, bx          ; Offset à 0
    int 0x13            ; Lit le secteur 49 du disque dur

    mov ax, 0x9000
    mov ds, ax
    mov si, 0x0000      ; [MODIF] opérande manquante — si pointe sur la première entrée du répertoire racine

check_entry:            ; début de la boucle - APRES la lecture disque, on ne relit pas à chaque tour
    mov al, [si]    
    cmp al, 0x00        ; Vérifie que le premier octet du secteur n'est pas 0 (indique un secteur vide)
    je dir_end          ; fin du répertoire, le kernel n'existe pas sur le disque
    cmp al, 0xE5        ; Vérifie que le premier octet du secteur n'est pas 0xE5 (l'entrée est inutilisé)
    je next_entry
    mov al, [si + 11]   ; offset 11 = octet d'attributs de l'entrée (pas 10 ; offset 10 = dernier octet du nom)
    cmp al, 0x0F        ; 0x0F = attribut LFN (Long File Name), entrée à ignorer car pas au format 8.3
    je next_entry
    mov di, kernel_name ; Adresse du nom de fichier attendu
    mov [current_entry], si ; Sauvegarde le début de l'entrée actuelle pour pouvoir y revenir plus tard
    push si             ; sauvegarde si sur la pile - read_name va l'incrémenter, il faut pouvoir revenir
    mov cx, 11          ; KERNEL  BIN

read_name: 
    mov al, [si]        ; lire un octet du secteur
    mov bl, [di]        ; lire un octet du nom attendu
    cmp al, bl
    jne name_mismatch   ; les octets diffèrent : ce n'est pas le bon fichier
    inc si              ; Passer à l'octet suivant du secteur
    inc di              ; Passer à l'octet suivant du nom attendu
    loop read_name      ; Répéter pour les 11 octets du nom
    pop si              ; les 11 octets correspondent : on restaure si (current_entry suffit mais on dépile proprement)
    jmp kernel_found    ; Si le nom correspond, on a trouvé le kernel

name_mismatch:
    pop si              ; restaure si avant de passer à l'entrée suivante (dépile ce qu'on a empilé avant read_name)

next_entry: 
    add si, 32          ; 32 car chaque entrée de répertoire fait 32 octets, on passe à la prochaine entrée
    jmp check_entry     ; retour au début de la boucle - SURTOUT PAS read_cluster_loop qui remettrait si à 0x9000

dir_end:                ; on a parcouru tout le répertoire sans trouver KERNEL  BIN
    cli
    hlt                 ; arrêt machine - le kernel est introuvable, rien à faire de plus

kernel_found:
    mov si, [current_entry] ; Revenir au début de l'entrée du kernel trouvée
    xor ebx, ebx
    mov bx, [si + 0x14]     ; cluster haut (16 bits)
    shl ebx, 16             ; décale vers le haut - Foutu spec de FAT que j'avais pas vu et qui m'a bloqué un peu mais j'ai trouvé ça
    mov bx, [si + 0x1A]     ; cluster bas (16 bits)
    ; ebx contient maintenant le numéro de cluster complet
    ; POV J'UTILISE ENFIN UN REGISTRE 32 BITS POUR LA PREMIERE FOIS WOOHOO

    ; NOTE: CHS hardcodé (63 secteurs/piste, 255 têtes) - fonctionne sur QEMU mais peut etre pas sur vrai hardware
    ; TODO: passer à LBA étendu (int 0x13 / 0x42) avant les tests hardware réels (phase 13)

    mov eax, ebx         ; Mettre le numéro de cluster ebx dans eax
    shl eax, 2           ; Multiplie par 4 car chaque entrée FAT32 fait 4 octets
    xor edx, edx         
    mov ecx, 512         ; diviseur : 512 octets par secteur
    div ecx              ; eax = secteur FAT, edx = offset dans ce secteur
    add eax, 32          ; On ajoute FAT_start à eax pour avoir le LBA réel

    push edx             ; sauvegarde l'offset avant les divisions CHS qui vont écraser edx

    ; Conversion LBA -> CHS
    ; C = LBA / (heads * sectors_per_track)
    ; H = (LBA / sectors_per_track) % heads
    ; S = (LBA % sectors_per_track) + 1

    mov ecx, 63          ; sectors per track
    xor edx, edx
    div ecx

    mov bl, dl            ; reste = secteur -1
    inc bl                ; secteur

    mov ecx, 255
    xor edx, edx
    div ecx               ; eax = cylindre, edx = head

    mov ch, al            ; cylindre
    mov dh, dl            ; tete
    mov cl, bl            ; secteur

    ; Lire le secteur FAT
    mov ah, 0x02
    mov al, 1
    mov dl, 0x80
    push ax               ; sauvegarder ah/al avant que mov ax,0x8000 les écrase (le bug qui ma pris du temp mdr)
    mov ax, 0x8000
    mov es, ax            ; Buffer = 0x8000:0x0000 = 0x80000
    pop ax                ; [remettre ah=0x02, al=1
    xor bx, bx
    int 0x13
    jc disk_error

    pop esi               ; Récupère l'offset sauvegardé avant les divisions CHS

follow_chain:
    mov eax, [es:esi]       
    ; 0FFFFFFF car il garde les 28 bits et supprime les 4 bits hauts
    ; and pour masquer
    and eax, 0x0FFFFFFF     ; Lire 4 octets (entrée FAT car les 4 bits de haut sont réservés au FAT system)
    cmp eax, 0x0FFFFFF8
    jae load_clusters       ; Si c'est le dernier cluster : on le charge
    mov ebx, eax            ; sinon prochain cluster
    jmp kernel_found        ; recalculer le CHS pour ce cluster et relire la FAT

load_clusters:
    ; ebx = premier cluster du kernel
    ; LBA = 32 + 2*8 + (cluster - 2) * 8   (sectors_per_cluster = 8 sur QEMU par défaut)
    ; charger à 0x2000:0x0000 = 0210000
    mov eax, ebx
    sub eax, 2
    mov ecx, 8              ; sectors_per_cluster 
    mul ecx               
    add eax, 1056           ; data_start = 32 + 2*512

    ; Conversion LBA -> CHS avant int 0x13 
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

    ; [Lire 8 secteurs (1 cluster) vers 0x1000:0x0000 = 0x10000
    mov ah, 0x02
    mov al, 8
    mov dl, 0x80
    push ax
    mov ax, 0x1000
    mov es, ax
    pop ax
    xor bx, bx
    int 0x13
    jc disk_error

    jmp 0x2000:0x0000       ; jump vers le kernel chargé à 0x20000

disk_error: 
    cli 
    hlt


; Le format de fat 32 est de 11 octets : le nom du fichier et l'extenssion donc on dois les remplir
kernel_name db "KERNEL  BIN"
current_entry dw 0              ; Pour Sauvegarder le début de l'entrée