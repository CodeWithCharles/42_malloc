# Prochains petits pas

## Session LARGE : TERMINÉE ✅
`large` 3418 → 218-240 ns/op (**×14-15**), témoins neutres, 16/16 tests.
Détail : D28→D31 dans `decisions.md`. Bilan de session : `state.md`.

## À faire tout de suite
1. **Commit.** Trois fichiers encore untracked :
   `src/core/ftm_large_cache.c`, `src/core/ftm_zone_map.c`, `tests/test_large_cache.c`.
2. **Correctif `run_tests.sh`** (cf. D30) : dériver la liste des tests depuis les sources
   `test_*.c`, pas depuis les binaires de `build/`. Un binaire orphelin ne doit plus
   pouvoir compter comme un succès.
3. **Relecture D14** du code ajouté : noms qui crient, zéro commentaire superflu.

## Rendu 42
Le projet reste conforme : cache et map vivent dans `g_heap` (une seule variable globale),
zéro métadonnée allouée dynamiquement, garantie « no segv » préservée, et le cache réduit
les `munmap` — explicitement demandé par le sujet.

### Arguments de soutenance sur cette optimisation
- **Le ×45 initial contre la glibc n'était pas un écart d'implémentation.** Preuve :
  `MALLOC_MMAP_THRESHOLD_=1 ./bench_alloc large 100000` → glibc à 3737 ns/op, soit plus
  lente que notre baseline (3418). À contrainte égale, ft_malloc est aujourd'hui
  **15,6× plus rapide qu'elle**.
- **La démarche vaut autant que le résultat** : le pas A (skip du scan) mesuré isolément
  était une **régression de 77 %** (D28) — le briefing se trompait en affirmant que ce
  scan échouait à 100 %. Il fallait le cache pour que A devienne un gain (D29). Savoir
  raconter ça montre une méthode expérimentale, pas de la chance.
- **Un test qui passait sans source** (D30) : binaire orphelin d'une autre branche resté
  dans `build/`. Trouvé en vérifiant, pas en faisant confiance au compteur de tests.

## Suite : optimisation continue
Deux documents dédiés, écrits le 2026-09-04 :
- **`perf-roadmap.md`** — pistes CONFORMES au sujet, à implémenter (c'est le plan de
  travail : free list explicite, boundary tags, réglages mémoire, madvise). Ordre
  recommandé et protocole de validation inclus.
- **`perf-hors-sujet.md`** — pistes qui cassent le sujet (tcache, arènes multiples, brk,
  free non validé). Non implémentables pour le rendu ; sert d'argumentaire de soutenance
  et de porte ouverte pour KFS-3.

Diagnostic mesuré qui pilote la suite : une zone TINY porte ~140 blocs dont 8 % libres,
donc le `first-fit` linéaire consomme à lui seul les ~200 ns/op de `small` et `mixed`.
Le verrou, lui, ne coûte que 4,1 ns (2 %) — ce n'est pas le goulot.

## Post-soutenance éventuel
- KFS-3 : le port kernel (cf. roadmap §8). La page map est littéralement une table
  inversée page→zone : bon entraînement avant la pagination de KFS-3.
- Réutilisation O(1) des résidus de zones LARGE (indexer les zones partiellement libres),
  pour récupérer l'efficacité mémoire que le pas A a échangée contre de la vitesse.
