# PIC 8259 - Programmable Interrupt Controller

## Idée générale

Le PIC est le chip qui reçoit les signaux des périphériques et les transmet au CPU sous forme d'interruptions.
Sans PIC, le CPU ne saurait jamais qu'une touche a été pressée ou que le timer a tick.

Il y en a deux en cascade :

- **PIC maître** - gère IRQ0 à IRQ7, connecté directement au CPU
- **PIC esclave** - gère IRQ8 à IRQ15, connecté sur la broche IRQ2 du maître

Le CPU ne voit qu'un seul PIC mais obtient 15 lignes d'interruption au total.

## Pourquoi remappe-t-on le PIC

Par défaut le PIC envoie IRQ0-7 sur les vecteurs 8-15 - exactement là où le CPU
place ses propres exceptions (`#DF`, `#GP`, `#PF`...).

Le timer (IRQ0) déclencherait le vecteur 8 qui correspond au double fault,
ce qui provoquerait un triple fault et un reboot silencieux.

On remappe donc :

- IRQ0-7 vers les vecteurs `0x20` à `0x27` (32-39)
- IRQ8-15 vers les vecteurs `0x28` à `0x2F` (40-47)

## La séquence d'initialisation ICW

Le PIC s'initialise en lui envoyant 4 commandes dans l'ordre strict : ICW1, ICW2, ICW3, ICW4.

- **ICW1** - démarre l'initialisation et indique qu'on va envoyer ICW4
- **ICW2** - définit l'offset de remapping (`0x20` pour le maître, `0x28` pour l'esclave)
- **ICW3** - configure la cascade : le maître sait que l'esclave est sur IRQ2, l'esclave sait qu'il est esclave
- **ICW4** - active le mode 8086

Entre chaque commande, on envoie un octet sur le port `0x80` comme délai pour laisser le temps au PIC de traiter.

## EOI - End Of Interrupt

Après chaque handler IRQ, il faut envoyer un EOI au PIC pour lui signaler que l'interruption est traitée.
Sans ça, le PIC reste bloqué et n'envoie plus aucune IRQ.

Si l'IRQ vient de l'esclave (IRQ >= 8), il faut envoyer l'EOI aux deux PICs -
d'abord l'esclave, ensuite le maître, car le maître attend aussi un acquittement.

## Masques IRQ

Chaque PIC possède un registre de masque 8 bits - un bit par IRQ.

- bit à `1` - l'IRQ est ignorée
- bit à `0` - l'IRQ est active

`pic_set_mask` met le bit à 1 avec un OR, `pic_clear_mask` le remet à 0 avec un AND NOT.
On initialise tous les masques à `0xFF` pour tout ignorer au démarrage,
puis on active les IRQ une par une au fur et à mesure qu'on implémente leurs handlers.