# Stage 1 — Bootloader (MBR)

## Idée générale

Quand tu allumes un PC, le BIOS lit les 512 premiers octets du disque (le **MBR**) et les charge à l'adresse `0x7C00`.

Si ces 512 octets se terminent par `0xAA55`, il considère ça comme un bootloader valide et lui donne la main.

512 octets c'est trop peu pour un kernel, donc le stage 1 a un seul but : charger le **stage 2** en mémoire et lui passer la main.

> Le BIOS démarre toujours en **real mode** (mode réel 16 bits), hérité du 8086 des années 70, pour rester compatible avec les vieux PC. C'est seulement après que le bootloader peut basculer en 32 ou 64 bits.

## Le BPB (BIOS Parameter Block)

Le standard FAT32 impose une structure à l'offset 3 du boot sector qui décrit la partition :

- taille des secteurs, nombre de FATs, cluster racine...
- sans ça, un OS ne peut pas lire la partition

## Initialisation

- on coupe les interruptions le temps de configurer les registres de segment
- en mode réel 16 bits, une adresse physique = `segment * 16 + offset`
- tout mettre à 0 simplifie les calculs
- la pile est placée dans une zone libre qui n'écrase rien

## Lecture du stage 2

On utilise `INT 0x13` (interruption BIOS) pour lire des secteurs depuis le disque.

- 2 secteurs lus à partir du secteur 3, juste après le BPB
- placés à `0x7E00`, juste après la fin du MBR en mémoire
- si ça échoue, le carry flag est levé et on halt proprement

## Saut vers le stage 2

Far jump manuel vers `0x7E00` — on recharge `CS` à 0 et `IP` à `0x7E00`, ce qui correspond bien à l'adresse où le stage 2 a été chargé.

## Padding et signature

Le MBR doit faire exactement 512 octets.

- le reste est rempli de zéros
- on termine par `0xAA55`, la magic signature que le BIOS vérifie