# État du projet

**Dernière mise à jour** : 2026-09-04
**Branche** : `master`
**Session en cours** : re-codage de l'optimisation du profil LARGE en mode
`collab-pairing-mentor`. Briefing complet : `.ai/mentor-large-perf.md` (à lire en premier).

## Projet de base : TERMINÉ
Roadmap 15/15, décisions D1→D27. 16 tests unitaires verts (fuzz 200k ops compris),
3 bonus validés (thread-safe, env vars debug, show_alloc_mem_ex), LD_PRELOAD OK sur
ls / bash / vim / git / python3. Tuning `m=2048` retenu (D27).

## Session LARGE — baseline mesurée (2026-09-04, machine de Charles)

Config : `./configure.sh --cflags="-DFTM_SMALL_MAX=2048"`, OPTIMIZE=y, page 4 KB, x86_64.

| mesure                                        | ns/op   |
|-----------------------------------------------|--------:|
| glibc `large`                                 |    56.9 |
| glibc `large` + `MALLOC_MMAP_THRESHOLD_=1`    |  3737.3 |
| **ft_malloc `large` (baseline)**              | **3119.9** |
| ft_malloc `small` (témoin, doit rester stable)|   176.3 |
| ft_malloc `mixed` (témoin, doit rester stable)|   201.9 |

**Le diagnostic est validé sur cette machine** : ft_malloc (3120) est **1,2× plus rapide**
que la glibc forcée dans le même régime mmap (3737). L'écart ×55 avec la glibc par défaut
mesure sa stratégie (pas de mmap sous 128 KB), pas la qualité de notre implémentation.

**Cible de session** : `large` sous 400 ns/op, `small`/`mixed` inchangés.
**ATTEINTE** : `large` = 240 ns/op (×14,2), témoins neutres. Cf. D31.

## Avancement de la session (plan du briefing)
- [x] Pas 1 — baseline chiffrée
- [x] Pas 2 — A : alloc LARGE en O(1) — codé, mais **régression isolée** (cf. D28), à ré-évaluer en fin de session
- [x] Pas 3 — types + constantes du cache
- [x] Pas 4 — `ftm_large_cache.c` + câblage heap (×3,3 sur la baseline, cf. D29)
- [x] Pas 5 — `ftm_zone_map.c` + câblage `ftm_heap_find_zone` (**240 ns/op, ×14,2**, cf. D31)
- [x] Pas 6 — `tests/test_large_cache.c` (4 scénarios : réutilisation, éviction, map, fuite)
- [x] Pas 7 — validation finale : **16/16 tests**, LD_PRELOAD ls/python3/git OK, 3 profils benchés

## Notes de session
- `Makefile.cfg` traînait un `-DFTM_LARGE_CACHE_MAX=32` hérité d'une session
  exploratoire (branche `large_opti`) sans code correspondant sur master → purgé au
  reconfigure du pas 1.
- `make re` ne reconstruit pas `bench_alloc` (hors `fclean`) : penser à
  `make bench_alloc` après une modif du bench.
- Branche de référence (prototype, corrigé de review) : `claude/malloc-performance-glibc-y5y5lz`,
  commit `e74e110`. Ne pas la coller à Charles — usage review uniquement.

## Session LARGE — TERMINÉE (2026-09-04)

Résultat : `large` **3418 → 218-240 ns/op (×14-15)**, témoins `small`/`mixed` neutres.
Les trois profils sont maintenant tous autour de 200 ns/op. Détail chiffré et
justification : D28 → D31 dans `decisions.md`.

Fichiers ajoutés : `src/core/ftm_large_cache.c`, `src/core/ftm_zone_map.c`,
`tests/test_large_cache.c`. Modifiés : `ftm_config.h`, `ftm_types.h`, `ftm_internal.h`,
`ftm_heap.c`.

### À faire pour clôturer
1. `git add` des 3 nouveaux fichiers + commit (ils sont encore untracked).
2. Correctif structurel de `run_tests.sh` (cf. D30) : itérer sur les sources `test_*.c`
   et non sur les binaires de `build/`, pour qu'un test orphelin ne puisse plus passer.
3. Relecture D14 du nouveau code (noms qui crient, zéro commentaire superflu).
