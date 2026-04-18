# IO - Ports d'entrée/sortie

## Idée générale

En x86, il existe deux façons de communiquer avec le matériel :

- la RAM - via `mov`, pour les périphériques mappés en mémoire comme le VGA
- les ports I/O - un bus séparé de 65536 ports numérotés de `0x0000` à `0xFFFF`

Chaque périphérique a ses propres ports : le PIC sur `0x20`, le port série sur `0x3F8`, le clavier sur `0x60`...

## outb / inb

`outb` envoie un octet sur un port, `inb` en lit un.

On utilise `al` (moitié basse de `ax`, 8 bits) car la plupart des ports I/O sont des registres 8 bits.
Les instructions assembleur `out` et `in` sont les seuls moyens d'accéder à ces ports - impossible de le faire autrement en C sans inline assembly.

Les deux fonctions sont `static inline` pour éviter le coût d'un appel de fonction,
elles sont appelées très fréquemment (à chaque caractère série, à chaque EOI, etc.).