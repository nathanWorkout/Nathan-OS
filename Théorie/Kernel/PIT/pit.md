# PIT - Programmable Interval Timer

## Idée générale

Le PIT est un circuit intégré sur la carte mère qui génère des interruptions à intervalle régulier.
C'est lui qui donne au kernel un sens du temps - sans PIT, impossible de faire un scheduler, un sleep, ou de mesurer quoi que ce soit.

Il déclenche IRQ0 à chaque tick, ce qui appelle `irq0_handler`.

## La fréquence de base

Le PIT oscille à une fréquence fixe de **1 193 182 Hz** - une valeur historique
héritée de la fréquence d'horloge du IBM PC original.

On ne peut pas la changer directement, mais on peut lui donner un **diviseur** :
le PIT déclenchera une IRQ tous les `diviseur` cycles.

diviseur = 1193182 / fréquence souhaitée

Si on veut 100 Hz (100 ticks par seconde) : `1193182 / 100 = 11931`.

## Configuration

Le PIT se configure via le port `0x43` (registre de commande) avec l'octet `0x36` :

- canal 0 - celui connecté à IRQ0
- mode 3 - square wave, le PIT recharge automatiquement le diviseur à chaque tick
- accès 16 bits - on envoie d'abord l'octet bas du diviseur, puis l'octet haut
- format binaire

Le diviseur est ensuite envoyé en deux fois sur le port `0x40` (canal 0) :
d'abord les 8 bits bas, puis les 8 bits hauts.