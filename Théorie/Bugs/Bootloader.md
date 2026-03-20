# Bootloader

## Bug #1 — Far jump stage1
Mauvais segment cs dans le far jump, tout le stage2 lisait ses variables au mauvais endroit en mémoire.

## Bug #2 — Répertoire racine FAT32
Confusion segment/offset pour l'adresse 0x9000, le CPU lisait à 0x90000 au lieu de 0x9000.

## Bug #3 — Comparaison noms FAT32
kernel_name lu avec le mauvais segment, "KERNEL  BIN" jamais reconnu malgré qu'il était bien là.

