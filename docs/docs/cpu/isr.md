---
sidebar_position: 9
title: ISR
---

# ISR

## Rôle

Enregistre les stubs assembleur dans l'IDT et définit le handler C appelé par `isr_common` lors d'une exception CPU. C'est le point de jonction entre la mécanique bas niveau des [stubs](./isr_stubs) et le code C du kernel.

## Fichier source

- `isr.c`

## Fonctionnement

### Enregistrement des entrées IDT

```c
void isr_init() {
    static void (*isr_table[32])() = {
        isr0,  isr1,  isr2,  isr3,  isr4,  isr5,  isr6,  isr7,
        isr8,  isr9,  isr10, isr11, isr12, isr13, isr14, isr15,
        isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23,
        isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31
    };

    for (int i = 0; i < 32; i++) {
        idt_set_entry(i, (uint64_t)isr_table[i], 0x08, 0x8e);
    }

    idt_set_entry(0x2C, (uint64_t)irq12, 0x08, 0x8e);
    idt_set_entry(32,   (uint64_t)irq0,  0x08, 0x8e);
    idt_set_entry(33,   (uint64_t)irq1,  0x08, 0x8e);
}
```

`isr_table` est un tableau de pointeurs de fonctions vers les 32 stubs déclarés dans [`isr_stubs.asm`](./isr_stubs). La boucle les enregistre dans les entrées 0 à 31 de l'IDT via [`idt_set_entry`](./idt), avec le sélecteur `0x08` (code kernel ring 0 du [GDT](../memory/gdt)) et les flags `0x8E` (Interrupt Gate 64 bits, DPL 0).

Les IRQ matériels sont enregistrés séparément après la boucle. Les IRQ PIC commencent à l'entrée 32 car les 32 premières sont réservées aux exceptions CPU. `irq12` est enregistré à `0x2C` (44) car c'est le vecteur standard du PIC esclave pour l'IRQ 12 (souris).

### Handler C

```c
void isr_handler(interrupt_frame_t* frame) {
    Canvas cv = fb_get_canvas();
    kernel_panic_init(&cv, frame);
    while(1);
}
```

`isr_handler` reçoit un pointeur vers le sommet de la stack au moment de l'exception, soit la structure `interrupt_frame_t` construite par `isr_common`. Elle contient les registres sauvegardés, le numéro d'exception et l'error code.

Toute exception CPU déclenche actuellement un kernel panic. Le `while(1)` gèle le CPU après l'affichage, ce qui évite un triple fault en sortant d'un handler sans avoir résolu la cause de l'exception.

### La structure interrupt_frame_t

`isr_common` passe `rsp` dans `rdi` juste avant d'appeler `isr_handler`. À ce moment la stack contient, du sommet vers le bas :

```
+------------+ <- rsp = frame
| r15        |
| r14        |
| ...        |
| rax        |
| error_code |
| int_num    |
| rip        |  ← poussé par le CPU
| cs         |
| rflags     |
| rsp        |
| ss         |
+------------+
```

Les cinq valeurs du bas (`rip`, `cs`, `rflags`, `rsp`, `ss`) sont poussées automatiquement par le CPU lors de l'entrée en interruption. Les deux suivantes (`int_num`, `error_code`) sont poussées par le stub. Les 15 registres généraux sont poussés par `isr_common`. La struct doit correspondre exactement à cet ordre, sans padding.

## Ce qui est appelé ensuite

`isr_init` est le dernier appel de la séquence d'initialisation des interruptions. Une fois terminé, l'IDT est opérationnelle et le CPU peut gérer exceptions et IRQ matériels.