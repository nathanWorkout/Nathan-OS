---
sidebar_position: 3
title: Entry
---

# Entry

## Rôle
Point d'entrée du kernel : initialise les segments, met en place la stack kernel, puis transfère l'exécution vers le code C.

## Fichier source
- `entry.asm`

## Fonctionnement

### Segments
Les registres de segment sont configurés manuellement au démarrage :

| Registre | Valeur | Raison |
|----------|--------|--------|
| `ss`, `ds`, `es` | `0x10` | Segment data du GDT (index 2) |
| `fs`, `gs` | `0x00` | Non utilisés au démarrage, mis à zéro |

`0x10` correspond à l'entrée data du GDT mise en place par Limine. Sans cette initialisation explicite, les registres de segment pourraient contenir des valeurs héritées du bootloader.

### Stack kernel
Une stack de **16 Ko** est réservée statiquement en `.bss` :

```nasm
section .bss
align 16
kernel_stack_bottom:
    resb 16384
kernel_stack_top:
```

`rsp` est pointé sur `kernel_stack_top`, la stack x86 croît vers le bas, donc on part du sommet. Elle est ensuite alignée sur **16 octets** :

```nasm
and rsp, ~0xF
```

Cet alignement est imposé par l'ABI System V x86-64 : toute instruction `call` attend une stack alignée sur 16 octets au moment de l'appel. Le non-respect de cette contrainte provoque des comportements indéfinis sur les instructions SSE.

### Appel de kmain
`rbp` est mis à zéro avant l'appel :

```nasm
xor rbp, rbp
```

Cela marque le bas de la call stack, les outils de debug (stack unwinder) s'arrêtent quand ils rencontrent `rbp = 0`. Puis `kmain` est appelé :

```nasm
call kmain
```

### Boucle de halt
Si `kmain` retourne (ce qui ne devrait jamais arriver), le CPU est gelé proprement :

```nasm
cli
.halt:
    hlt
    jmp .halt
```

`cli` coupe les interruptions pour éviter toute reprise d'exécution. `hlt` suspend le CPU jusqu'à la prochaine interruption, comme elles sont désactivées, le CPU reste suspendu indéfiniment. Le `jmp .halt` est une sécurité au cas où une interruption non masquable (NMI) réveille le CPU malgré tout.

## Ce qui est appelé ensuite
Une fois la stack en place, `entry.asm` saute dans [`kmain`](../kernel/kmain)
