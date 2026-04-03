markdown

# PMM - Physical Memory Manager

## Idée générale

Le PMM est le premier composant de gestion mémoire du kernel.
Son rôle est de savoir quelles pages physiques sont libres et lesquelles sont occupées,
et de pouvoir en allouer ou libérer à la demande.

Sans PMM, impossible d'implémenter la pagination, le heap, ou quoi que ce soit
qui nécessite de la mémoire dynamique.

## La page

La mémoire physique est découpée en **pages** de 4096 octets (4 Ko).
C'est l'unité minimale d'allocation - on n'alloue jamais moins d'une page.

4096 octets c'est une valeur imposée par le CPU x86 pour la pagination.

## Le bitmap

Le PMM représente l'état de chaque page avec un **bitmap** - un tableau de bits
où chaque bit correspond à une page physique.

- bit à `1` - page occupée
- bit à `0` - page libre

On utilise un tableau de `uint32_t` car un registre 32 bits contient 32 bits,
donc 32 pages par case. Pour trouver la case et le bit d'une page :

case  = page / 32   (ou page >> 5)
bit   = page % 32   (ou page & 31)


65536 cases × 32 bits = 2 097 152 pages × 4096 octets = **8 Go adressables**.

## Initialisation

On commence par tout marquer comme occupé (`0xFFFFFFFF` partout).
Ensuite on parcourt la carte mémoire E820 récupérée par le bootloader,
et pour chaque région de type 1 (RAM utilisable) on libère les pages correspondantes.

Enfin on re-marque les pages du kernel comme occupées - le kernel est en mémoire,
on ne peut pas l'écraser. Les symboles `kernel_start` et `kernel_end` sont définis
par le linker script et donnent les adresses exactes du kernel en mémoire.

## Allocation et libération

`pmm_alloc_page` parcourt le bitmap à la recherche d'une case qui n'est pas
entièrement pleine (`!= 0xFFFFFFFF`), trouve le premier bit à 0, le met à 1,
et retourne l'adresse physique correspondante.

`pmm_free_page` fait l'inverse - elle remet le bit à 0 pour rendre la page disponible.