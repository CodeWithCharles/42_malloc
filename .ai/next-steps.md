# Prochains petits pas

## Session du 2026-09-04 : TERMINÉE
Voir `state.md` pour le bilan chiffré et `decisions.md` D28→D46 pour le détail.
Résultat : `small` ×3,2, `mixed` ×2,9, `large` ×37,5, `calloc` ×25. 16/16 tests.

## À faire tout de suite
1. **Commiter** le travail restant (V1 est commité ; la correction libft ne l'est pas).
   ⚠️ `thirdparty/libft` est un **sous-module** : il faut commiter DANS le sous-module
   (Makefile `-O2`, `ft_memset.c`, `ft_memcpy.c`) puis mettre à jour la référence dans
   le dépôt parent.
2. **Le stash V2** (`git stash list`) contient les free lists ségrégées, annulées en D44.
   Le garder ou le jeter — il ne sert que si un profil très fragmenté apparaît un jour.
3. Vérifier `LD_PRELOAD` sur `ls` / `python3` / `git` / `vim` après la correction libft.

## Pistes restantes (cf. `perf-roadmap.md`)
- **M1** — supprimer `request_size` en stockant le delta dans les bits 32-63 de `flags`
  (cf. **D42**) : en-tête 48 → 32, zone TINY 5 → 4 pages (−20 %), **sans perdre**
  l'affichage de la taille demandée. C'est la piste restante la plus rentable.
- **M2** — réutiliser les résidus des zones LARGE (jusqu'à 4 Ko/zone).
- **M3** — `FIT_FACTOR` 2 → 1,5 (un `--cflags`, non mesuré).
- **V5** — index O(1) des zones ayant de la place (gain jugé faible : 2 zones TINY).
- ~~V2~~ annulée (D44), ~~V4~~ sans objet (D45), ~~étape 3~~ annulée (D39),
  ~~madvise~~ repoussée (D36), ~~cache TINY/SMALL~~ abandonnée (D32).

## Rendu 42 — arguments de soutenance
Cf. `perf-hors-sujet.md` pour ce qui casse le sujet et pourquoi.
Le point fort de la session est **méthodologique** : sur huit pistes évaluées, cinq ont été
annulées ou redirigées **par la mesure**, dont trois qui semblaient évidentes. Les deux qui
ont payé supprimaient des **syscalls** ou des **cache miss**, jamais une complexité
algorithmique. Et le plus gros gain de la journée (`calloc` ×25) n'était pas une
optimisation mais **un bug de build** : la libft compilée en `-O0`.
