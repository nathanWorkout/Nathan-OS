---
sidebar_position: 5
title: Limine Requests
---

# Limine Requests

## Rôle
Déclare les requêtes adressées à Limine au démarrage. Limine lit ces structures avant de sauter sur `_start` et y écrit ses réponses. C'est le seul moyen de récupérer des informations sur l'environnement matériel depuis le bootloader.

## Fichier source
- `limine_request.c`

> Ce fichier évolue constamment. Pour la version exacte et à jour, consulte directement le [repo GitHub](https://github.com/nathanWorkout/Nathan-OS).

## Fonctionnement

Chaque requête est une struct placée dans la section `.requests` via `__attribute__((used, section(".requests")))`. L'attribut `used` empêche le compilateur de supprimer la variable (elle n'est jamais appelée explicitement). Limine scanne cette section au boot, reconnaît les structs par leur `.id`, et remplit le champ `.response` de chacune.

## Requêtes déclarées

### Base Revision
```c
LIMINE_BASE_REVISION(3);
```
Indique à Limine la version du protocole attendue. Si Limine ne supporte pas cette révision, il refuse de booter. Doit être déclarée en premier.

### Memory Map
Demande la carte mémoire complète de la machine. Utilisée par le **PMM** pour connaître les régions disponibles et éviter d'allouer sur des zones réservées (BIOS, ACPI, kernel, framebuffer...).

### HHDM
Demande l'offset du **Higher Half Direct Map** : Limine mappe toute la mémoire physique en virtuel à une adresse haute fixe. Cet offset permet de convertir une adresse physique en adresse virtuelle accessible :
```c
virt = phys + hhdm_request.response->offset;
```

### Kernel Address
Fournit les adresses de base virtuelle et physique où Limine a chargé le kernel. Utilisé par le VMM pour calculer les conversions `virt / phys` sur les symboles du kernel.

### Module
Demande les modules chargés par Limine aux côtés du kernel. Utilisé pour récupérer le **fond d'écran** (PNG) passé en tant que module Limine.

## Ce qui est appelé ensuite
Les réponses sont consommées dès `kmain` : [`pmm_init`](../memory/pmm) lit `memmap_request.response`, [`vmm_create_kernel_space`](../memory/vmm) lit `kernel_address_request.response`.
