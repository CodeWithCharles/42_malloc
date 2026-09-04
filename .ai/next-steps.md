# Prochains petits pas

## La liste d'optimisations est ÉPUISÉE (2026-09-04)

Douze pistes évaluées, **quatre retenues, huit annulées par la mesure**.

### Retenues
| # | Optimisation | Effet |
|---|---|---|
| — | Cache de zones LARGE + page map O(1) | `large` ×37 (D31) |
| V1 | Zones cachées conservées dans la page map | `large` −33 % (D43) |
| — | libft recompilée `-O2` + `memset`/`memcpy` mot à mot | `calloc` ×25, `mixed` −24 % (D45) |
| M1 | En-tête 48 → 32 via le delta dans `flags` | zone TINY −20 %, densité +10 % (D47) |

### Annulées, chacune avec ses chiffres
`cache TINY/SMALL` (D32) · `madvise` (D36) · `boundary tags` (D39) · `free lists ségrégées`
(D44) · `calloc sans memset` (D45) · `FIT_FACTOR` dans les deux sens (D48) ·
`résidus LARGE / M2` (D49) · `index O(1) des zones / V5` (D49)

## Banc de mesure (D50)
`make -C bench demo` — la séquence à montrer en soutenance.
`make -C bench ablation` — ce que chaque optimisation apporte, une à une.
`make -C bench sweep-m|sweep-cache|sweep-map|sweep-fit` — justifier chaque valeur retenue.
Régler la durée avec `RUNS=3 OPS=100000` pour une démo en direct.
⚠️ Vérifier `uptime` avant toute campagne : au-dessus de 0,2 de charge les mesures dérivent.

## À faire
1. **Commiter.** ⚠️ Deux dépôts : d'abord DANS `thirdparty/libft` (Makefile `-O2`,
   `ft_memset.c`, `ft_memcpy.c`), puis la référence du sous-module dans le parent.
2. Vérifier que `include/ftm_config.h` est bien revenu à `FIT_NUM 2` / `FIT_DEN 1`, et
   `./configure.sh --cflags="-DFTM_SMALL_MAX=2048"`.
3. `LD_PRELOAD` sur `ls` / `python3` / `git` / `vim` — dernière validation d'intégration.
4. Relecture D14 du code ajouté aujourd'hui (noms, commentaires au minimum).
5. Le `git stash` de V2 (free lists ségrégées) : garder ou jeter.

## Piste rouverte par le banc (D50)
**N'indexer que les zones LARGE dans la page map.** L'ablation montre que la map coûte
10 % sur `small` et 17 % sur `mixed` (elle n'y remplace qu'un parcours de 1 à 2 zones) et
ne paie que sur `large`. `ftm_heap_find_zone` pourrait consulter la map puis retomber sur
le parcours linéaire — court par construction pour TINY/SMALL. À mesurer avant de coder.

## Si on reprend un jour
- Sinon, rien de conforme au sujet ne reste sur la table. Tout ce qui subsiste est dans
  `perf-hors-sujet.md` (tcache, `brk`, `free` sans validation, en-tête 16 o) et casse une
  contrainte.
- **M2 redevient pertinent en 32 bits** (KFS-3) : cf. `perf-hors-sujet.md` §6.
- Le bench mesure mal l'empreinte mémoire (jeu de travail fixe, pas d'écriture dans les
  blocs). Un profil « mémoire » dédié serait le prérequis à toute future piste mémoire.
