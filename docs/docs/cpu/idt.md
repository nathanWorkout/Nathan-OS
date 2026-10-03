---
sidebar_position: 7
title: IDT
---

# IDT

## Rôle

Initialise l'Interrupt Descriptor Table : la table que le CPU consulte à chaque interruption ou exception pour savoir quel handler exécuter. Sans IDT valide, toute exception, division par zéro, page fault, instruction illégalem provoque un triple fault et reboot la machine.

## Fichier source

- `idt.c`

## Fonctionnement

### Structure d'une entrée

```c
typedef struct __attribute__((packed)) {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} idt_entry_t;
```

Chaque entrée fait 16 octets. L'adresse du handler est découpée en trois champs (`offset_low`, `offset_mid`, `offset_high`) éparpillés dans la struct, même héritage x86 que pour le GDT. `__attribute__((packed))` est obligatoire pour la même raison.

`ist` et `zero` sont toujours à 0 dans cette implémentation mais doivent être présents, leur position est fixée par la convention Intel et le CPU les lit inconditionnellement.

### Remplissage d'une entrée

```c
void idt_set_entry(uint8_t num, uint64_t base, uint16_t selector, uint8_t flags) {
    idt[num].offset_low  = base & 0xFFFF;
    idt[num].offset_mid  = (base >> 16) & 0xFFFF;
    idt[num].offset_high = (base >> 32) & 0xFFFFFFFF;
    idt[num].selector    = selector;
    idt[num].ist         = 0;
    idt[num].type_attr   = flags;
    idt[num].zero        = 0;
}
```

`base` est l'adresse du stub assembleur à exécuter lors de l'interruption. Elle est découpée en trois morceaux et placée dans les champs correspondants. `selector` est le sélecteur de segment depuis lequel le handler s'exécutera, `0x08` partout, soit le segment code kernel ring 0 défini dans le [GDT](../memory/gdt). `flags` encode le type de gate et le privilege level requis pour déclencher l'interruption.

Les entrées sont remplies depuis `isr_init` dans [`isr.c`](./isr) et non depuis `idt_init`, `idt_init` se contente de préparer la table vide et de la charger.

### Chargement de l'IDTR

```c
void idt_init() {
    idtr.limit = sizeof(idt_entry_t) * 256 - 1;
    idtr.base  = (uint64_t)&idt;

    asm volatile("lidt %0" : : "m"(idtr));
}
```

`idtr.limit` vaut `sizeof(idt_entry_t) * 256 - 1` : la taille totale de la table en octets moins 1, convention Intel identique au GDTR. `idtr.base` est l'adresse de la table en mémoire.

`lidt` charge ces deux valeurs dans le registre IDTR. Contrairement au GDT, recharger l'IDT ne nécessite pas de far return : aucun registre de segment n'est lié à l'IDT, le CPU se contente de mémoriser où se trouve la table. Les entrées peuvent être modifiées après le `lidt`, c'est ce que fait `isr_init` juste après.

À ce stade la table contient 256 entrées nulles. Une interruption déclenchée avant que `isr_init` ait rempli les entrées provoquerait un triple fault.

### L'octet type_attr

Toutes les entrées sont enregistrées avec `flags = 0x8E` :

```
0x8E = 1000 1110
       ||||||||
       |||||||+-- \
       ||||||+---  | type : 1110 = Interrupt Gate 64 bits
       |||||+----  |        (désactive les interruptions pendant l'exécution du handler)
       ||||+-----/
       |||+------ réservé, toujours 0
       ||+------- DPL bit 0 ─┐ privilege level minimum pour déclencher via int :
       |+-------- DPL bit 1  ┘ 00 = seul le kernel peut déclencher, 11 = user aussi
       +--------- présent, doit être à 1 sinon le CPU considère l'entrée invalide
```

Le type `1110` (Interrupt Gate) a une propriété importante : le CPU clear automatiquement le flag `IF` (Interrupt Flag) en entrant dans le handler, ce qui désactive les interruptions matérielles pendant son exécution. Elles sont réactivées à la sortie par `iret`. Utiliser une Trap Gate (`1111`) laisserait les interruptions actives et permettrait à un IRQ d'interrompre le handler en cours, comportement non voulu ici.

Le DPL à `00` signifie que seul le kernel peut déclencher ces interruptions via l'instruction `int`. Les exceptions CPU ignorent le DPL et peuvent toujours se déclencher depuis n'importe quel ring.

## Ce qui est appelé ensuite

Une fois `idt_init` terminé, `kmain` appelle `isr_init` dans [`isr.c`](./isr) qui remplit les 256 entrées avec les adresses des stubs définis dans [`isr_stubs.asm`](./isr_stubs). C'est seulement à ce moment que l'IDT devient opérationnelle.