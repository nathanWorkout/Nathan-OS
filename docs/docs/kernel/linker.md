---
sidebar_position: 2
title: Linker
---

# Linker

## Rôle
Définit l'organisation du kernel en mémoire : adresse de chargement, ordre des sections, et exposition des adresses de début et de fin utilisées par le reste du kernel.

## Fichier source
- `linker.ld`

## Fonctionnement

### Adresses de chargement
Le kernel est mappé en **higher half** :

| | Adresse |
|---|---|
| Virtuelle | `0xFFFFFFFF80000000 + 0x100000` |
| Physique | `0x100000` |

Le kernel réside dans la moitié haute de l'espace d'adressage 64 bits (`0xFFFFFFFF80000000`, les bits de poids fort sont à `1`). Cela permet une séparation stricte entre l'espace utilisateur (adresses basses) et l'espace noyau (adresses hautes) : un processus utilisateur ne peut jamais accéder aux adresses hautes, c'est bloqué au niveau de la pagination. C'est une convention suivie par la majorité des kernels modernes, Linux inclus.

L'offset `+ 0x100000` vient du fait que le premier mégaoctet de la mémoire physique (`0x0` -> `0xFFFFF`) est réservé sur x86 — BIOS, zones mémoire legacy, EBDA. Charger le kernel là-dedans écraserait des structures critiques. Le kernel démarre donc à `0x100000` en physique, et le même offset est appliqué en virtuel pour que l'arithmétique reste simple :

`adresse_physique = adresse_virtuelle - KERNEL_VIRT_BASE`

C'est exactement ce que calculent les symboles `kernel_phys_start` et `kernel_virt_start`.

### Sections

| Section | Contenu |
|---------|---------|
| `.text` | Code exécutable |
| `.rodata` | Données en lecture seule (constantes, chaînes) |
| `.data` | Données initialisées |
| `.tss` | Task State Segment |
| `.bss` | Données non initialisées, mises à zéro au démarrage |

Chaque section est alignée sur **4096 octets** (une page). Cet alignement est requis par la pagination pour assigner des permissions distinctes à chaque région mémoire : exécutable pour `.text`, lecture seule pour `.rodata`, etc. Sans cet alignement, deux sections aux permissions différentes partageraient la même page, ce qui rendrait l'application des permissions impossible.

La `.tss` a sa propre section plutôt que d'être dans `.data` parce que le CPU y accède directement via le GDT, elle doit être isolée et alignée sur sa propre page.

La `.bss` ne stocke aucune donnée dans le binaire : le linker note juste sa taille, et c'est le kernel qui la met à zéro au démarrage. Cela évite d'avoir un binaire inutilement lourd.

### Adresses exportées

Les adresses de début et de fin du kernel sont exposées comme symboles et accessibles directement en C :

```c
extern uint64_t kernel_virt_start;
extern uint64_t kernel_phys_start;
extern uint64_t kernel_virt_end;
extern uint64_t kernel_phys_end;
```

Ces symboles sont utilisés notamment par le **PMM** pour identifier la région mémoire occupée par le kernel et s'assurer qu'elle ne sera pas écrasée lors de l'initialisation de l'allocateur physique.

## Ce qui est appelé ensuite
Une fois le kernel placé en mémoire, Limine saute sur `_start` → [`entry.asm`](../kernel/entry)
