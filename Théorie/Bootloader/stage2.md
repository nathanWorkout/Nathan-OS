# Stage 2 - Bootloader

## Idée générale

Le stage 2 fait tout ce que le stage 1 ne pouvait pas faire en 512 octets :

- activer la ligne A20
- lire le système de fichiers FAT32 pour trouver le kernel
- basculer en mode protégé 32 bits
- parser l'ELF et sauter vers le kernel

## Activation de la ligne A20

Sans A20, le bit 21 des adresses est bloqué à 0, impossible d'accéder au-delà de 1 Mo de mémoire.

On l'active via le port `0x92` en mettant le bit 1 à 1. Ensuite on vérifie que ça a bien marché en comparant deux adresses qui devraient être différentes si A20 est actif. Si ça échoue, on tente via l'interruption BIOS `INT 0x15`.

## Vérification du support LBA et lecture FAT32

Avant de lire le disque, on vérifie que le BIOS supporte les extensions LBA (`INT 0x13 / AH=0x41`). LBA c'est un système d'adressage simple par numéro de secteur, en remplacement de l'ancien système CHS des vieux disques.

On utilise un **DAP** (Disk Address Packet), une structure de 16 octets en mémoire que le BIOS lit pour savoir quoi lire et où le mettre. Le répertoire racine se trouve au secteur LBA 1056 (`32 + 2 × 512`). On parcourt ses entrées (32 octets chacune) jusqu'à trouver `KERNEL  BIN`.

## Chargement du kernel cluster par cluster

Une fois le cluster de départ trouvé, on suit la **chaîne FAT** : on lit l'entrée FAT du cluster courant pour obtenir le suivant, on charge ses 8 secteurs en mémoire, et on répète jusqu'à tomber sur `>= 0x0FFFFFF8` (marqueur de fin). Chaque cluster est chargé à partir de `0x20000`, en avançant de 4096 octets à chaque fois.

## Carte mémoire E820

Avant de quitter le mode réel, on demande au BIOS la carte mémoire via `INT 0x15 / EAX=0xE820`. Elle liste toutes les zones de RAM disponibles, le kernel en aura besoin pour gérer la mémoire.

## Bascule en mode protégé 32 bits

Toutes les lectures disque sont terminées, `INT 0x13` ne fonctionne plus après cette étape.

On charge la **GDT** (Global Descriptor Table) : une table que le CPU lit pour connaître les segments mémoire, leurs permissions et leur niveau de privilège (ring 0 pour le kernel, ring 3 pour le userland). Ensuite on met le bit `PE` de `CR0` à 1 et on fait un far jump qui recharge `CS` avec le sélecteur code kernel (`0x08`).

## Parse ELF et saut vers le kernel

Le kernel est compilé en **ELF32**. On ne peut pas juste sauter au début du fichier, l'ELF contient un header qui décrit où chaque segment doit être placé en mémoire et à quelle adresse commencer l'exécution (`e_entry`).

On lit donc les program headers pour copier chaque segment `PT_LOAD` à son adresse virtuelle, on zéroïse le BSS, puis on saute sur `e_entry` - le kernel prend la main.