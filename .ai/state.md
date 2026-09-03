# État du projet

**Dernière mise à jour** : 2026-09-03
**Branche** : `mentor/large-perf-redo` — branche de session mentor : re-coder
l'optimisation du profil LARGE (cache de zones + page map + alloc O(1)).
**À lire en premier** : `.ai/mentor-large-perf.md` — le briefing complet de la session
(diagnostic, design validé par prototype, plan en petits pas, pièges, chiffres).
**Périmètre de la session** : projet **`malloc`** uniquement. KFS-3 a servi de référence
pour quelques choix d'architecture, mais n'est pas au programme ici (cf. roadmap §8).

## Où on en est
Phase 0 non démarrée. Le repo `42_malloc` ne contient que `.git/` et `.ai/`.

Fait :
- Sujet `malloc` dépouillé (16 contraintes obligatoires + 4 bonus)
- Langage tranché : **C**, avec de l'ASM à trois endroits ciblés (cf. `decisions.md` D7)
- Architecture arrêtée : `core/` (algorithme pur) + `port/` + `arch/`, reliés par un
  contrat de 9 fonctions (`ftm_port.h`)
- Chiffrage fait : `n=128`, `m=1024`, `N`/`M` dérivés → 16 384 et 106 496 octets
- Roadmap en 15 phases (`.ai/roadmap.md`)
- 11 décisions consignées (`.ai/decisions.md`)

## Ce qui marche
Rien encore — on démarre.

## Points ouverts
1. **`libft` : existe-t-elle déjà ?** Le sujet exige de la livrer avec son propre Makefile
   si on l'utilise. `core/` n'en a besoin que pour `memcpy`/`memset`, via le port.
2. **Granularité du lock** (phase 10) : mutex global vs arena par thread. À trancher au
   moment du bench, pas avant.
3. **Comportement de `malloc(0)` et `realloc(p, 0)`** : à figer en phase 5 / 7 et à
   consigner dans `decisions.md`.

## Contexte machine
- `~/42_malloc` : ce projet
- Hôte : Linux x86-64 → `HOSTTYPE` = `x86_64_Linux` → `libft_malloc_x86_64_Linux.so`
- `~/kfs-1` : projet KFS-1/2 en C — **référence seule, ne pas modifier**. Deux choses en
  sont reprises dès maintenant :
  - le style de Makefile (fichiers `.S` assemblés via `$(CC) -c`, arborescence `build/`
    miroir des sources, dépendances explicites)
  - `includes/types.h` contient `typedef u32 size_t` — c'est ce qui a motivé D9
    (l'indirection `ftm_stdint.h`), pour que `core/` ne dépende jamais directement d'un
    header système.
