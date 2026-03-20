# Interrupt Descriptor Table (IDT)

L'**Interrupt Descriptor Table (IDT)** est une **structure de données binaire** utilisée sur les architectures **x86 (IA‑32 et x86‑64)** pour dire au processeur *où se trouvent les routines de gestion d'interruption (ISR)*. C'est l'équivalent en mode protégé/long mode de la table d'interruptions en mode réel.

---

## À quoi ça sert

Quand une interruption se produit (ex : clavier, timer, erreur CPU…), le processeur :
1. Trouve dans l'IDT l'entrée correspondant au numéro d'interruption.
2. Charge l'adresse de la routine qui doit gérer cette interruption.
3. Exécute cette routine (ISR).

---

## Le registre IDTR

- Le **registre IDTR** contient :
  - **Offset** : l'adresse mémoire de l'IDT.
  - **Size** : taille de la table - 1.
- Il est chargé avec l'instruction **`LIDT`**.

---

## Taille et emplacement

- L'IDT peut avoir **jusqu'à 256 entrées** (vecteurs 0…255).
- Chaque entrée correspond à un type d'interruption ou d'exception.
- Si une entrée n'existe pas, une **General Protection Fault** sera déclenchée.

---

## Structure des entrées

### En mode 32 bits (IA-32)

- **Taille d'une entrée : 8 octets**
- Contient :
  - **Offset** : adresse de l'ISR.
  - **Selector** : sélecteur de segment (doit pointer vers un segment de code valide).
  - **Type et attributs** (DPL, P, type de gate).
- Types de gates valides :
  - **Interrupt Gate** (32 bits)
  - **Trap Gate** (32 bits)
  - **Task Gate** (rare)

### En mode 64 bits (x86-64)

- **Taille d'une entrée : 16 octets**
- Contient les mêmes informations, avec :
  - Un **offset 64 bits**.
  - Un champ **IST** (Interrupt Stack Table).
- Gate types :
  - **64-bit Interrupt Gate**
  - **64-bit Trap Gate**

---

## Différences entre gates

| Type de gate       | Quand l'utiliser            | Comportement principal                                 |
|--------------------|-----------------------------|--------------------------------------------------------|
| **Interrupt Gate** | IRQ ou interruption normale | Désactive les interruptions pendant l'exécution (IF=0) |
| **Trap Gate**      | Exceptions CPU              | Autorise d'autres interruptions (IF conservé)          |
| **Task Gate**      | IA-32 uniquement            | Change de tâche matériellement (rare)                  |

---

## Les 3 catégories de vecteurs

### Exceptions CPU (vecteurs 0x00..0x1F)

Déclenchées par le CPU lui-même quand il détecte un problème.

| Vecteur | Nom | Error code | Cause |
|---------|-----|-----------|-------|
| 0x00 | #DE | non | division par zéro |
| 0x06 | #UD | non | instruction inconnue |
| 0x08 | #DF | oui (= 0) | exception dans une exception |
| 0x0D | #GP | oui | violation de protection |
| 0x0E | #PF | oui | page absente, adresse fautive dans CR2 |

Les exceptions **avec error code** : le CPU pousse un mot de 32 bits
supplémentaire avant de sauter au handler. Il faut en tenir compte
dans la structure `registers_t` pour que tout soit bien aligné.

### IRQ hardware (vecteurs 0x20..0x2F après remap PIC)

Déclenchées par les périphériques (timer, clavier, souris...).

**Obligatoire** : envoyer un EOI (End Of Interrupt) au PIC à la fin
du handler, sinon le PIC ne renverra plus jamais d'IRQ.

| Vecteur | IRQ | Périphérique |
|---------|-----|-------------|
| 0x20 | IRQ0 | timer PIT |
| 0x21 | IRQ1 | clavier PS/2 |
| 0x2C | IRQ12 | souris PS/2 |
| 0x2E | IRQ14 | disque ATA |

### Syscalls (vecteur 0x80)

Pas encore — ce sera pour le ring 3 / userland (phase 7 de ta roadmap).
La seule différence : type_attr = `0xEE` (trap gate DPL=3) au lieu de `0x8E`.

---

## Pourquoi remappe-t-on le PIC

Par défaut le PIC envoie IRQ0..IRQ7 sur les vecteurs 8..15.
Or les vecteurs 8..15 sont des exceptions CPU (#DF, #TS, #NP...).
Si IRQ0 (timer) déclenche le vecteur 8 (#DF), le CPU croit à un double fault
et provoque un triple fault → reboot silencieux.

Le remap déplace IRQ0-7 vers les vecteurs 32-39 et IRQ8-15 vers 40-47,
loin des exceptions CPU.

**Le remap PIC doit être fait AVANT idt_init().**

---

## EOI — End Of Interrupt

Le PIC 8259 a deux puces en cascade :
- PIC maître sur le port `0x20`
- PIC esclave sur le port `0xA0`

À la fin de chaque handler IRQ, il faut envoyer EOI :
- IRQ 0..7 (maître) : `outb(0x20, 0x20)`
- IRQ 8..15 (esclave) : `outb(0xA0, 0x20)` puis `outb(0x20, 0x20)`

Sans EOI le PIC est bloqué et n'envoie plus d'IRQ.

---

## Pourquoi de l'assembleur pour les stubs

Le C ne peut pas sauvegarder/restaurer les registres de façon fiable
pour une interruption. Le compilateur réorganise les registres librement.

Il faut donc écrire à la main en assembleur le prologue/épilogue de chaque
handler. On utilise des **macros NASM** pour éviter de répéter 38 fois le
même code — une macro pour les exceptions sans error code, une pour celles
avec, une pour les IRQ.

---

## Ce que fait isr_common_stub

C'est le stub générique appelé par tous les handlers. Il :
1. Sauvegarde tous les registres généraux (`pushad`)
2. Sauvegarde `ds`
3. Charge le segment de données kernel (0x10)
4. Passe un pointeur sur la pile (`registers_t*`) au dispatcher C
5. Appelle `isr_handler()`
6. Restaure `ds` et les registres généraux (`popad`)
7. Nettoie `int_no` et `error_code` de la pile
8. Fait `iret` pour retourner là où le CPU était

---

## Structure registers_t

C'est la photo complète de l'état du CPU au moment de l'interruption.
Elle est construite par les push successifs des stubs + du CPU lui-même.

```
[ ss          ]  <- poussé par le CPU (changement de ring seulement)
[ esp         ]  <- poussé par le CPU (changement de ring seulement)
[ eflags      ]  <- poussé par le CPU toujours
[ cs          ]  <- poussé par le CPU toujours
[ eip         ]  <- poussé par le CPU toujours
[ error_code  ]  <- poussé par le CPU (si exception avec error code)
                    ou 0 dummy (poussé par le stub)
[ int_no      ]  <- poussé par le stub
[ eax..edi    ]  <- poussé par pushad dans isr_common_stub
[ ds          ]  <- poussé manuellement dans isr_common_stub
  ^
  esp pointe ici -> registers_t*
```

---

## Étapes d'implémentation

### 1. Écrire pic_init() dans drivers/pic.c

Envoie les 4 commandes ICW au PIC maître (ports 0x20/0x21) et esclave
(ports 0xA0/0xA1) pour remappe les IRQ sur les vecteurs 32-47.
Masque toutes les IRQ sauf IRQ0 (timer) pour commencer proprement.
Écris aussi pic_eoi(uint8_t irq) qui envoie EOI au bon PIC selon le numéro d'IRQ.

Vérifie : le Makefile compile drivers/pic.c correctement.

### 2. Écrire isr_stubs.asm dans kernel/

Définis les 3 macros NASM : ISR_NO_ERR, ISR_ERR, IRQ.
Écris isr_common_stub qui sauvegarde les registres, appelle isr_handler, restaure.
Instancie les 22 stubs d'exceptions et les 16 stubs IRQ avec les macros.

Vérifie : nasm compile sans erreur, les symboles isr0..isr21 et irq0..irq15
sont bien dans le .o (utilise nm build/isr_stubs.o).

### 3. Écrire idt_set_entry() et idt_init() dans kernel/idt.c

Déclare le tableau statique de 256 InterruptDescriptor32 et l'IDTR.
idt_set_entry() découpe l'adresse 32 bits en offset_1/offset_2.
idt_init() appelle idt_set_entry() pour chaque vecteur avec le bon type_attr
(0x8E pour tout sauf 0x80 syscall plus tard), puis charge lidt.

Vérifie : GDB -> après lidt, x/8hx adresse_idt doit montrer les bonnes valeurs.

### 4. Écrire isr_handler() dans kernel/idt.c

Switch sur r->int_no. Pour chaque exception : cli+hlt pour l'instant
(tu ajouteras printk plus tard). Pour chaque IRQ : appelle pic_eoi().
Pour IRQ0 timer : incrémente un compteur ticks. Pour IRQ1 clavier : lit port 0x60.

### 5. Appeler dans kmain dans le bon ordre

pic_init() -> idt_init() -> sti().
Teste avec une division par zéro : sans IDT reboot silencieux, avec IDT hlt propre.

### 6. Mettre à jour le Makefile

Vérifie que isr_stubs.asm est bien compilé (il est dans kernel/ donc
le wildcard ASM_SRCS le trouve automatiquement, mais entry.asm est exclu
explicitement — isr_stubs.asm ne doit pas l'être).
