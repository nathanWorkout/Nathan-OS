---
sidebar_position: 6
title: GDT
---

# GDT

## Rôle

Initialise la Global Descriptor Table : la table que le CPU consulte pour valider chaque accès mémoire selon le segment et le privilege level. Sans GDT valide, le CPU ne peut pas distinguer le code kernel du code utilisateur.

## Fichier source

- `gdt.c`

## Fonctionnement

### Structure d'une entrée

```c
typedef struct __attribute__((packed)) {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  flags_limit;
    uint8_t  base_high;
} gdt_entry_t;
```

Chaque entrée fait 8 octets. La base et la limite sont découpées en plusieurs champs éparpillés dans la struct : c'est un héritage du x86 16 bits qu'Intel a conservé pour rester rétrocompatible. `__attribute__((packed))` est obligatoire, sans lui, le compilateur insère du padding et le CPU lit une structure corrompue.

### Remplissage d'une entrée

```c
static void gdt_set_entry(int i, uint64_t base, uint64_t limit, uint8_t access, uint8_t flags) {
    gdt[i].limit_low   = limit & 0xFFFF;
    gdt[i].base_low    = base & 0xFFFF;
    gdt[i].base_middle = (base >> 16) & 0xFF;
    gdt[i].access      = access;
    gdt[i].flags_limit = ((limit >> 16) & 0x0F) | (flags & 0xF0);
    gdt[i].base_high   = (base >> 24) & 0xFF;
}
```

Le champ `flags_limit` est particulier : il contient à la fois les 4 bits hauts de la limite (`(limit >> 16) & 0x0F`) et les 4 bits hauts du paramètre `flags` (`flags & 0xF0`) combinés dans le même octet. C'est pourquoi les valeurs de flags passées ont toujours le nibble bas à 0 (`0xA0`, `0x80`) : les 4 bits bas seraient masqués de toute façon.

### Entrées déclarées

```c
gdt_set_entry(0, 0, 0, 0, 0);                              // null descriptor
gdt_set_entry(1, 0x00000000, 0x000FFFFF, 0x9A, 0xA0);      // code kernel ring 0
gdt_set_entry(2, 0x00000000, 0x000FFFFF, 0x92, 0x80);      // data kernel ring 0
gdt_set_entry(3, 0x00000000, 0x000FFFFF, 0xFA, 0xA0);      // code utilisateur ring 3
gdt_set_entry(4, 0x00000000, 0x000FFFFF, 0xF2, 0x80);      // data utilisateur ring 3
gdt_set_entry(5, 0, 0, 0x89, 0);                           // TSS low
gdt_set_entry(6, 0, 0, 0, 0);                              // TSS high
```

| Index | Sélecteur | Contenu | Access |
|-------|-----------|---------|--------|
| 0 | `0x00` | Null descriptor (obligatoire, le CPU interdit de l'utiliser) | `0x00` |
| 1 | `0x08` | Code kernel ring 0 | `0x9A` |
| 2 | `0x10` | Data kernel ring 0 | `0x92` |
| 3 | `0x18` | Code utilisateur ring 3 | `0xFA` |
| 4 | `0x20` | Data utilisateur ring 3 | `0xF2` |
| 5 | `0x28` | TSS low | `0x89` |
| 6 | `0x30` | TSS high | `0x00` |

L'octet `access` encode huit informations sur le segment :

```
0x9A = 1001 1010
       ||||||||
       |||||||+-- bit 0 : accédé (mis à 1 par le CPU quand il utilise le segment)
       ||||||+--- bit 1 : lisible (segment code) / écrivable (segment data)
       |||||+---- bit 2 : direction/conforming, 0 pour les segments kernel
       ||||+----- bit 3 : type, 1 = code / 0 = data
       |||+------ bit 4 : 1 = segment normal (code/data), 0 = segment système
       ||+------- bit 5 : DPL bit 0 ─┐ privilege level :
       |+-------- bit 6 : DPL bit 1  ┘ 00 = ring 0, 11 = ring 3
       +--------- bit 7 : présent, doit être à 1 sinon le CPU considère l'entrée invalide
```

| Access | Binaire     | Signification                            |
|--------|-------------|------------------------------------------|
| `0x9A` | `1001 1010` | Présent, ring 0, code exécutable/lisible |
| `0x92` | `1001 0010` | Présent, ring 0, data lecture/écriture   |
| `0xFA` | `1111 1010` | Présent, ring 3, code exécutable/lisible |
| `0xF2` | `1111 0010` | Présent, ring 3, data lecture/écriture   |
| `0x89` | `1000 1001` | Présent, ring 0, TSS disponible (type 9) |

La seule différence entre kernel et user est dans les bits 5-6 : `00` pour ring 0, `11` pour ring 3.

Le TSS occupe deux entrées consécutives (5 et 6) car en 64 bits une entrée TSS fait 16 octets. Les 32 bits hauts de la base ne rentrent pas dans les 8 octets d'une entrée standard et débordent dans l'entrée suivante.

### Chargement du GDTR

```c
static inline void gdt_load(gdt_descriptor_t *gdtr) {
    __asm__ volatile (
        "lgdt (%0)\n"
        "lea 1f(%%rip), %%rax\n"
        "push $0x08\n"
        "push %%rax\n"
        "lretq\n"
        "1:\n"
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        : : "r"(gdtr) : "rax", "memory");
}
```

#### lgdt

`lgdt` charge le registre GDTR avec la structure passée en paramètre : taille de la table en octets moins 1, et son adresse en mémoire. Après cette instruction le CPU sait où se trouve la nouvelle table, mais aucun registre de segment n'est rechargé, `cs`, `ds`, `ss` pointent encore sur les anciennes entrées.

#### Le problème de cs

`ds`, `es`, `fs`, `gs` et `ss` se rechargent avec un `mov` classique. `cs` est le seul registre de segment que le CPU interdit de modifier de cette façon : il détermine le ring courant, et une modification directe permettrait à du code user de s'élever en ring 0 sans passer par une gate contrôlée.

La seule façon de modifier `cs` est via un far jump ou un far return, qui rechargent `cs` et `rip` en même temps de façon atomique. Le far jump avec adresse absolue 64 bits n'existe plus en mode long, il reste le far return, simulé en construisant manuellement ce qu'il attend sur la stack.

#### lea 1f(%%rip), %%rax

Calcule l'adresse du label `1:` et la place dans `rax`. L'adresse est calculée en RIP-relative : `adresse = rip + offset`. Le `f` dans `1f` signifie forwar, l'assembleur cherche le prochain label `1:` en avant dans le code, convention pour les labels locaux anonymes. Après cette instruction, `rax` contient l'adresse exacte où l'exécution doit reprendre après le far return.

#### push $0x08 et push %%rax

Un far return attend deux valeurs sur la stack : l'adresse de retour (`rip`), puis le sélecteur de segment (`cs`). Elles sont empilées dans l'ordre inverse car la stack croît vers le bas.

```
état de la stack avant lretq :

adresse basse
+------------+ <- rsp
|   0x08     |              sera dépilé dans cs
+------------+
| adresse 1: |              sera dépilé dans rip
+------------+
adresse haute
```

`0x08` encode l'index 1 du GDT (`0000 0000 0000 1000` : index 1, TI 0, RPL 0), le segment code kernel ring 0.

#### lretq

Dépile `rip` puis `cs`, valide l'entrée GDT pointée, recharge les deux registres en une seule opération atomique. `lretq` et non `lret` car en 64 bits les valeurs dépilées sont 64 bits, utiliser `lret` corromprait la stack.

#### Les mov finaux

```nasm
1:
mov $0x10, %%ax
mov %%ax, %%ds
...
```

`cs` est à jour. Les autres registres de segment sont rechargés avec `0x10` : index 2 du GDT, segment data kernel ring 0. Le passage par `ax` est obligatoire, les registres de segment n'acceptent pas de valeur immédiate comme opérande source. `rax` est listé dans les clobbers pour signaler au compilateur que la fonction le modifie.

#### Résumé du flux

```
lgdt          → le CPU connaît le nouveau GDT, aucun segment rechargé
lea + push    → faux contexte de retour construit sur la stack
                rsp pointe sur [0x08, adresse 1:]
lretq         → cs = 0x08, rip = adresse 1:, stack nettoyée
1:            → ds, es, fs, gs, ss = 0x10
                tous les registres de segment sont à jour
```

### Mise à jour du TSS

```c
void gdt_set_tss_entry(uint64_t base, uint64_t limit) {
    gdt_set_entry(5, base, limit, 0x89, 0x00);
    uint32_t *high = (uint32_t *)&gdt[6];
    high[0] = (base >> 32) & 0xFFFFFFFF;
    high[1] = 0;
    gdt_load(&gdtr);
}
```

Appelée par `tss_init` pour écrire l'adresse réelle du TSS dans les entrées 5 et 6. L'entrée 5 reçoit les 32 bits bas de la base via `gdt_set_entry`. Les 32 bits hauts sont écrits directement dans l'entrée 6 en castant son adresse en `uint32_t *` — le format d'entrée standard ne prévoit pas de champ pour eux. Le GDT est rechargé ensuite pour que le CPU prenne en compte la nouvelle entrée TSS.

## Ce qui est appelé ensuite

Une fois le GDT chargé, `kmain` initialise l'IDT avec [`idt_init`](../interrupts/idt). Le GDT doit précéder l'IDT dans la séquence d'initialisation : les entrées IDT référencent le sélecteur `0x08` (code kernel) dans leur champ `selector`, et ce sélecteur n'est valide qu'une fois le GDT en place. Appeler `lidt` avant `lgdt` produirait un triple fault au premier appel de handler.