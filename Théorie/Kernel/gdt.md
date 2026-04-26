# GDT & TSS — Théorie complète

---

## La GDT (Global Descriptor Table)

### Pourquoi la GDT existe

En mode protégé x86, le CPU ne travaille pas directement avec des adresses physiques pour les segments. Il passe par la GDT — une table en mémoire qui décrit les régions mémoire accessibles, leurs permissions, et leur niveau de privilège.

Quand tu mets une valeur dans `cs`, `ds`, `ss`, tu ne donnes pas une adresse — tu donnes un **sélecteur** qui pointe vers une entrée de la GDT. Le CPU lit cette entrée et sait ce qu'il peut faire.

---

### Structure d'une entrée GDT (8 octets)

Chaque entrée fait exactement 8 octets. La base (adresse de départ) et la limite (taille) sont éclatées en plusieurs morceaux dans ces 8 octets — un héritage du 286 qu'Intel a conservé pour la compatibilité.

```
Offset  Champ           Taille
0       limit_low       16 bits   bits 0-15 de la limite
2       base_low        16 bits   bits 0-15 de la base
4       base_middle      8 bits   bits 16-23 de la base
5       access           8 bits   permissions et type
6       flags_limit      8 bits   bits 16-19 de la limite + flags
7       base_high        8 bits   bits 24-31 de la base
```

---

### L'access byte

C'est le champ le plus important — il encode les permissions du segment bit par bit.

```
Bit 7   P   Present         Doit être 1 pour une entrée valide
Bits 6-5 DPL Privilege Level 00 = ring 0, 11 = ring 3
Bit 4   S   Segment type    1 = code/data, 0 = système (TSS...)
Bit 3   E   Executable      1 = code, 0 = data
Bit 2   DC  Direction       Pour data : sens de croissance
Bit 1   RW  Read/Write      Readable (code) ou Writable (data)
Bit 0   A   Accessed        Mis à 1 par le CPU à l'utilisation
```

**Access bytes utilisés dans Vault-OS :**

| Valeur | Binaire      | Signification         |
|--------|--------------|----------------------|
| `0x9A` | `1001 1010`  | Code ring 0          |
| `0x92` | `1001 0010`  | Data ring 0          |
| `0xFA` | `1111 1010`  | Code ring 3          |
| `0xF2` | `1111 0010`  | Data ring 3          |
| `0x89` | `1000 1001`  | TSS 32 bits          |

La différence entre ring 0 et ring 3 : uniquement les bits DPL (6-5). `0x9A` → `0xFA` c'est `00` → `11` sur ces deux bits. Tout le reste est identique.

---

### Les flags

Contenu dans les 4 bits hauts de `flags_limit` :

```
Bit 7   G   Granularity     1 = limite en pages de 4Ko, 0 = en octets
Bit 6   DB  Operation size  1 = 32 bits, 0 = 16 bits
Bit 5   L   Long mode       0 en 32 bits
Bit 4   AVL Available       Inutilisé, toujours 0
```

Pour les segments code/data 32 bits : `flags = 0xCF` → G=1, DB=1.
Pour le TSS : `flags = 0x00` → pas de granularité page, limite en octets.

---

### Les sélecteurs

Un sélecteur est un index de 16 bits encodé ainsi :

```
Bits 15-3   Index dans la GDT
Bit 2       TI — 0 = GDT, 1 = LDT
Bits 1-0    RPL — Requested Privilege Level
```

Le sélecteur = index × 8. Exemples :

| Index | Sélecteur | Rôle              |
|-------|-----------|-------------------|
| 0     | `0x00`    | Null (obligatoire)|
| 1     | `0x08`    | Code ring 0       |
| 2     | `0x10`    | Data ring 0       |
| 3     | `0x18`    | Code ring 3       |
| 4     | `0x20`    | Data ring 3       |
| 5     | `0x28`    | TSS               |

Le sélecteur n'est jamais stocké dans la struct GDT — c'est le CPU qui le calcule à partir de la position de l'entrée dans le tableau.

---

### Pourquoi le far jump après lgdt

Après `lgdt`, le registre `cs` contient encore l'ancien sélecteur. Le CPU garde en cache interne le descripteur de l'ancien segment de code et continue à l'utiliser — même si la GDT a changé.

Le far jump (`jmp $0x08, $.flush`) est la seule instruction qui force le CPU à recharger `cs` depuis la nouvelle GDT. Sans lui, `cs` pointe sur un descripteur qui n'existe plus.

Après le far jump, les autres registres de segment (`ds`, `es`, `fs`, `gs`, `ss`) peuvent être rechargés normalement avec `mov $0x10, %%ax` suivi de copies dans chaque registre.

---

### GDT de Vault-OS

```
gdt[0]  0x00  Null                — obligatoire, toujours vide
gdt[1]  0x08  Code ring 0  0x9A  — kernel exécute ici
gdt[2]  0x10  Data ring 0  0x92  — kernel lit/écrit ici
gdt[3]  0x18  Code ring 3  0xFA  — userspace exécute ici
gdt[4]  0x20  Data ring 3  0xF2  — userspace lit/écrit ici
gdt[5]  0x28  TSS          0x89  — rempli par tss_init()
```

---

## Le TSS (Task State Segment)

### Pourquoi le TSS existe

Quand une interruption ou exception arrive depuis ring 3, le CPU doit basculer en ring 0 pour la gérer. Mais ring 3 et ring 0 n'ont pas la même pile. Le CPU a besoin de savoir sur quelle pile kernel basculer.

C'est exactement le rôle du TSS : stocker l'adresse de la pile kernel (`esp0` et `ss0`) que le CPU chargera automatiquement lors du passage ring 3 → ring 0.

Sans TSS : la première interruption depuis ring 3 provoque une GPF immédiate.

---

### Ce que le TSS ne fait pas dans Vault-OS

Intel a conçu le TSS pour faire du **task switching hardware** — sauvegarder et restaurer automatiquement tous les registres d'un processus lors d'un switch. C'est lent (sauvegarde des dizaines de champs inutiles) et rigide.

Vault-OS n'utilise pas ce mécanisme. Le context switch est fait **à la main** en assembleur dans `switch.asm` — on sauvegarde uniquement ce dont on a besoin, dans `process_t`. Le TSS ne sert qu'à fournir `esp0` au CPU.

---

### Structure du TSS 32 bits (104 octets minimum)

La structure complète telle que définie par Intel. Les champs en gras sont ceux qu'on utilise réellement.

| Offset | Champ              | Taille  | Rôle                                      |
|--------|--------------------|---------|-------------------------------------------|
| 0      | previous_task_link | 16 bits | Lien vers le TSS précédent (unused)       |
| 2      | reserved0          | 16 bits | —                                         |
| **4**  | **esp0**           | 32 bits | **Pile kernel ring 0 — indispensable**    |
| **8**  | **ss0**            | 16 bits | **Segment pile ring 0 — indispensable**   |
| 10     | reserved1          | 16 bits | —                                         |
| 12     | esp1               | 32 bits | Pile ring 1 (unused)                      |
| 16     | ss1                | 16 bits | —                                         |
| 18     | reserved2          | 16 bits | —                                         |
| 20     | esp2               | 32 bits | Pile ring 2 (unused)                      |
| 24     | ss2                | 16 bits | —                                         |
| 26     | reserved3          | 16 bits | —                                         |
| 28     | cr3                | 32 bits | Page directory (unused — on gère cr3 nous-mêmes) |
| 32     | eip                | 32 bits | Instruction pointer (unused)              |
| 36     | eflags             | 32 bits | Flags (unused)                            |
| 40     | eax                | 32 bits | Registres généraux (unused)               |
| 44     | ecx                | 32 bits | —                                         |
| 48     | edx                | 32 bits | —                                         |
| 52     | ebx                | 32 bits | —                                         |
| 56     | esp                | 32 bits | —                                         |
| 60     | ebp                | 32 bits | —                                         |
| 64     | esi                | 32 bits | —                                         |
| 68     | edi                | 32 bits | —                                         |
| 72     | es                 | 16 bits | Registres de segment (unused)             |
| 76     | cs                 | 16 bits | —                                         |
| 80     | ss                 | 16 bits | —                                         |
| 84     | ds                 | 16 bits | —                                         |
| 88     | fs                 | 16 bits | —                                         |
| 92     | gs                 | 16 bits | —                                         |
| 96     | ldt                | 16 bits | Sélecteur LDT (unused)                    |
| 100    | reserved11         | 16 bits | —                                         |
| **102**| **io_map_base**    | 16 bits | **Offset du bitmap I/O — doit être >= 104** |

La struct doit être `__attribute__((packed))` — sans ça, GCC ajoute du padding et les offsets ne correspondent plus à ce qu'Intel attend.

---

### Initialisation du TSS dans Vault-OS

Trois opérations sont nécessaires :

**1. Tout mettre à zéro**
```
tss = (tss_t){0}
```
Tous les champs inutilisés à zéro — comportement défini et prévisible.

**2. Remplir ss0 et io_map_base**
- `ss0 = 0x10` — sélecteur data ring 0, la pile kernel utilise ce segment
- `io_map_base = sizeof(tss)` — dit au CPU qu'il n'y a pas de bitmap I/O. Si 0, le CPU cherche le bitmap au début du TSS et lit des données aléatoires

**3. Remplir l'entrée GDT et charger TR**
- `gdt_set_tss_entry(base, limit)` — remplit gdt[5] avec l'adresse physique et la taille du TSS
- `ltr 0x28` — charge le Task Register. Le CPU ne sait pas quel TSS utiliser tant que `ltr` n'est pas exécuté. Sans ça, le TSS existe en mémoire mais le CPU ne le trouve pas

---

### tss_set_kernel_stack()

À chaque context switch, la pile kernel change — chaque processus a sa propre pile kernel. Il faut mettre à jour `esp0` dans le TSS pour que le CPU bascule sur la bonne pile lors de la prochaine interruption depuis ring 3.

C'est le rôle de `tss_set_kernel_stack(uint32_t esp0)` — une fonction que le scheduler appellera à chaque switch.

---

### Résumé du flux complet ring 3 → ring 0

```
1. Processus tourne en ring 3
2. Interruption arrive (timer / syscall / page fault...)
3. CPU consulte le registre TR → trouve le TSS à 0x28
4. CPU lit tss.esp0 et tss.ss0
5. CPU bascule sur la pile kernel (esp0)
6. CPU sauvegarde cs, eip, eflags, ss_user, esp_user sur la pile kernel
7. Handler s'exécute en ring 0
8. iret → restaure cs, eip, eflags, reprend ring 3
```

Sans TSS correctement initialisé, l'étape 4 échoue → GPF → double fault → triple fault → reboot silencieux.
