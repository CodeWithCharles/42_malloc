# Banc de mesure `ft_malloc`

Mesure la vitesse et l'empreinte mémoire de l'allocateur, compare à la glibc, et permet
de désactiver chaque optimisation ou de faire varier chaque paramètre. Conçu pour être
lancé en direct devant un évaluateur.

```bash
make -C bench demo RUNS=3 OPS=100000
```

---

## Cibles

| Cible | Ce qu'elle montre |
|---|---|
| `demo` | **La séquence de soutenance.** glibc par défaut, glibc sous notre contrainte `mmap`, notre état initial, notre état final. |
| `all` | Notre référence face à la glibc (avec et sans contrainte `mmap`). C'est la cible par défaut. |
| `ablation` | Ce que chaque optimisation apporte, en les désactivant une par une puis toutes. |
| `sweep-m` | Balayage de `FTM_SMALL_MAX` : 1024, 2048, 4096. |
| `sweep-cache` | Balayage du cache de zones LARGE : 4, 16, 32, 64, 128 zones. |
| `sweep-map` | Balayage de la page map : 1024, 2048, 4096, 8192 entrées. |
| `sweep-fit` | Balayage du critère de réutilisation : 3/2, 2/1, 3/1. |
| `everything` | Tout ce qui précède, à la suite. |
| `restore` | Reconstruit avec la configuration du `./configure.sh`. Appelée automatiquement à la fin de chaque cible. |

Chaque cible reconstruit la `.so` pour chaque variante, puis restaure ta configuration.
`Makefile.cfg` n'est jamais modifié : les drapeaux sont passés en variable de ligne de
commande (`CMOREFLAGS=...`), qui prime sur l'affectation du fichier.

## Variables

| Variable | Défaut | Rôle |
|---|---|---|
| `RUNS` | `7` | Nombre de mesures par profil. La **médiane** est affichée. |
| `OPS` | `200000` | Opérations par mesure. |
| `PROFILES` | `small mixed large calloc` | Profils à mesurer. |

```bash
make -C bench ablation RUNS=3 OPS=100000        # rapide, pour une demo en direct
make -C bench sweep-cache PROFILES="large"      # un seul profil, plus lisible
make -C bench demo RUNS=15                      # plus fiable, plus lent
```

## Les profils

Tous maintiennent 512 emplacements et alternent aléatoirement allocation et libération.

| Profil | Tailles | Particularité |
|---|---|---|
| `small` | 1 à 128 o | TINY uniquement |
| `mixed` | 70 % en 1-128, 25 % en 129-1024, 5 % en 1025-2048 | **30 % des opérations sont des `realloc`** |
| `large` | 2048 à 10239 o | LARGE uniquement, une zone `mmap` par allocation |
| `calloc` | mêmes tailles que `large` | mesure le coût du `memset` |
| `sawtooth` | 300 allocations de 128 o puis 300 libérations, en boucle | exhibe le recyclage de zones (témoin, cf. D32) |

`sawtooth` n'est pas dans `PROFILES` par défaut ; ajoute-le explicitement si tu en as besoin.

## Lire la sortie

```
                                       small     mixed     large    calloc       VmPeak
---------------------------------------------------------------------------------------
ft_malloc (reference)                  59.13     74.91    101.86    222.77      5684 KB
```

Les quatre colonnes sont des **nanosecondes par opération** (médiane sur `RUNS` mesures).
`VmPeak` est le pic de mémoire **virtuelle** du processus sur le profil `large`, lu dans
`/proc/self/status`.

Le bench n'écrit jamais dans la mémoire allouée : seul l'allocateur touche les pages, en y
posant ses en-têtes. `VmPeak` reflète donc ce qui est mappé, pas ce qui est réellement
utilisé par un programme. **C'est une limite à connaître** : le banc mesure mal l'empreinte
réelle, et sous-estime les gains des optimisations mémoire.

## Désactiver une optimisation

Trois drapeaux, définis dans `include/ftm_config.h`, tous à `1` par défaut :

| Drapeau | Ce qu'il désactive |
|---|---|
| `FTM_ENABLE_LARGE_CACHE` | Le cache de zones LARGE : chaque `free` redevient un `munmap`. |
| `FTM_ENABLE_ZONE_MAP` | La table page → zone : `ftm_heap_find_zone` repasse en O(n). |
| `FTM_ENABLE_LARGE_FASTPATH` | Le court-circuit du scan pour LARGE (le « pas A »). |

Ils passent par des **macros de façade** (`FTM_CACHE_TAKE`, `FTM_MAP_INSERT`,
`FTM_SCANS_ZONES`…), pas par des `#if` dans le corps des fonctions : le code métier reste
identique quelle que soit la configuration.

```bash
make re CMOREFLAGS="-DFTM_SMALL_MAX=2048 -DFTM_ENABLE_ZONE_MAP=0"
```

⚠️ `tests/Makefile` a ses propres `CFLAGS` et **ne lit pas `CMOREFLAGS`**. L'archive de test
est donc toujours construite dans la configuration de référence : un `16/16` obtenu sous
ablation ne valide pas la variante. Sans conséquence pour le banc, qui passe par
`LD_PRELOAD`.

## Faire varier un paramètre

| Paramètre | Défaut | Effet |
|---|---|---|
| `FTM_TINY_MAX` | 128 | Frontière TINY / SMALL. |
| `FTM_SMALL_MAX` | 2048 | Frontière SMALL / LARGE. |
| `FTM_LARGE_CACHE_MAX_ZONES` | 64 | Nombre de zones LARGE retenues. |
| `FTM_LARGE_CACHE_FIT_NUM` / `_DEN` | 2 / 1 | Une zone est réutilisée si sa taille tient entre le besoin et `besoin × NUM / DEN`. |
| `FTM_ZONE_MAP_CAPACITY` | 2048 | Entrées de la page map (puissance de deux). |

Tous sont réglables par `-D` sans toucher au code :

```bash
make re CMOREFLAGS="-DFTM_SMALL_MAX=4096 -DFTM_LARGE_CACHE_MAX_ZONES=128"
```

Les justifications chiffrées de chaque valeur sont dans `.ai/decisions.md` : D27 pour
`SMALL_MAX`, D35 pour le cache, D38 pour la map, D48 pour le fit factor.

## Avant une campagne de mesure

```bash
uptime
```

Au-dessus de **0,2** de charge moyenne, les mesures dérivent — on a observé le même binaire
donner de 106 à 249 ns/op sur une machine occupée. Ferme ce qui tourne, ou augmente `RUNS`.

Et ne conclus pas sur un écart inférieur à **15 %** sans plusieurs séries : le profil
`small` est bimodal sur cette machine (deux groupes nets autour de 53 et 65 ns/op),
probablement à cause de la mise à l'échelle de fréquence.
