# exceptions.md

## Idée générale

Quand le CPU rencontre une erreur (division par zéro, page fault, etc.),
il déclenche une **exception** et saute vers un handler enregistré dans l'IDT.

Ce fichier contient les 32 entrées (isr0 à isr31) qui correspondent aux exceptions x86.

## Le problème du code d'erreur

Intel a décidé que certaines exceptions poussent automatiquement un code d'erreur sur la pile,
et d'autres non.

Pour garder une pile uniforme côté C, on pousse un faux code d'erreur `0` pour les exceptions
qui n'en ont pas - comme ça `isr_handler` reçoit toujours la même structure.

## isr_common

Toutes les entrées sautent vers `isr_common` qui :

- sauvegarde tous les registres avec `pushad`
- pousse manuellement le numéro d'exception, le code d'erreur et l'EIP
- appelle `isr_handler` en C
- nettoie la pile et restaure les registres
- termine avec `iret` qui restaure `EIP`, `CS` et `EFLAGS` automatiquement

L'ordre des arguments est inversé par rapport au code C car la convention x86
lit les arguments dans l'ordre inverse de leur push.