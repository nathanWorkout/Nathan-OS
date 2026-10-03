---
sidebar_position: 1
title: Introduction
---

# VaultOs

**Système d'exploitation 64 bits écrit from scratch — Tout casser sans jamais briser.**

---

## Concept

VaultOs est un système d'exploitation 64 bits construit de zéro, sans libc externe, sans framework.

Son principe central : **un noyau immuable que rien ne peut corrompre, entouré d'environnements utilisateur que l'on peut casser librement.**

Peu importe ce qui se passe dans l'espace utilisateur — mauvaise config, driver planté, environnement graphique cassé — le Core reste intact et le système reste toujours récupérable.

> **Expérimente sans jamais casser ton OS.**

---

## Architecture

VaultOs adopte une architecture inspirée des noyaux monolithiques (Windows NT), avec une séparation stricte entre le Core immuable et les espaces utilisateur isolés par profil.

### Mémoire

| Composant | Rôle |
|-----------|------|
| **PMM** | Allocateur physique bitmap, bootstrap |
| **VMM** | Gestion de la mémoire virtuelle, pagination 64 bits |
| **Heap** | Allocateur kernel |

### Interruptions & CPU

| Composant | Rôle |
|-----------|------|
| **GDT** | Segments noyau / utilisateur |
| **IDT** | 256 entrées, ISR stubs, gestion des exceptions CPU |
| **TSS** | Stack kernel mise à jour à chaque context switch |
| **PIC** | Remappé, IRQs gérés proprement |

### Graphique

- Framebuffer via Limine
- Primitives de dessin 
- **SSAA** et **SDF** fonctionnels
- Chargeur de fond d'écran avec parsing PNG *(en cours)*

### Shell & TTY

- TTY fonctionnel avec couleurs
- Shell interactif intégré au noyau

---

## VaultFs

VaultFs est le système de fichiers conçu spécifiquement pour VaultOs. Aucun filesystem existant ne proposant d'isolation native par profil, il a été inventé from scratch pour incarner le principe central de l'OS.

### Modèle 3 couches

```
Layer 0 (Core)     : Lecture seule, partagé entre tous les profils
Layer 1 (Shared)   : Fichiers publiés par les profils, visibles de tous
Layer 2 (Private)  : Espace privé par profil, modifiable librement
```


La résolution d'un chemin cherche d'abord dans le Layer 2 du profil courant, puis Layer 1, puis Layer 0. Un nœud marqué `VAULT_DELETED` dans le Layer 2 masque les couches inférieures — le fichier d'origine n'est jamais touché.

Concrètement : deux profils peuvent avoir des dotfiles complètement différents pour le même programme. C'est la différence fondamentale avec Linux.

Pour aller plus loin : [VaultFs en détail](vaultfs/overview)

---

## Build & Run

```bash
git clone https://github.com/nathanWorkout/Nathan-OS
cd Nathan-OS

make full   # Compiler et lancer QEMU
make img    # Créer uniquement l'image
```

Pour les prérequis et la configuration : [Build](build/makefile)
