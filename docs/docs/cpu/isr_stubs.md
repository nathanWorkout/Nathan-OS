---
sidebar_position: 8
title: ISR Stubs
---

# ISR Stubs

## Rôle

Fournit les points d'entrée assembleur pour les 32 exceptions CPU et les IRQ matériels. Chaque stub est une fonction appelée directement par le CPU lors d'une interruption, c'est le premier code qui s'exécute avant que le contrôle passe au handler C.

## Fichier source

- `isr_stubs.asm`

## Fonctionnement

### Le problème de l'error code

Certaines exceptions CPU poussent automatiquement un error code sur la stack avant d'appeler le handler, d'autres non. Pour que `isr_common` reçoive une stack identique dans tous les cas, les exceptions sans error code en poussent un factice à 0.

```nasm
isr0:  push 0   ; error code factice
       push 0   ; numéro d'exception
       jmp isr_common

isr8:           ; le CPU pousse déjà un error code
       push 8   ; numéro d'exception
       jmp isr_common
```

Les exceptions qui poussent un error code réel sont : 8 (Double Fault), 10, 11, 12, 13 (General Protection Fault), 14 (Page Fault), 17, 21. Toutes les autres poussent `0` en premier.

### isr_common

```nasm
isr_common:
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    cld
    mov rdi, rsp

    call isr_handler

    pop r15
    ...
    pop rax
    add rsp, 16

    db 0x48
    iret
```

Tous les stubs convergent vers `isr_common`. Il sauvegarde les 15 registres généraux sur la stack, puis passe `rsp` dans `rdi`, premier argument de la convention d'appel System V. `isr_handler` reçoit donc un pointeur vers le sommet de la stack, qui correspond exactement à la structure `interrupt_frame_t` définie dans [`isr.c`](./isr).

`cld` remet le flag direction à 0 avant l'appel C. La convention System V exige que `DF` soit à 0 à l'entrée d'une fonction, le CPU ne le garantit pas en entrant dans un handler.

Au retour, les registres sont restaurés dans l'ordre inverse. `add rsp, 16` saute par-dessus les deux valeurs poussées par le stub (error code et numéro d'exception), elles ont servi, elles ne font pas partie du contexte à restaurer.

#### Le préfixe REX.W sur iret

```nasm
db 0x48
iret
```

`iret` seul est encodé en 32 bits par NASM même en mode 64 bits, il dépilerait `rip`, `cs` et `rflags` sur 32 bits et corromprait la stack. Le préfixe `0x48` force l'encodage REX.W qui passe l'instruction en 64 bits, équivalent à `iretq`. C'est un contournement d'une limitation de l'assembleur.

### IRQ matériels

Les IRQ suivent la même mécanique de sauvegarde/restauration mais sans error code ni numéro d'exception, ils n'en ont pas besoin car chaque IRQ a son propre stub dédié.

```nasm
irq0:
    push rax ... push r15
    call irq0_handler
    mov al, 0x20
    out 0x20, al    ; EOI au PIC maître
    pop r15 ... pop rax
    db 0x48
    iret
```

La différence avec les exceptions est l'EOI (End Of Interrupt) : après avoir appelé le handler, le stub envoie `0x20` au port `0x20` pour signaler au PIC qu'il peut envoyer de nouveaux IRQ. Sans cet acquittement le PIC reste bloqué et aucune autre interruption matérielle ne peut se déclencher.

`irq12` (souris) est connecté au PIC esclave — il nécessite un double acquittement : `0xA0` au PIC esclave d'abord, puis `0x20` au PIC maître.

```nasm
irq12:
    ...
    call irq12_handler
    mov al, 0x20
    out 0xA0, al    ; EOI PIC esclave
    out 0x20, al    ; EOI PIC maître
    ...
```

`irq1` (clavier) n'envoie pas d'EOI depuis le stub, c'est le handler C dans [`isr.c`](./isr) qui s'en charge après avoir lu la touche.

## Ce qui est appelé ensuite

Les adresses de ces stubs sont enregistrées dans l'IDT par `isr_init` dans [`isr.c`](./isr). C'est la dernière étape avant que l'IDT soit opérationnelle.