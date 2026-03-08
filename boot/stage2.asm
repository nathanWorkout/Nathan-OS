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

read_cluster_loop: 
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
    mov ax, 0x0900      ; 
    mov es, ax          ; es:bx = adresse de destination pour le secteur lu (0x9000:0x0000 = 0x90000)
    xor bx, bx          ; Offset à 0
    int 0x13            ; Lit le secteur 49 du disque dur
    
    mov si, 0x9000      ; Adresse du secteur lu (0x9000)
    mov al, [si]    
    cmp al, 0x00        ; Vérifie que le premier octet du secteur n'est pas 0 (indique un secteur vide)
    je next_entry       ; Si le secteur est vide, continuer la recherche
    cmp al, 0xE5        ; Vérifie que le premier octet du secteur n'est pas 0xE5 (l'entrée est inutilisé)
    je next_entry   ; Saute si = 0
    mov al, [si + 10]   ; 10 car le 10eme offset correspoond au 10eme octet (0 offset = 1er octet)
    cmp al, 0x0F        ; Vérifie que l'entré de nom du fichier n'est pas trop longue
    je next_entry       ; Pareil
    mov di, kernel_name ; Adresse du nom de fichier attendu
    mov [current_entry], si ; Sauvegarde le début de l'entrée actuelle pour pouvoir y revenir plus tard
    mov cx, 11          ; KERNEL  BIN

read_name: 
    mov al, [si]        ; lire un octet du secteur
    mov bl, [di]        ; lire un octet du nom attendu
    cmp al, bl
    jne next_entry      ; Passer à l'entrée suivante du répertoire racine
    inc si              ; Passer à l'octet suivant du secteur
    inc di              ; Passer à l'octet suivant du nom attendu
    loop read_name      ; Répéter pour les 11 octets du nom
    jmp kernel_found      ; Si le nom correspond, on a trouvé le kernel

next_entry: 
    add si, 32          ; 32 car chaque entrée de répertoire fait 32 octets, on passe à la prochaine entrée
    jmp read_cluster_loop ; Recommencer la boucle pour lire la prochaine entrée du répertoire racine

kernel_found:
    sub si, 11
    mov si, [current_entry] ; Revenir au début de l'entrée du kernel trouvée
    mov bx, [si + 0x14]  ; cluster haut (16 bits)
    shl ebx, 16          ; décale vers le haut          ; Foutu spec de FAT que j'avais pas vu et qui m'a bloqué un peu mais j'ai trouvé ça
    mov bx, [si + 0x1A]  ; cluster bas (16 bits)
    ; ebx contient maintenant le numéro de cluster complet
    ; POV J'UTILISE ENFIN UN REGISTRE 32 BITS POUR LA PREMIERE FOIS WOOHOO 

    ; Maintenant faut calculer le secteur FAT qui corespond au secteur lu dans ebx
    ; secteur FAT = FAT_start + (cluster * 4) / 512
    ; offset      = (cluster * 4) % 512
    



; Le format de fat 32 est de 11 octets : le nom du fichier et l'extenssion donc on dois les remplir
kernel_name db "KERNEL  BIN"
current_entry dw 0              ; Pour Sauvegarder le début de l'entrée



