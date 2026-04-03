# ISR - Handlers & Kernel Panic

## Idée générale

Quand le CPU déclenche une exception, il saute vers le handler enregistré dans l'IDT.
Sans handler, le CPU ne sait pas quoi faire et reboot silencieusement - impossible à débugger.

`isr_init` enregistre les 32 handlers d'exceptions dans l'IDT, plus `irq0` en entrée 32 pour le timer.
Chaque handler assembleur saute vers `isr_handler` qui gère l'affichage et stoppe le système.

## ISR - Interrupt Service Routine

Quand une exception se déclenche, le CPU pousse automatiquement sur la pile :

- `EIP` - l'adresse de l'instruction fautive
- `CS` - le segment de code
- `EFLAGS` - l'état des flags au moment de l'exception
- un code d'erreur pour certaines exceptions seulement

`isr_handler` reçoit trois arguments : le numéro de l'exception, le code d'erreur et l'EIP.
Ces infos sont suffisantes pour savoir ce qui s'est passé et où.

Les exceptions x86 couvrent tous les cas d'erreur matériel et logiciel :
division par zéro, opcode invalide, page fault, general protection fault, double fault...
Chacune a un numéro fixé par Intel de 0 à 31.

## La mémoire VGA

En mode protégé, on n'a pas de `printf`. Le seul moyen d'afficher quelque chose
est d'écrire directement dans la mémoire vidéo VGA mappée à `0xB8000`.

C'est un tableau de 80 × 25 cases, chaque case faisant 2 octets :

- octet haut - la couleur (`fond << 4 | texte`)
- octet bas - le caractère ASCII

0x1f00 | 'A'  ->  fond bleu, texte blanc, caractère 'A'

La palette est celle des 16 couleurs CGA, héritée des années 80.
Le curseur hardware est désactivé via les ports `0x3D4`/`0x3D5`
pour éviter qu'il s'affiche par-dessus l'écran.

## Pourquoi un error screen

Sans écran de kernel panic, une exception provoque un reboot silencieux.
On ne sait pas quelle exception s'est déclenchée, ni à quelle adresse, ni pourquoi.

L'écran affiche le numéro de l'exception, son nom lisible, et l'EIP -
ce qui permet de retrouver exactement l'instruction fautive dans le code.

Une fois affiché, on boucle indéfiniment avec `while(1)`.
Le kernel est mort, il n'y a rien à récupérer.