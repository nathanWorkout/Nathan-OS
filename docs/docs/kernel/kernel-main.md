---
sidebar_position: 4
title: kmain
---

# kmain

## Rôle
Point d'entrée C du kernel. Ne contient aucune logique propre, il orchestre l'initialisation séquentielle de tous les composants du kernel dans le bon ordre.

## Fichier source
- `kmain.c`

> Ce fichier évolue constamment. Pour la version exacte et à jour, consulte directement le [repo GitHub](https://github.com/nathanWorkout/Nathan-OS).

## Séquence d'initialisation

L'ordre est critique : chaque composant dépend de ceux initialisés avant lui.

### 1. CPU & interruptions (base)
```c
gdt_init();
idt_init();
isr_init();
serial_init();
```
Le GDT et l'IDT sont mis en place en premier, sans eux, toute exception CPU est fatale. Le port série est initialisé tôt pour permettre le debug dès le début.

### 2. Mémoire
```c
enable_nxe();
pmm_init(memmap_request.response);
address_space_t *kernel_space = vmm_create_kernel_space();
pmm_init_full(memmap_request.response, kernel_space);
vmm_apply_nx(kernel_space);
```
`enable_nxe()` active le bit NX dans le registre EFER, doit précéder tout mapping mémoire. Le PMM est initialisé en deux passes : une passe minimale pour pouvoir créer l'espace d'adressage kernel, puis une passe complète une fois le VMM opérationnel. `vmm_apply_nx` applique les permissions par section (`.text` exécutable, `.rodata` / `.data` / `.bss` NX).

### 3. Framebuffer & espace d'adressage
```c
Canvas screen = fb_get_canvas();
vmm_map_framebuffer(kernel_space, &screen);
vmm_switch_space(kernel_space);
pmm_switch_to_full();
enable_wp();
```
Le framebuffer est mappé avant le switch d'espace d'adressage. `vmm_switch_space` charge le nouveau PML4 dans `cr3`, à partir de ce moment, c'est le VMM kernel qui est actif. `enable_wp()` active Write Protect dans `cr0` : le kernel lui-même ne peut plus écrire sur les pages marquées read-only.

### 4. Périphériques
```c
pic_init();
pit_init(1000);
tss_init();
```
Le PIC est reconfiguré (IRQs remappés), le PIT cadencé à 1000 Hz (tick toutes les 1 ms), le TSS initialisé pour les futurs context switches.

### 5. Protection des structures critiques
```c
vmm_set_readonly(kernel_space, (uint64_t)gdt, sizeof(gdt_entry_t) * GDT_ENTRY_COUNT);
vmm_set_readonly(kernel_space, tss_get_addr(), tss_get_size());
```
Le GDT et le TSS sont verrouillés en lecture seule après initialisation. Toute tentative d'écriture dessus déclenchera un page fault.

### 6. Affichage & shell
```c
gfx_init(&screen);
tty_init(screen);
__asm__ volatile ("sti");
pic_clear_mask(0);
pic_clear_mask(1);
vaultfs_init(&g_vaultfs);
shell_run(&cv);
```
Le sous-système graphique et le TTY sont initialisés, les interruptions activées (`sti`), les IRQs clavier et timer démasqués. VaultFs est monté, puis le shell lancé, à partir de là le kernel est en attente d'input.

## Ce qui est appelé ensuite
Le kernel entre dans [`shell_run`](../shell/shell) et ne retourne jamais, la boucle `while(1) hlt` en fin de `kmain` est un filet de sécurité uniquement.
