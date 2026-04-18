# COM1 - Debug série

## Idée générale

En développement d'OS, on n'a pas de `printf`.

Le port série COM1 permet d'envoyer du texte vers un terminal externe.
QEMU peut le rediriger vers le terminal hôte, ce qui en fait le premier outil de debug disponible.

## Baudrate et DLAB

Le port série communique à une vitesse fixe en bauds - le nombre de bits envoyés par seconde.

Pour configurer le baudrate, il faut d'abord activer le **DLAB** (Divisor Latch Access Bit)
en mettant le bit 7 du registre de ligne (`COM1 + 3`) à 1.
Le DLAB redirige les deux premiers registres vers le diviseur de fréquence.

Le port série oscille à 115 200 Hz. Pour obtenir 38 400 bauds : `115200 / 38400 = 3`.
On envoie ce diviseur en deux octets - octet bas puis octet haut.

Une fois le diviseur configuré, on désactive le DLAB et on configure le format **8N1** :
8 bits de données, pas de parité, 1 bit de stop - le standard série classique.

## Écriture

Avant d'envoyer un octet, on attend que le buffer d'émission soit vide
en lisant le **LSR** (Line Status Register) sur `COM1 + 5`.

Le LSR retourne un octet de flags complet - on isole le bit 5 avec `& 0x20`
car `0x20 = 0b00100000`, ce qui correspond exactement au bit 5.

## Fonctions disponibles

- `serial_print` - envoie une chaîne caractère par caractère
- `serial_println` - idem avec un `\n` à la fin
- `serial_print_hex` - convertit un `uint32_t` en `0xXXXXXXXX`
pour afficher des adresses ou valeurs de registres