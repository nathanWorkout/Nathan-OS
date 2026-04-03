# IDT - Interrupt Descriptor Table

## Idée générale

L'IDT est une table de 256 entrées que le CPU consulte quand une interruption se déclenche.
Chaque entrée pointe vers le handler correspondant.

Quand une exception arrive, le CPU lit le numéro d'interruption,
cherche l'entrée dans l'IDT, et saute vers l'adresse du handler.

## Structure d'une entrée

Chaque entrée fait 8 octets et contient :

- l'adresse du handler découpée en `offset_low` et `offset_high`
- le sélecteur de segment code (`0x08` - ring 0 dans la GDT)
- un octet `type_attr` qui décrit le type de gate et le niveau de privilège

## Initialisation

`idt_set_entry` remplit une entrée pour un numéro d'interruption donné.

`idt_init` calcule la taille de la table et charge le registre `IDTR` du CPU via `lidt` -
sans ça le CPU ne sait pas où est la table et planterait à la première interruption.