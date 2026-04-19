# Nathan-OS

Un système d'exploitation 32 bits écrit from scratch en C et assembleur (x86 i686).

> Projet personnel — tout est fait à la main, sans libc externe, sans framework.

---

## Screenshots

<!-- Ajoute tes screenshots ici -->
| Shell | Kernel Panic |
|-------|--------------|
| ![Shell](/home/nathan/Images/shell.png) | ![Kernel Panic](/home/nathan/Images/kernel_panic.png) |

---

## Fonctionnalités actuelles

### Bootloader (2 stages)
- **Stage 1**
- **Stage 2**

### Kernel
- **GDT**
- **IDT**
- **Exceptions CPU**
- **PIC 8259**
- **PIT**
- **Driver VGA texte**
- **Port série COM1**

### Mémoire
- **Memory map E820**
- **PMM**
- **Pagination**

### Entrées
- **Clavier PS/2**
- **Ring buffer**

### Shell
- `help`
- `clear`
- `say <texte>`
- `reboot`
- Backspace, couleurs VGA

### Libc minimale
- `strcmp`, `strncmp`

---

## Structure du projet

```
Nathan-OS/
├── boot/
├── kernel/
│   ├── drivers/
│   ├── idt/
│   ├── io/
│   ├── lib/
│   ├── memory/
│   ├── pic/
│   ├── pit/
│   ├── serial/
│   ├── shell/
│   ├── tty/
│   ├── entry.asm
│   └── kernel.c
├── linker.ld
└── Makefile
```

---

## Build & Run

### Dépendances

```bash
# Arch Linux
yay -S i686-elf-gcc nasm qemu-system-x86 grub xorriso mtools
```

### Compiler et lancer

```bash
make clean && make img && make run-img
```

### Debug avec GDB

```bash
make debug
# Dans un autre terminal :
gdb -ex "target remote :1234" build/kernel.bin
```

---

## Toolchain

| Outil | Usage |
|-------|-------|
| `i686-elf-gcc` | Cross-compilateur C 32 bits |
| `nasm` | Assembleur |
| `i686-elf-ld` | Linker |
| `qemu-system-i386` | Émulateur |
| `mkfs.fat` + `mcopy` | Création image FAT32 |


