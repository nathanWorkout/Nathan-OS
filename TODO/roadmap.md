Vault-OS — Roadmap complète

  ══════════════════════════════════════════════════════════════════
  ÉTAT ACTUEL — CE QUI EST DÉJÀ FAIT
  ══════════════════════════════════════════════════════════════════

  Bootloader:
    ✔ Stage 1 — MBR 512 octets, signature 0xAA55, lecture stage2 via int 0x13 CHS
    ✔ Stage 2 — activation A20 (port 0x92 + fallback int 0x15), vérification LBA étendu
    ✔ Stage 2 — lecture répertoire racine FAT32 via int 0x13 LBA étendu (DAP)
    ✔ Stage 2 — recherche "KERNEL  BIN" format 8.3, suivi chaîne de clusters FAT32
    ✔ Stage 2 — chargement kernel cluster par cluster vers 0x20000
    ✔ Stage 2 — memory map E820 stockée à 0x6000 avec compteur à 0x5FFE
    ✔ Stage 2 — GDT 5 entrées (null, code r0, data r0, code r3, data r3), passage mode protégé
    ✔ Stage 2 — parse ELF32 : program headers PT_LOAD, copie segments, zéro BSS, saut e_entry
    ✔ Entry point assembleur : initialisation pile kernel, appel kmain

  Kernel — base:
    ✔ GDT kernel (3 entrées : null, code r0, data r0) rechargée depuis C
    ✔ IDT 256 entrées, chargement via lidt
    ✔ ISR stubs assembleur (isr0-isr31) avec gestion code d'erreur CPU
    ✔ IRQ0 (timer), IRQ1 (clavier) en assembleur avec pushad/iret
    ✔ ISR handler C : kernel panic visuel (lune + loup ASCII art, messages couleur)
    ✔ PIC 8259 : initialisation, remapping IRQ 0-7 → 32-39, masquage, EOI
    ✔ PIT : configuration fréquence (1000 Hz actuellement), canal 0 mode 3
    ✔ Serial COM1 : init 38400 baud 8N1, putchar, print, println, print_hex
    ✔ TTY VGA 80x25 : putchar, puts, printk (%d %s %c), scroll, curseur hardware
    ✔ PMM bitmap : init depuis E820, alloc_page, free_page, réservation kernel + zone basse
    ✔ Paging basique : 1 page directory globale, identity map 4 premiers Mo, activation cr0
    ✔ Clavier PS/2 AZERTY : table scancodes, shift, ring buffer input
    ✔ Shell texte : readline, commandes help/clear/say/reboot


  ══════════════════════════════════════════════════════════════════
  PHASE 0 — CORRECTIONS SUR LA BASE EXISTANTE
  ══════════════════════════════════════════════════════════════════
  NOTE: Ces corrections sont des prérequis bloquants.
  Rien de ce qui suit ne peut être construit solidement sans elles.
  Ne pas passer à la phase 1 tant que les 3 points ne sont pas validés.

  0.1 — Restructurer l'interrupt frame (isr.asm + isr.c):
    ☐ Définir interrupt_frame_t packed avec tous les registres dans l'ordre de la pile
        ☐ edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax  (du pushad)
        ☐ num, error_code                                 (tes 2 push manuels)
        ☐ eip, cs, eflags                                 (poussés par le CPU)
        ☐ user_esp, user_ss                               (seulement si depuis ring 3)
    ☐ Modifier isr_common dans isr.asm
        ☐ Après pushad, mettre esp dans eax (pointe sur le début de la frame)
        ☐ Push eax comme unique argument → call isr_handler(interrupt_frame_t*)
        ☐ Supprimer les 3 push manuels (num, error_code, eip) qui existaient avant
        ☐ add esp, 4 après le call pour nettoyer l'argument
    ☐ Modifier isr_handler en C pour prendre interrupt_frame_t* au lieu de 3 uint32_t
    ☐ Vérifier que frame->num donne le bon numéro d'exception
    ☐ Vérifier que frame->eip pointe vers l'instruction fautive via GDB
    ☐ Objectif : div par zéro → frame->num == 0, frame->eip == adresse du div

  0.2 — Étendre la GDT (gdt.c):
    NOTE: Ta GDT actuelle n'a que 3 entrées. Ring 3 et TSS sont impossibles sans ça.
    ☐ Agrandir le tableau gdt[] à 6 entrées
    ☐ Entrée 3 → code ring 3  (sélecteur 0x18, access byte 0xFA)
    ☐ Entrée 4 → data ring 3  (sélecteur 0x20, access byte 0xF2)
    ☐ Entrée 5 → TSS          (sélecteur 0x28, sera remplie par tss_init)
    ☐ Après lgdt, faire un far jump pour recharger cs avec 0x08
    ☐ Recharger ds, es, fs, gs, ss avec 0x10 après le far jump
    ☐ Mettre à jour gdtr.limit avec la nouvelle taille (6 entrées)
    ☐ Vérifier via GDB : cs == 0x08, ds == 0x10 après gdt_init
    ☐ Objectif : kernel démarre, aucune GPF, tous les segments corrects

  0.3 — Vérifier la cohérence du Makefile:
    ☐ S'assurer que les nouveaux dossiers (proc/, fs/, vault/, elf/, ipc/, gfx/) sont dans les wildcards
    ☐ Ajouter une règle générique pour éviter de dupliquer chaque dossier manuellement
    ☐ Vérifier que make clean supprime bien tous les .o et l'image disque
    ☐ Objectif : make && make img sans erreur après ajout d'un nouveau fichier .c


  ══════════════════════════════════════════════════════════════════
  PHASE 1 — TSS + STRUCTURES PROCESSUS + CONTEXT SWITCH
  ══════════════════════════════════════════════════════════════════
  NOTE: Le TSS est la pièce que le CPU consulte automatiquement lors
  d'une interruption depuis ring 3 pour trouver la pile kernel.
  Sans TSS : le premier int depuis ring 3 = GPF ou pile corrompue.
  Le context switch doit être en assembleur — le compilateur C réorganise
  les registres librement et rendrait le switch non déterministe.

  1.1 — TSS (kernel/proc/tss.h + tss.c):
    ☐ Définir tss_entry_t avec les 104 octets packed (prev_tss, esp0, ss0, ...)
    ☐ Allouer un TSS statique global (une seule instance suffit pour commencer)
    ☐ Initialiser : ss0 = 0x10 (data ring 0), iomap_base = sizeof(tss), tout le reste à 0
    ☐ Remplir l'entrée gdt[5] avec base/limite/access du TSS
        ☐ access byte = 0x89 (présent, ring 0, TSS 32 bits disponible)
        ☐ flags = 0x00 (pas de granularité 4Ko pour le TSS)
    ☐ Charger le registre TR avec ltr 0x28
    ☐ Écrire tss_set_kernel_stack(uint32_t esp0) qui met à jour tss.esp0
    ☐ Appeler tss_init() depuis kmain après gdt_init()
    ☐ Vérifier via GDB : registre TR == 0x28 après ltr
    ☐ Objectif : ltr s'exécute sans GPF, tss.esp0 modifiable dynamiquement

  1.2 — Structure process_t (kernel/proc/process.h):
    ☐ Définir l'enum process_state_t : RUNNING, READY, BLOCKED, ZOMBIE
    ☐ Définir process_t avec les champs dans cet ordre précis (impacte les offsets asm) :
        ☐ uint32_t pid
        ☐ process_state_t state
        ☐ uint32_t *page_directory   (adresse physique du cr3 de ce processus)
        ☐ uint32_t esp               (pile kernel sauvegardée lors du context switch)
        ☐ uint32_t kernel_stack_top  (sommet de la pile kernel allouée)
        ☐ uint32_t eip               (point d'entrée, utilisé au 1er scheduling)
        ☐ uint32_t parent_pid
        ☐ int32_t  exit_code
        ☐ char     name[64]
        ☐ struct process *next       (liste chaînée circulaire pour le scheduler)
    ☐ Calculer et documenter les offsets de chaque champ (utilisés dans switch.asm)
        ☐ Ajouter un static_assert pour vérifier offsetof(process_t, esp) au compile-time
    ☐ Écrire process_create_kernel(name, entry_fn) :
        ☐ Allouer process_t via pmm_alloc_page()
        ☐ Allouer 8Ko pour la pile kernel via pmm_alloc_page() × 2
        ☐ Préparer la pile pour que le premier ret dans switch_context atterrisse sur entry_fn
        ☐ Initialiser state = READY, page_directory = kernel_page_dir global actuel
    ☐ Écrire process_current() qui retourne le processus en cours d'exécution
    ☐ Objectif : process_create_kernel ne crashe pas, les champs sont accessibles

  1.3 — Context switch en assembleur (kernel/proc/switch.asm):
    NOTE: Seuls les registres callee-saved doivent être sauvegardés ici.
    Les caller-saved (eax, ecx, edx) sont à la charge de l'appelant.
    ☐ Écrire switch_context(process_t *old, process_t *new) en global
    ☐ Sauvegarder ebp, ebx, esi, edi sur la pile kernel courante
    ☐ Sauvegarder esp dans old->esp (utiliser l'offset calculé en 1.2)
    ☐ Charger esp depuis new->esp
    ☐ Comparer old->page_directory et new->page_directory
        ☐ Si différents : charger new->page_directory dans cr3 (flush TLB automatique)
        ☐ Si identiques : ne pas toucher cr3 (optimisation — évite le flush TLB inutile)
    ☐ Appeler tss_set_kernel_stack avec new->kernel_stack_top
    ☐ Restaurer edi, esi, ebx, ebp depuis la nouvelle pile
    ☐ ret — retourne dans le contexte du nouveau processus
    ☐ Tester avec 2 kernel threads : A écrit "A" en serial, B écrit "B", vérifier alternance
    ☐ Objectif : switch déterministe, pas de corruption de registres, pas de triple fault

  1.4 — Scheduler round-robin (kernel/proc/scheduler.c):
    ☐ Déclarer run_queue (liste chaînée circulaire de process_t*)
    ☐ Déclarer current_process (process_t* global)
    ☐ Écrire scheduler_add(process_t*) :
        ☐ Si run_queue vide : process->next = process (auto-référence)
        ☐ Sinon : insérer après run_queue, maintenir circularité
    ☐ Écrire scheduler_remove(process_t*) pour les processus qui se terminent
    ☐ Écrire schedule() :
        ☐ Si run_queue vide ou un seul processus : retourner sans changer
        ☐ Parcourir la liste à partir de current->next
        ☐ Trouver le premier processus avec state == READY
        ☐ Mettre old->state = READY, new->state = RUNNING
        ☐ Mettre à jour current_process
        ☐ Appeler switch_context(old, new)
    ☐ Créer le processus idle (pid 0) : boucle hlt, ajouté au scheduler au boot
        NOTE: hlt libère le CPU quand rien d'autre ne tourne — économise de l'énergie
    ☐ Modifier irq0_handler pour appeler schedule() à chaque tick timer
    ☐ Objectif : 2 kernel threads stables pendant 10 secondes sans crash


  ══════════════════════════════════════════════════════════════════
  PHASE 2 — VMM (Virtual Memory Manager)
  ══════════════════════════════════════════════════════════════════
  NOTE: C'est le composant le plus critique de Vault-OS.
  L'isolation des profils, l'immuabilité du Core et le COW
  reposent entièrement dessus. Prendre le temps de bien le faire.
  Le fichier pagging.c actuel est à remplacer, pas à patcher.

  2.1 — Refonte du paging (kernel/memory/vmm.c):
    NOTE SUR LES FLAGS x86 :
    Bit 0 (Present)   : la page est en mémoire physique
    Bit 1 (R/W)       : 0 = lecture seule, 1 = lecture/écriture
    Bit 2 (U/S)       : 0 = superviseur uniquement, 1 = accessible ring 3
    Bits 9-11         : disponibles pour l'OS — on utilisera le bit 9 pour COW
    Combinaisons Vault-OS :
      Core immuable   : Present=1, R/W=0, U/S=0  (ni ring 3 ni écriture)
      Données kernel  : Present=1, R/W=1, U/S=0  (kernel seul, modifiable)
      Page COW profil : Present=1, R/W=0, U/S=1, COW=1  (ring 3 lecture, fault sur écriture)
      Page privée profil : Present=1, R/W=1, U/S=1  (copie COW, ring 3 peut écrire)
    ☐ Définir les constantes PAGE_PRESENT, PAGE_RW, PAGE_USER, PAGE_COW
    ☐ Définir address_space_t {uint32_t *page_directory, uint32_t pid}
    ☐ Écrire vmm_create_space() :
        ☐ Allouer 1 page (4Ko) via pmm_alloc_page() pour le page directory
        ☐ Zéroïser les 1024 entrées
        ☐ Copier les mappings kernel (entries 768-1023 si higher half, ou 0-3 si identity)
        ☐ Retourner l'address_space_t initialisé
    ☐ Écrire vmm_map(space, virt, phys, flags) :
        ☐ Calculer l'index dans le page directory (virt >> 22)
        ☐ Si la page table n'existe pas : allouer via pmm_alloc_page(), zéroïser
        ☐ Calculer l'index dans la page table ((virt >> 12) & 0x3FF)
        ☐ Écrire l'entrée : phys | flags
    ☐ Écrire vmm_unmap(space, virt) :
        ☐ Mettre l'entrée de page table à 0
        ☐ Invalider le TLB : asm volatile("invlpg (%0)" :: "r"(virt) : "memory")
    ☐ Écrire vmm_get_phys(space, virt) → adresse physique mappée
    ☐ Écrire vmm_get_flags(space, virt) → flags de l'entrée (bits 0-11)
    ☐ Écrire vmm_switch_space(space) → charger space->page_directory dans cr3
    ☐ Écrire vmm_destroy_space(space) :
        ☐ Libérer toutes les frames des pages user (ne pas libérer les pages kernel partagées)
        ☐ Libérer les page tables elles-mêmes
        ☐ Libérer le page directory
    ☐ Objectif : créer 2 espaces, mapper la même frame physique dans les deux, vérifier indépendance

  2.2 — Page fault handler COW (ISR 14):
    NOTE: C'est ici que le Core devient vraiment immuable.
    Quand un profil essaie d'écrire sur une page COW :
    1. CPU déclenche une page fault (ISR 14)
    2. Le kernel alloue une nouvelle frame physique
    3. Copie le contenu de l'ancienne frame dans la nouvelle
    4. Remap la page du profil vers la nouvelle frame avec R/W=1
    5. Invalide le TLB pour cette adresse
    6. Retourne — le CPU réessaie l'instruction, qui réussit cette fois
    ☐ Créer un ISR stub dédié pour l'exception 14 dans isr.asm (isr14)
    ☐ Enregistrer isr14 dans idt_set_entry(14, ...) dans isr_init()
    ☐ Implémenter vmm_page_fault_handler(interrupt_frame_t *frame) :
        ☐ Lire cr2 pour obtenir l'adresse virtuelle fautive
        ☐ Extraire error_code : bit 0 = present, bit 1 = write, bit 2 = user
        ☐ Cas COW : present=1, write=1, user=1, page marquée PAGE_COW
            ☐ pmm_alloc_page() → nouvelle frame physique
            ☐ Copier 4096 octets de l'ancienne frame vers la nouvelle
            ☐ vmm_map() avec (flags & ~PAGE_COW) | PAGE_RW
            ☐ invlpg sur l'adresse fautive
            ☐ Retourner (le CPU réessaie l'instruction)
        ☐ Cas page non présente en ring 3 : tuer le processus (phase 3)
        ☐ Cas page non présente en ring 0 : kernel panic avec cr2 + error_code
        ☐ Cas violation protection en ring 0 : kernel panic (bug kernel)
    ☐ Test COW : 2 espaces mappent la même frame en COW, écriture dans l'un → l'autre intact
    ☐ Objectif : COW transparent, kernel panic sur accès illégal ring 0

  2.3 — Immuabilité du Core:
    NOTE: C'est la garantie centrale de Vault-OS.
    Sans ça, un bug kernel peut corrompre le code kernel lui-même.
    ☐ Écrire vmm_lock_kernel_text() :
        ☐ Parcourir les pages de kernel_start à kernel_end (depuis le linker script)
        ☐ Pour les pages .text : vmm_set_flags(PAGE_PRESENT) sans R/W ni U/S
        ☐ Pour les pages .data/.bss : vmm_set_flags(PAGE_PRESENT | PAGE_RW) sans U/S
        ☐ invlpg sur chaque page modifiée
    ☐ Appeler vmm_lock_kernel_text() en fin de vmm_init(), après activation de la pagination
    ☐ Test : tenter d'écrire sur une adresse .text depuis ring 0 → page fault ISR 14
    ☐ Objectif : toute écriture sur le code kernel = page fault immédiate, même depuis ring 0


  ══════════════════════════════════════════════════════════════════
  PHASE 3 — HEAP KERNEL (kmalloc / kfree)
  ══════════════════════════════════════════════════════════════════
  NOTE: kmalloc est nécessaire pour presque tout ce qui suit.
  VFS, processus, fenêtres GUI — tout alloue dynamiquement.
  À implémenter immédiatement après le VMM.

  3.1 — Allocateur par blocs (kernel/memory/heap.c):
    ☐ Définir HEAP_START = 0x200000 et HEAP_MAX = 0x800000 (6 Mo de heap kernel)
    ☐ Définir heap_block_t {uint32_t magic, uint32_t size, bool free, heap_block_t *next, *prev}
    ☐ Initialiser le heap : mapper la première page, créer un seul bloc libre couvrant tout
    ☐ Écrire kmalloc(size) :
        ☐ Parcourir la liste chaînée, trouver le premier bloc libre assez grand (first-fit)
        ☐ Si le bloc est bien plus grand : le diviser en 2 (bloc alloué + bloc libre restant)
        ☐ Marquer free = false, retourner ptr + sizeof(heap_block_t)
        ☐ Si pas de bloc assez grand : appeler heap_sbrk() pour étendre
    ☐ Écrire kmalloc_aligned(size, align) pour les structures qui nécessitent alignement 4Ko
    ☐ Écrire kfree(ptr) :
        ☐ Retrouver le heap_block_t (ptr - sizeof(heap_block_t))
        ☐ Vérifier le magic (détecter double-free ou corruption)
        ☐ Marquer free = true
        ☐ Coalescence : fusionner avec le bloc suivant si libre
        ☐ Coalescence : fusionner avec le bloc précédent si libre
    ☐ Écrire krealloc(ptr, new_size) : allouer + copier + libérer
    ☐ Écrire heap_sbrk() : étendre via pmm_alloc_page() + vmm_map() sur le heap
    ☐ Tests :
        ☐ 100 alloc de tailles variées (1, 7, 64, 512, 4096 octets) → pas de crash
        ☐ free de chacune dans l'ordre inverse → coalescence correcte
        ☐ Remplir le heap → kmalloc retourne NULL proprement (pas de panic)
    ☐ Objectif : heap stable après 1000 alloc/free de tailles aléatoires


  ══════════════════════════════════════════════════════════════════
  PHASE 4 — USERSPACE RING 3 + SYSCALLS
  ══════════════════════════════════════════════════════════════════
  NOTE: C'est la frontière entre un OS jouet et un OS sérieux.
  En ring 3, une segfault tue le processus — pas le kernel.
  En ring 0, une segfault = kernel panic ou triple fault silencieux.

  4.1 — Loader ELF ring 3 (kernel/elf/elf_loader.c):
    NOTE: La logique existe déjà dans stage2.asm — il faut la porter en C.
    ☐ Vérifier le magic ELF : 0x7F 'E' 'L' 'F' aux 4 premiers octets
    ☐ Vérifier e_type == ET_EXEC (2) et e_machine == EM_386 (3)
    ☐ Lire e_entry (point d'entrée), e_phoff (offset program headers), e_phnum (nombre)
    ☐ Créer un nouvel espace d'adressage via vmm_create_space()
    ☐ Pour chaque program header PT_LOAD :
        ☐ Pour chaque page couverte par p_vaddr..p_vaddr+p_memsz :
            ☐ pmm_alloc_page() → frame physique
            ☐ vmm_map() avec flags selon PF_R/PF_W/PF_X du segment
        ☐ Activer temporairement l'espace d'adressage (vmm_switch_space)
        ☐ memcpy de p_filesz octets depuis elf_data+p_offset vers p_vaddr
        ☐ memset 0 pour p_memsz - p_filesz (zone BSS)
    ☐ Allouer 2 pages pour la pile user (en haut de l'espace user, ex: 0xBFFFF000)
        ☐ vmm_map() avec PAGE_PRESENT | PAGE_RW | PAGE_USER
    ☐ Retourner {entry_point, address_space, user_stack_top}
    ☐ Objectif : un ELF statique minimal chargé sans crash, segments aux bonnes adresses

  4.2 — Saut vers ring 3 via iret:
    NOTE: iret depuis ring 0 → ring 3 attend sur la pile (dans l'ordre push) :
    ss_user, esp_user, eflags, cs_user, eip_user
    ☐ Écrire process_enter_userspace(entry, user_stack_top) :
        ☐ Charger ds/es/fs/gs avec 0x23 (data ring 3 = 0x20 | 3)
        ☐ push 0x23 (ss user)
        ☐ push user_stack_top
        ☐ pushf, puis or eax, 0x200 (activer IF), push eax (eflags)
        ☐ push 0x1B (cs user = 0x18 | 3)
        ☐ push entry_point
        ☐ iret
    ☐ Tester avec un ELF minimal qui boucle infiniment → pas de GPF au saut
    ☐ Objectif : CPU en ring 3, cs == 0x1B, ds == 0x23 confirmés via GDB

  4.3 — Syscall interface int 0x80 (kernel/syscall/syscall.c):
    NOTE: Le vecteur 0x80 doit être une trap gate avec DPL=3
    pour que ring 3 puisse l'appeler sans GPF.
    ☐ Enregistrer le vecteur 0x80 dans l'IDT avec flags 0xEF (trap gate DPL=3)
    ☐ Écrire un stub assembleur isr_syscall qui passe la frame comme pointeur
    ☐ Définir les numéros de syscall :
        ☐ SYS_EXIT    = 1
        ☐ SYS_READ    = 3
        ☐ SYS_WRITE   = 4
        ☐ SYS_OPEN    = 5
        ☐ SYS_CLOSE   = 6
        ☐ SYS_GETPID  = 20
        ☐ SYS_FORK    = 2   (phase suivante)
        ☐ SYS_EXEC    = 11  (phase suivante)
    ☐ Écrire le dispatcher syscall_handler(interrupt_frame_t*) :
        ☐ eax = numéro syscall, ebx/ecx/edx = arguments
        ☐ switch(frame->eax) → appel de la bonne fonction sys_*
        ☐ Résultat mis dans frame->eax avant retour
    ☐ VALIDATION OBLIGATOIRE de chaque pointeur reçu depuis ring 3 :
        ☐ Écrire is_user_ptr(ptr, len) : vérifie que ptr est dans 0x400000..0xBFFFFFFF
        ☐ Tout syscall qui reçoit un pointeur appelle is_user_ptr avant de déréférencer
        ☐ Si pointeur invalide : retourner -EFAULT dans frame->eax, ne pas paniquer
    ☐ Implémenter sys_exit(code) :
        ☐ state = ZOMBIE, exit_code = code
        ☐ Libérer l'espace d'adressage user (vmm_destroy_space)
        ☐ Appeler schedule() — ne retourne pas
    ☐ Implémenter sys_write(fd, buf, len) :
        ☐ Valider buf+len avec is_user_ptr
        ☐ Pour fd 1 (stdout) : copier vers TTY via putchar loop
        ☐ Retourner le nombre d'octets écrits
    ☐ Implémenter sys_getpid() → retourner current_process->pid
    ☐ Test : programme ring 3 appelle sys_write(1, "hello\n", 6) → visible sur TTY
    ☐ Test : programme ring 3 appelle sys_exit(0) → kernel continue de tourner
    ☐ Test : programme ring 3 passe un pointeur kernel invalide → EFAULT, pas de panic
    ☐ Objectif : hello world complet en ring 3 via syscall

  4.4 — Gestion des faults ring 3:
    ☐ Modifier vmm_page_fault_handler : si fault en ring 3 sans COW → sys_exit(-1)
    ☐ Modifier isr_handler : si GPF (ISR 13) depuis ring 3 → afficher erreur + sys_exit(-1)
    ☐ Test : programme ring 3 déréférence NULL → message "Segfault pid=X", kernel continue
    ☐ Test : programme ring 3 tente d'accéder à 0xC0000000 (mémoire kernel) → GPF catchée
    ☐ Objectif : ring 3 peut crasher librement, kernel survit toujours


  ══════════════════════════════════════════════════════════════════
  PHASE 5 — VFS + SYSTÈMES DE FICHIERS
  ══════════════════════════════════════════════════════════════════
  NOTE SUR L'ORDRE :
  RamFS d'abord (pas de dépendance disque), puis DevFS, puis FAT32.
  "/" est toujours un RamFS — jamais FAT32 directement.
  Structure de l'arborescence au boot :
    /              ← RamFS (toujours présente, même sans disque)
    ├── dev/       ← DevFS
    │   ├── tty
    │   ├── com1
    │   └── null
    ├── tmp/       ← RamFS (fichiers temporaires)
    ├── config/    ← RamFS (configuration utilisateur)
    │   └── profiles/
    └── disk/      ← point de montage FAT32

  5.1 — VFS — couche d'abstraction (kernel/fs/vfs.c):
    ☐ Définir vfs_node_t :
        ☐ name[256], type (VFS_FILE / VFS_DIR / VFS_DEV), size, inode
        ☐ Pointeurs de fonctions : read, write, close, readdir, finddir, mkdir, unlink
        ☐ void *fs_data (données privées du filesystem — ramfs_inode_t*, fat32_node_t*...)
        ☐ uint32_t ref_count (pour savoir quand libérer le nœud)
    ☐ Définir mount_point_t {path[256], vfs_node_t *fs_root}
    ☐ Écrire vfs_mount(path, fs_root) : ajouter à la table des points de montage
    ☐ Écrire vfs_resolve(path) : trouver le bon fs_root selon le préfixe de chemin
    ☐ Écrire vfs_open(path) → vfs_node_t*
    ☐ Écrire vfs_read(node, offset, size, buf) → int32_t (octets lus)
    ☐ Écrire vfs_write(node, offset, size, buf) → int32_t (octets écrits)
    ☐ Écrire vfs_close(node) : décrémente ref_count, libère si 0
    ☐ Écrire vfs_readdir(node, index) → vfs_node_t* (entrée n du répertoire)
    ☐ Écrire vfs_mkdir(parent, name) → vfs_node_t*
    ☐ Écrire vfs_unlink(parent, name) → int
    ☐ Objectif : interface générique qui fonctionne identiquement pour tous les fs

  5.2 — RamFS (kernel/fs/ramfs.c):
    ☐ Définir ramfs_inode_t {name, type, size, data* (kmalloc), children_list}
    ☐ Implémenter ramfs_read, ramfs_write, ramfs_readdir, ramfs_finddir, ramfs_mkdir, ramfs_unlink
    ☐ Créer la racine RamFS au boot et la monter sur "/"
    ☐ Créer /dev, /tmp, /config, /config/profiles, /disk vides
    ☐ Test : créer /tmp/test.txt, écrire "hello", relire → "hello", supprimer → disparu
    ☐ Objectif : RamFS stable, toutes les opérations fonctionnelles

  5.3 — DevFS (kernel/fs/devfs.c):
    ☐ Pseudo-filesystem : les ops de read/write appellent directement les drivers
    ☐ Écrire devfs_register(name, read_fn, write_fn) pour ajouter un device en 1 ligne
    ☐ Enregistrer /dev/tty  : read = get_key, write = putchar loop
    ☐ Enregistrer /dev/com1 : read = serial_getchar (à écrire), write = serial_putchar loop
    ☐ Enregistrer /dev/null : read retourne 0, write retourne len sans rien faire
    ☐ Enregistrer /dev/zero : read retourne des zéros, write retourne len
    ☐ Monter le DevFS sur "/dev"
    ☐ Test : write(open("/dev/null"), "test", 4) → retourne 4, aucun output
    ☐ Objectif : tous les devices accessibles comme des fichiers via vfs_open

  5.4 — FAT32 kernel (kernel/fs/fat32.c):
    NOTE: La logique FAT32 existe déjà dans stage2.asm.
    Il faut la porter en C, pas la réécrire from scratch.
    ☐ Porter la lecture du BPB depuis stage2 (reserved_sectors, fat_size, root_cluster...)
    ☐ Calculer data_start = reserved + fat_count * fat_size
    ☐ Implémenter fat32_read_cluster(n, buf) : LBA = data_start + (n-2) * sectors_per_cluster
    ☐ Implémenter fat32_get_next_cluster(n) : lire l'entrée FAT à l'offset n*4
    ☐ Implémenter fat32_readdir(node, index) : lire les entrées 32 octets du répertoire
    ☐ Implémenter fat32_finddir(node, name) : chercher un nom 8.3 dans le répertoire
    ☐ Implémenter fat32_open(path) : descendre l'arbre de répertoires jusqu'au fichier
    ☐ Implémenter fat32_read(node, offset, size, buf) : suivre la chaîne de clusters
    ☐ Monter le FAT32 sur "/disk" au boot (si la partition est détectée)
    ☐ Test : vfs_open("/disk/kernel.bin") → lecture réussie, magic ELF présent
    ☐ Objectif : vfs_open("/disk/test.txt") retourne le bon contenu

  5.5 — File descriptors par processus:
    ☐ Ajouter int fds[64] dans process_t (index = numéro fd, valeur = pointeur vfs_node_t*)
    ☐ Écrire fd_alloc(process, node) : retourne le premier fd libre (≥ 3)
    ☐ Écrire fd_free(process, fd) : ferme le node, met fd à NULL
    ☐ Initialiser fd 0/1/2 dans process_create : stdin = /dev/tty, stdout = /dev/tty, stderr = /dev/com1
    ☐ Modifier sys_write et sys_read pour utiliser process->fds[fd]
    ☐ Implémenter sys_open(path, flags) → fd_alloc + vfs_open
    ☐ Implémenter sys_close(fd) → fd_free
    ☐ Objectif : programme ring 3 peut lire/écrire des fichiers via syscalls


  ══════════════════════════════════════════════════════════════════
  PHASE 6 — PROCESSUS COMPLETS + IPC
  ══════════════════════════════════════════════════════════════════

  6.1 — Gestion complète des processus:
    ☐ Implémenter sys_fork() :
        ☐ Dupliquer process_t (nouveau pid, même parent_pid)
        ☐ Dupliquer l'espace d'adressage : toutes les pages user en COW
            NOTE: les deux processus partagent les frames physiques en lecture,
            COW se déclenchera à la première écriture de chacun
        ☐ Dupliquer la table de fd (vfs_node ref_count++)
        ☐ Ajouter au scheduler
        ☐ Dans le parent : retourner child_pid dans eax
        ☐ Dans l'enfant : retourner 0 dans eax (modifier la frame sauvegardée)
    ☐ Implémenter sys_exec(path) :
        ☐ Charger l'ELF depuis le VFS
        ☐ Remplacer l'espace d'adressage courant (vmm_destroy_space + elf_load)
        ☐ Fermer les fd non-hérités
        ☐ Sauter vers le nouveau entry point
    ☐ Implémenter process_wait(pid) :
        ☐ Bloquer le processus courant (state = BLOCKED)
        ☐ Schedule vers un autre processus
        ☐ Quand le fils passe ZOMBIE : réveiller le parent (state = READY)
        ☐ Récupérer exit_code du zombie et libérer ses ressources restantes
    ☐ Objectif : fork() + exec() + wait() fonctionnels, zombies nettoyés proprement

  6.2 — Pipes (kernel/ipc/pipe.c):
    ☐ Définir pipe_t {buffer[4096], read_pos, write_pos, readers, writers, spinlock}
    ☐ Écrire pipe_create() → alloue deux vfs_node_t (read end + write end)
    ☐ Implémenter pipe_read : bloquant si buffer vide (state = BLOCKED + schedule)
    ☐ Implémenter pipe_write : bloquant si buffer plein
    ☐ Fermeture automatique quand readers ou writers tombe à 0
    ☐ Implémenter sys_pipe(fds[2]) : crée le pipe, alloue 2 fd dans le processus courant
    ☐ Héritage des pipes lors de fork()
    ☐ Test : processus A écrit dans un pipe, processus B lit → données reçues correctement
    ☐ Objectif : pipe fonctionnel sans perte de données

  6.3 — Signaux:
    ☐ Définir SIGKILL=9, SIGSEGV=11, SIGTERM=15, SIGCHLD=17
    ☐ Ajouter pending_signals (uint32_t bitmap) dans process_t
    ☐ Écrire signal_send(pid, sig) : mettre le bit correspondant dans pending_signals
    ☐ Vérifier pending_signals au retour de chaque syscall et de chaque interrupt handler
    ☐ SIGKILL → sys_exit(-1) forcé, non catchable
    ☐ SIGSEGV → message "Segmentation fault", sys_exit(-1)
    ☐ SIGTERM → sys_exit(0) par défaut, catchable par le processus (phase ultérieure)
    ☐ SIGCHLD → notifier le parent quand un fils passe ZOMBIE
    ☐ Test : process A envoie SIGKILL à process B → B se termine, A continue
    ☐ Objectif : Ctrl+C dans le shell envoie SIGTERM au processus foreground


  ══════════════════════════════════════════════════════════════════
  PHASE 7 — MINI LIBC
  ══════════════════════════════════════════════════════════════════
  NOTE: La mini libc est nécessaire avant le GUI et les applications.
  Elle sert de base à tout programme user et aux applis graphiques.

  7.1 — string.h (kernel/lib/string.c):
    ☐ strlen, strcpy, strncpy, strlcpy (version safe)
    ☐ strcmp, strncmp, strcasecmp
    ☐ strcat, strncat
    ☐ strchr, strrchr, strstr
    ☐ strtok (avec contexte thread-safe : strtok_r)
    ☐ Tests limites : chaîne vide, NULL, chaîne sans \0 (strncpy/strncmp)
    ☐ Objectif : tous les cas limites couverts sans UB

  7.2 — memory (kernel/lib/memory.c):
    ☐ memcpy, memset, memcmp
    ☐ memmove (gère les buffers qui se chevauchent)
    ☐ Test memmove avec src et dst qui se chevauchent (sens avant et arrière)
    ☐ Objectif : memmove stable sur tous les cas de chevauchement

  7.3 — ctype.h (kernel/lib/ctype.c):
    ☐ isdigit, isalpha, isalnum, isspace, ispunct
    ☐ isupper, islower, toupper, tolower
    ☐ isxdigit (pour parser les nombres hex dans le shell)

  7.4 — stdlib.h (kernel/lib/stdlib.c):
    ☐ atoi (string → int)
    ☐ itoa(int n, char *buf, int base) : décimal, hex, octal
    ☐ strtol(str, endptr, base)
    ☐ malloc / free (wrappers kmalloc / kfree)
    ☐ realloc : alloc + memcpy + free si agrandissement nécessaire
    ☐ abort() : kernel panic avec message
    ☐ exit(code) : sys_exit si ring 3, halt si ring 0

  7.5 — stdio.h (kernel/lib/stdio.c):
    ☐ printf(fmt, ...) : %d, %s, %x, %u, %c, %p, %% (padding minimal avec largeur fixe)
    ☐ sprintf(buf, fmt, ...) : formatage dans un buffer
    ☐ snprintf(buf, n, fmt, ...) : version safe avec limite de taille
    ☐ putchar, puts (wrappers TTY)
    ☐ Objectif : printf("%d %s %x\n", 42, "test", 0xABCD) → sortie correcte


  ══════════════════════════════════════════════════════════════════
  PHASE 8 — SHELL TEXTE COMPLET
  ══════════════════════════════════════════════════════════════════
  NOTE: Le shell actuel est très basique. Il faut le réécrire
  proprement maintenant qu'on a les syscalls, le VFS et la libc.

  8.1 — Amélioration du driver clavier (kernel/drivers/keyboard/keyboard.c):
    ☐ Ajouter gestion Caps Lock (toggle, indicateur LED via port 0x60)
    ☐ Ajouter gestion AltGr (scancodes 0xE0 + code) pour les caractères spéciaux
    ☐ Ajouter flèches haut/bas/gauche/droite (scancodes étendus 0xE0)
    ☐ Ajouter Home, End, PageUp, PageDown, Delete, Insert
    ☐ Distinguer key press et key release dans les events

  8.2 — readline() amélioré:
    ☐ Buffer jusqu'au \n (max 512 chars)
    ☐ Backspace : effacer buffer + TTY (déjà partiellement fait)
    ☐ Flèche gauche/droite : déplacer curseur, insérer/supprimer au milieu de la ligne
    ☐ Flèche haut/bas : historique de commandes (tableau circulaire 50 entrées)
    ☐ Home / End : aller en début / fin de ligne
    ☐ Ctrl+C : envoyer SIGTERM au processus foreground
    ☐ Ctrl+L : clear screen
    ☐ Tab : autocomplétion basique sur les noms de fichiers du répertoire courant

  8.3 — Commandes du shell:
    ☐ help    : liste toutes les commandes
    ☐ clear   : efface l'écran (déjà fait)
    ☐ echo    : affiche les arguments (déjà fait partiellement)
    ☐ reboot  : redémarre via port 0x64 (déjà fait)
    ☐ mem     : affiche RAM totale, pages libres, heap utilisé
    ☐ pwd     : affiche le répertoire courant (process->cwd)
    ☐ cd      : changer de répertoire, met à jour process->cwd
    ☐ ls      : liste le répertoire via vfs_readdir
    ☐ cat     : affiche le contenu d'un fichier via vfs_read
    ☐ mkdir   : crée un répertoire via vfs_mkdir
    ☐ rm      : supprime un fichier via vfs_unlink
    ☐ exec    : charge et exécute un ELF ring 3 depuis le VFS, attend sa fin
    ☐ ps      : liste les processus actifs (pid, state, name)
    ☐ kill    : envoyer un signal à un processus par pid
    ☐ Objectif : session complète sans crash, historique fonctionnel, autocomplétion basique


  ══════════════════════════════════════════════════════════════════
  PHASE 9 — COUCHE GRAPHIQUE
  ══════════════════════════════════════════════════════════════════
  NOTE SUR L'ORDRE :
  1. Framebuffer abstrait (gfx_surface_t)
  2. Backend VESA (le plus simple)
  3. Primitives de dessin
  4. Rendu texte PSF
  5. Double buffering
  6. Curseur souris
  7. Window Manager
  Ce n'est qu'après ces 7 étapes qu'on peut construire les 3 WM Vault-OS.

  9.1 — Framebuffer VESA (kernel/gfx/framebuffer.c):
    ☐ Passer les infos VESA depuis le bootloader via une structure en mémoire partagée
        ☐ Modifier stage2 pour détecter et stocker fb_addr, width, height, pitch, bpp
        ☐ Stocker à une adresse connue (ex: 0x7000) accessible depuis kmain
    ☐ Lire la structure VESA dans kmain, mapper les pages du framebuffer via vmm_map()
    ☐ Définir gfx_surface_t {uint32_t *pixels, width, height, pitch, bpp}
    ☐ Créer gfx_screen (surface pointant directement sur le framebuffer physique)
    ☐ Test : mettre tous les pixels à 0xFF0000 (rouge) → écran rouge
    ☐ Objectif : pixel visible à l'écran en mode graphique

  9.2 — Primitives de dessin (kernel/gfx/primitives.c):
    ☐ gfx_put_pixel(surface, x, y, color) avec bounds checking
    ☐ gfx_fill_rect(surface, x, y, w, h, color) : remplissage rapide ligne par ligne
    ☐ gfx_draw_rect(surface, x, y, w, h, color) : contour seulement
    ☐ gfx_draw_line(surface, x0, y0, x1, y1, color) : algorithme de Bresenham
    ☐ gfx_draw_circle(surface, cx, cy, r, color) : algorithme de Midpoint
    ☐ gfx_blit(dst, src, x, y) : copier une surface dans une autre (sans transparence)
    ☐ gfx_blit_alpha(dst, src, x, y) : copie avec canal alpha (pour les fenêtres)
    ☐ gfx_create_surface(w, h) : alloue un back buffer via kmalloc
    ☐ gfx_destroy_surface(s) : libère le buffer

  9.3 — Rendu texte (kernel/gfx/font.c):
    ☐ Parser une police PSF 8x16 (Linux console font, domaine public)
    ☐ Stocker la police compilée dans le kernel (tableau statique de glyphes)
    ☐ gfx_draw_char(surface, x, y, c, fg, bg)
    ☐ gfx_draw_string(surface, x, y, str, fg, bg) avec gestion \n et \t
    ☐ Test : "Vault-OS" affiché en blanc sur fond noir en mode graphique

  9.4 — Double buffering:
    ☐ Allouer back_buffer (gfx_surface_t en RAM, même taille que gfx_screen)
    ☐ Toutes les opérations de rendu → back_buffer uniquement
    ☐ gfx_flip() : memcpy back_buffer.pixels → gfx_screen.pixels (ou blit optimisée)
    ☐ Appeler gfx_flip() en fin de chaque frame du WM
    ☐ Objectif : animation sans flickering (ex: rectangle qui se déplace)

  9.5 — Driver souris PS/2 (kernel/drivers/mouse/mouse.c):
    ☐ Initialiser la souris PS/2 via port 0x64/0x60 (activer le second port, activer les paquets)
    ☐ Handler IRQ12 : lire les 3 octets du paquet souris
    ☐ Parser delta X, delta Y, boutons gauche/droite/milieu
    ☐ Pousser INPUT_MOUSE_MOVE / INPUT_MOUSE_BUTTON dans le ring buffer
    ☐ Objectif : mouvements souris détectés, coordonnées X/Y qui changent

  9.6 — Curseur souris (kernel/gfx/cursor.c):
    ☐ Sprite curseur 12x20 pixels (flèche ASCII art converti en bitmap)
    ☐ Sauvegarder les pixels sous le sprite avant de dessiner
    ☐ À chaque INPUT_MOUSE_MOVE : restaurer les pixels sauvegardés, mettre à jour pos, redessiner
    ☐ Clamp position dans [0, width-1] × [0, height-1]
    ☐ Dessiner le curseur après gfx_flip() (toujours par dessus tout)
    ☐ Objectif : curseur fluide sans laisser de trace à l'écran

  9.7 — Window Manager générique (kernel/gfx/wm.c):
    ☐ Définir window_t {id, x, y, w, h, title[128], surface, visible, focused, zorder}
    ☐ Liste globale des fenêtres triée par z-order croissant
    ☐ wm_create_window(x, y, w, h, title) : gfx_create_surface + insertion liste
    ☐ wm_destroy_window(id) : gfx_destroy_surface + retrait liste
    ☐ wm_render() :
        ☐ Effacer back_buffer avec la couleur du bureau
        ☐ Pour chaque window dans l'ordre z croissant :
            ☐ Dessiner décoration (barre titre, bordure, bouton fermeture rouge)
            ☐ gfx_blit(back_buffer, window->surface, window->x, window->y)
        ☐ Dessiner curseur souris par dessus tout
        ☐ gfx_flip()
    ☐ wm_dispatch_events() :
        ☐ Lire input_events depuis le ring buffer
        ☐ Clic dans barre titre → activer drag (déplacer avec MOUSE_MOVE)
        ☐ Clic dans fenêtre → focus (z-order max) + barre titre couleur active
        ☐ Clic bouton fermeture → wm_destroy_window
        ☐ Clic dans fond de bureau → défocus
    ☐ Boucle principale GUI : wm_dispatch_events() + wm_render() à 60 Hz (via PIT)
    ☐ Test : ouvrir 3 fenêtres, les déplacer, les fermer → pas de fuite mémoire
    ☐ Objectif : WM stable, drag fluide, focus correct


  ══════════════════════════════════════════════════════════════════
  PHASE 10 — PROFILS VAULT-OS
  ══════════════════════════════════════════════════════════════════
  NOTE: C'est ici que la vision Vault-OS devient concrète.
  Chaque profil est un espace d'adressage COW isolé avec son propre WM.
  Si un profil crashe complètement, les autres continuent de tourner.
  Le Core reste accessible depuis n'importe quel état.

  10.1 — Structure vault_profile_t (kernel/vault/profile.c):
    ☐ Définir vault_profile_t :
        ☐ uint32_t id
        ☐ char name[64]
        ☐ address_space_t *space  (espace d'adressage COW isolé)
        ☐ process_t *main_process (processus principal du profil)
        ☐ char config_path[256]   (chemin vers /config/profiles/<n>/)
        ☐ bool active
        ☐ uint32_t wm_type        (WM_WINDOWS_LIKE / WM_TILING / WM_CANVAS)
    ☐ Écrire vault_create_profile(name, wm_type) :
        ☐ Créer l'espace d'adressage : vmm_create_space()
        ☐ Mapper les pages Core en COW (partagées, lecture seule, COW sur écriture)
        ☐ Créer /config/profiles/<id>/ dans le VFS
        ☐ Créer le processus principal dans l'espace isolé
        ☐ Démarrer le WM correspondant au wm_type
    ☐ Écrire vault_switch_profile(profile) : mettre le focus sur ce profil
    ☐ Écrire vault_destroy_profile(profile) :
        ☐ Tuer tous les processus du profil (SIGKILL)
        ☐ vmm_destroy_space() → libère toutes les pages COW privées
        ☐ Libérer les ressources VFS spécifiques au profil
    ☐ Test : créer 3 profils simultanément → 3 cr3 différents vérifiés
    ☐ Test : écrire dans l'espace mémoire d'un profil → les 2 autres non affectés
    ☐ Objectif : profils réellement isolés au niveau hardware

  10.2 — Profil A — Windows-like (kernel/vault/wm_classic.c):
    ☐ Bureau avec couleur de fond configurable depuis /config/profiles/A/desktop.cfg
    ☐ Barre de tâches 32px en bas :
        ☐ z-order maximum (au dessus de toutes les fenêtres sauf curseur)
        ☐ Bouton par fenêtre ouverte (clic → focus + raise)
        ☐ Horloge en bas à droite (lue depuis un driver RTC à implémenter)
        ☐ Bouton "démarrer" en bas à gauche (menu basique)
    ☐ Icônes sur le bureau (tableau de {label, icon_bitmap, action})
    ☐ Double-clic sur icône → lancer l'application associée
    ☐ Fenêtres avec min/max/close, redimensionnement par les bords
    ☐ Menu contextuel clic droit sur le bureau (Nouveau fichier, Rafraîchir...)
    ☐ Objectif : expérience proche de Windows, utilisable sans doc

  10.3 — Profil B — Tiling WM (kernel/vault/wm_tiling.c):
    NOTE: Inspiré de i3/Hyprland. Les fenêtres se placent automatiquement.
    ☐ Arbre de disposition : nœuds {type (H_SPLIT/V_SPLIT/LEAF), ratio, enfants}
    ☐ Nouvelle fenêtre : divisée dans le conteneur actif selon la direction courante
    ☐ Raccourcis clavier configurables depuis /config/profiles/B/keybinds.cfg :
        ☐ Super+Enter → ouvrir terminal
        ☐ Super+Q → fermer fenêtre focusée
        ☐ Super+H/V → changer direction de split
        ☐ Super+Flèches → changer le focus
        ☐ Super+Shift+Flèches → déplacer la fenêtre dans l'arbre
        ☐ Super+F → toggle plein écran pour la fenêtre focusée
        ☐ Super+1..9 → workspaces (espaces de travail)
    ☐ Workspaces : 9 espaces indépendants, chacun avec son propre arbre
    ☐ Barre de statut en haut : workspace actif, fenêtre focusée, heure
    ☐ Gaps configurables entre les fenêtres (outer_gap, inner_gap dans keybinds.cfg)
    ☐ Objectif : power user peut tout faire au clavier, zéro souris nécessaire

  10.4 — Profil C — Canvas infini (kernel/vault/wm_canvas.c):
    NOTE: Inspiré de vxwm / Zellij. Espace 2D navigable librement.
    ☐ Définir canvas_viewport_t {offset_x, offset_y, zoom (0.1 → 3.0)}
    ☐ Définir canvas_window_t {window_t, canvas_x, canvas_y}
        NOTE: canvas_x/canvas_y sont les coordonnées absolues sur le canvas infini,
        indépendantes du viewport courant.
    ☐ Rendu : pour chaque canvas_window, calculer screen_x = canvas_x - viewport.offset_x
    ☐ Navigation au clavier :
        ☐ Ctrl+Flèches → déplacer le viewport
        ☐ Ctrl++ / Ctrl+- → zoom in / out (redimensionner les fenêtres à l'écran)
        ☐ Ctrl+0 → revenir au zoom 1.0
    ☐ Navigation à la souris :
        ☐ Clic + drag sur le fond → déplacer le viewport
        ☐ Scroll → zoom centré sur la position du curseur
    ☐ Mini-map :
        ☐ Overlay 200x150 pixels en bas à droite
        ☐ Représentation de toutes les fenêtres à l'échelle
        ☐ Rectangle bleu = zone visible (viewport)
        ☐ Clic sur la mini-map → téléporter le viewport à cet endroit
    ☐ Mode overview (Super+Tab) :
        ☐ Zoom arrière automatique pour tout voir
        ☐ Clic sur une fenêtre → zoom avant et focus sur cette fenêtre
    ☐ Placement libre : fenêtres posables n'importe où, y compris hors écran
    ☐ Snap optionnel : maintenir Shift lors du drag → snap à la grille de 32px
    ☐ Objectif : navigation fluide sur un canvas de 10000×10000 pixels virtuels


  ══════════════════════════════════════════════════════════════════
  PHASE 11 — DRIVERS USB
  ══════════════════════════════════════════════════════════════════
  NOTE SUR L'ORDRE UHCI → EHCI → xHCI :
  Toujours commencer par UHCI (le plus simple).
  Ne jamais commencer par xHCI — c'est une erreur classique qui mène à des semaines perdues.

  11.1 — Bus PCI (kernel/dev/pci.c):
    ☐ Définir pci_device_t {bus, slot, func, vendor_id, device_id, class, subclass, bars[6], irq}
    ☐ Écrire pci_read_config(bus, slot, func, offset) via ports 0xCF8 / 0xCFC
    ☐ Scanner tous les bus/slot/func (0-255 / 0-31 / 0-7)
    ☐ Lire et stocker les 6 BARs (distinguer MMIO vs I/O port)
    ☐ Afficher au boot : bus:slot vendor:device class subclass
    ☐ Objectif : tous les devices PCI listés au boot dans le serial

  11.2 — UHCI USB 1.1:
    ☐ Détecter le contrôleur UHCI via PCI (class 0x0C, subclass 0x03, prog-if 0x00)
    ☐ Lire le BAR I/O du contrôleur
    ☐ Reset le contrôleur via USBCMD
    ☐ Allouer et initialiser la Frame List (1024 entrées, alignée 4Ko)
    ☐ Implémenter les Transfer Descriptors (TD) et Queue Heads (QH)
    ☐ Énumération USB : GET_DESCRIPTOR à l'adresse 0, SET_ADDRESS, GET_DESCRIPTOR complet
    ☐ Objectif : Device Descriptor affiché pour un device branché

  11.3 — Driver HID clavier USB:
    ☐ Détecter classe HID 0x03, usage keyboard dans les descripteurs
    ☐ SET_CONFIGURATION, SET_IDLE, GET_REPORT_DESCRIPTOR
    ☐ Transfert Interrupt périodique sur l'endpoint IN
    ☐ Parser les HID reports (modifier keys + keycodes[6])
    ☐ Convertir HID keycode → input_event, pousher dans le ring buffer
    ☐ Objectif : clavier USB fonctionne identiquement au PS/2 dans le shell

  11.4 — EHCI USB 2.0 (après UHCI validé):
    ☐ Détecter via PCI (prog-if 0x20), mapper BAR MMIO
    ☐ Parser Capability Registers, reset, init Queue Heads + Transfer Descriptors
    ☐ Objectif : device USB 2.0 détecté et initialisé


  ══════════════════════════════════════════════════════════════════
  PHASE 12 — APPLICATIONS
  ══════════════════════════════════════════════════════════════════

  12.1 — Terminal graphique (apps/terminal/):
    ☐ Fenêtre WM 640x400, fond noir, police PSF
    ☐ Buffer 80x25 caractères avec scroll
    ☐ Recevoir GUI_EVENT_KEY_DOWN → readline → afficher via gfx_draw_char
    ☐ Scroll automatique à la 25ème ligne
    ☐ Shell complet branché sur le terminal graphique
    ☐ Copier/coller avec Ctrl+Shift+C / Ctrl+Shift+V
    ☐ Objectif : terminal graphique aussi complet que le shell TTY

  12.2 — Gestionnaire de fichiers (apps/filemanager/):
    ☐ Fenêtre WM avec liste des fichiers (vfs_readdir)
    ☐ Icônes différentes pour fichiers, dossiers, exécutables
    ☐ Navigation : double-clic dossier, bouton parent, chemin dans la barre titre
    ☐ Barre latérale : raccourcis /disk, /tmp, /config
    ☐ Ouvrir : .elf → exec ring 3, .txt → éditeur, .npl → interpréteur NPL
    ☐ Clic droit → menu contextuel (Copier, Coller, Renommer, Supprimer)
    ☐ Objectif : navigation complète du VFS en mode graphique

  12.3 — Éditeur de texte minimal (apps/editor/):
    ☐ Fenêtre WM, ouverture de fichier via argument ou dialogue
    ☐ Buffer de lignes (tableau de char* alloués dynamiquement)
    ☐ Navigation : flèches, Home, End, PageUp, PageDown
    ☐ Insertion / suppression de caractères, gestion des newlines
    ☐ Ctrl+S : sauvegarde via vfs_write
    ☐ Ctrl+Z : undo (buffer d'opérations)
    ☐ Objectif : éditer un fichier de config et le sauvegarder

  12.4 — Barre de tâches globale (tous les profils):
    ☐ Overlay permanent, z-order maximal dans chaque profil
    ☐ Bouton par fenêtre ouverte (clic → focus)
    ☐ Indicateur de profil actif (A / B / C)
    ☐ Raccourci pour switcher de profil (ex: Ctrl+Super+1/2/3)
    ☐ Horloge en temps réel (driver RTC à implémenter)
    ☐ Indicateur mémoire (% utilisé, affiché en temps réel)


  ══════════════════════════════════════════════════════════════════
  PHASE 13 — TESTS & POLISH FINAL
  ══════════════════════════════════════════════════════════════════

  Tests kernel:
    ☐ Provoquer chaque exception CPU (0, 6, 8, 13, 14) → message correct, pas de reboot
    ☐ Division par zéro depuis ring 3 → process tué, kernel continue
    ☐ Accès mémoire kernel depuis ring 3 → GPF catchée, process tué proprement
    ☐ Allouer toute la RAM → kmalloc retourne NULL, kernel ne crashe pas
    ☐ 1000 alloc/free de tailles variées → heap stable, pas de fragmentation

  Tests multitâche:
    ☐ 10 processus simultanés pendant 60 secondes → pas de deadlock, pas de corruption
    ☐ fork() + exec() + wait() : 50 cycles → pas de zombie non nettoyé
    ☐ SIGKILL depuis un processus sur un autre → cible terminée, envoyeur continue
    ☐ Pipe : 1 Mo de données transférées → pas de perte

  Tests profils:
    ☐ 3 profils simultanés → 3 cr3 différents confirmés via GDB
    ☐ Provoquer un crash dans un profil (écriture invalide) → les 2 autres continuent
    ☐ Détruire un profil → toute la mémoire COW est libérée (pmm vérifié)

  Tests VFS:
    ☐ Lire /disk/test.txt (FAT32) et /tmp/test.txt (RamFS) → contenu correct
    ☐ Écrire /tmp/test.txt, relire → données intactes
    ☐ /dev/null, /dev/zero : comportement attendu, pas de crash
    ☐ 100 open/close en boucle → pas de fuite de fd

  Tests GUI:
    ☐ 20 fenêtres ouvertes/fermées en boucle → pas de fuite mémoire
    ☐ Drag rapide sur tout l'écran → pas de glitch, curseur fluide
    ☐ Switch de profil 50 fois → WM stable, aucune corruption vidéo

  Polish visuel:
    ☐ Splash screen au démarrage (logo Vault-OS, barre de progression)
    ☐ Informations boot en serial : RAM détectée, version, devices PCI
    ☐ Messages d'erreur colorisés selon la gravité
    ☐ Écran kernel panic stylisé (rouge, blanc, message clair)
    ☐ Animation de démarrage des profils (fade-in)

  Expérience premier démarrage (OOBE):
    ☐ Écran de bienvenue avec logo et description Vault-OS
    ☐ Sélection langue FR / EN → change la keymap active
    ☐ Saisie nom d'utilisateur → stocké dans /config/user.cfg
    ☐ Choix du profil par défaut (A / B / C) avec description de chacun
    ☐ Bouton "Commencer" → lancement du profil choisi
    ☐ Flag /config/oobe.done → OOBE ne se relance pas au prochain démarrage

  Hardware réel:
    ☐ Créer une image USB bootable (dd sur Linux, Rufus sur Windows)
    ☐ Boot sur vrai PC : UEFI Legacy / CSM activé si besoin
    ☐ Vérifier que la RAM détectée via E820 est correcte sur vrai hardware
    ☐ Vérifier le framebuffer VESA sur vrai hardware (résolution, couleurs)
    ☐ Clavier PS/2 ou USB HID fonctionnel hors QEMU
    ☐ Tester les 3 profils sur vrai hardware → stabilité confirmée

  Distribution:
    ☐ Image ISO finale propre avec tous les binaires
    ☐ README complet : prérequis, build, run QEMU, run hardware
    ☐ Documentation de l'architecture : diagramme des couches, rôle de chaque composant
    ☐ Guide de création d'un nouveau profil custom
    ☐ Objectif : quelqu'un peut builder et tester Vault-OS depuis zéro sans aide
