# Session mentor — Optimisation du profil LARGE (à re-coder par Charles)

> **Pour l'agent qui lit ceci** : cette session se fait en mode `collab-pairing-mentor`
> (skill à charger). Tu guides, tu donnes le code en markdown, tu reviews — **tu ne
> modifies aucun fichier hors de `.ai/`**. Charles implémente tout lui-même.
> Ce document est le briefing complet : diagnostic, design cible, plan en petits pas,
> pièges connus (rencontrés lors d'un prototype), protocole de validation.

## Contexte

Le projet malloc est fini (roadmap 15/15, cf. `.ai/next-steps.md` et `decisions.md`
jusqu'à D27). Le profil bench `large` (2-10 KB) était à ~4000 ns/op contre 89 ns pour la
glibc. Un prototype écrit par Claude en mode executor (branche
`claude/malloc-performance-glibc-y5y5lz`, commit `e74e110`, non poussée — patch
`perf-large-cache.patch` chez Charles) a validé le design ci-dessous : **~320 ns/op,
soit ×12,7**, 16/16 tests verts, LD_PRELOAD ls/python3/git OK. Charles veut maintenant
le **re-coder lui-même** pour se l'approprier, sur cette branche-ci.

La branche embarque déjà deux correctifs d'hygiène (ne pas les refaire) :
- `ftm_fatal` (port POSIX) ignorait le retour de `write` → cassait `-Werror` sur les
  toolchains à `warn_unused_result` (cas des conteneurs distants) ;
- `tests/test_show_ex.c` vérifiait le hexdump de `"Hello"` (`48 65…`) alors que le test
  écrit `"hello"` (`68 65…`).

## Diagnostic (à réexpliquer à Charles au démarrage, c'est le cœur pédagogique)

1. **La comparaison brute avec la glibc est faussée.** Son seuil mmap démarre à 128 KB
   (et monte dynamiquement) : les allocs de 2-10 KB sont servies par tcache/bins sans
   syscall. Le sujet 42 impose `LARGE > m → mmap dédié`. Preuve chiffrée :
   `MALLOC_MMAP_THRESHOLD_=1 ./bench large` force la glibc dans le même régime →
   **4582 ns/op**, plus lente que notre baseline (4065).
2. **~2000-3000 ns = le couple mmap/munmap + page fault** sur l'en-tête de zone fraîche.
3. **Le reste = deux parcours O(n) dans notre code** :
   - `heap_reserve_block` scanne toutes les zones LARGE à chaque malloc LARGE, or une
     zone LARGE vivante est occupée **par construction** (1 bloc, détruite dès qu'il est
     libéré) → le scan échoue 100 % du temps, ~256 cache miss par appel sur le bench ;
   - `ftm_heap_find_zone` (chemin du free) parcourt linéairement les 3 listes de zones.

## Design cible (3 composants, validés par le prototype)

### A. Alloc LARGE en O(1) — `src/core/ftm_heap.c`
Dans `heap_reserve_block`, ne scanner les zones existantes que si `kind != FTM_LARGE` ;
pour LARGE, aller directement à `heap_push_zone`. Un seul `if`.

### B. Cache de zones LARGE — nouveau fichier `src/core/ftm_large_cache.c`
Une zone LARGE entièrement libre part dans une liste de cache (elle reste mappée) au
lieu d'être munmap. Le prochain malloc LARGE la réutilise si elle est assez grande.

- Champs ajoutés à `t_heap` : `t_zone *large_cache; size_t large_cache_count;
  size_t large_cache_hits;` (stat utile aux tests).
- Constantes dans `ftm_config.h` : `FTM_LARGE_CACHE_MAX_ZONES` (32),
  `FTM_LARGE_CACHE_FIT_FACTOR` (2).
- Contrat (prototypes dans `ftm_internal.h`) :
  ```c
  t_zone *ftm_large_cache_take(size_t payload_size); /* NULL si aucun fit */
  t_zone *ftm_large_cache_put(t_zone *zone);         /* renvoie la zone évincée ou NULL */
  void    ftm_large_cache_flush(void);               /* détruit tout (pour reset) */
  ```
- Politique de fit : `total_size >= besoin && total_size <= besoin * FIT_FACTOR`
  (le besoin = `ftm_zone_total_size(FTM_LARGE, payload_size)`). Sans borne haute, une
  zone de 12 KB servirait éternellement des demandes de 2 KB (gaspillage).
- Éviction : insertion en tête, éviction en queue quand `count == MAX` (liste ≤ 32,
  parcourir pour trouver la queue est acceptable). `put` **renvoie** l'évincée : c'est
  `ftm_heap.c` qui la détruit et incrémente `unmap_calls` (le cache ne touche pas aux
  compteurs du heap, sauf `flush` qui peut détruire directement).

### C. Page map — nouveau fichier `src/core/ftm_zone_map.c`
Hash open-addressing **statique dans `g_heap`** (pas d'allocation dynamique de
métadonnées, et toujours une seule variable globale → conforme au sujet) :
`ftm_heap_find_zone` devient O(1).

- Type : `t_zone_map_entry { uintptr_t page; t_zone *zone; }` ; table
  `zone_map[FTM_ZONE_MAP_CAPACITY]` dans `t_heap` + `zone_map_live` + `zone_map_disabled`.
- Constantes : `FTM_ZONE_MAP_CAPACITY` 8192 (puissance de 2, ~128 KB en BSS),
  `FTM_ZONE_MAP_MAX_LIVE` = 3/4 de la capacité, `FTM_ZONE_MAP_TOMBSTONE` = `(uintptr_t)-1`.
- Clé = adresse de page (`ptr - ptr % page_size`) ; hash multiplicatif
  (`(page / page_size) * 2654435761u & (CAP-1)`), sondage linéaire.
- **Chaque page de chaque zone vivante** est insérée (une zone SMALL = ~27-52 pages).
- Contrat :
  ```c
  void    ftm_zone_map_insert(t_zone *zone);
  void    ftm_zone_map_remove(t_zone *zone);   /* tombstones */
  t_zone *ftm_zone_map_lookup(void *ptr);
  bool    ftm_zone_map_is_active(void);
  void    ftm_zone_map_reset(void);
  ```
- Saturation : si `zone_map_live + pages(zone) > MAX_LIVE`, poser `zone_map_disabled`
  **avant** d'insérer quoi que ce soit (tout-ou-rien par zone) ; `ftm_heap_find_zone`
  retombe alors sur le parcours linéaire existant. `remove` opère toujours (inoffensif).

### Câblage dans `ftm_heap.c`
- `heap_push_zone` : pour LARGE, tenter `ftm_large_cache_take` d'abord (hit →
  `large_cache_hits++`, pas de `map_calls++`) ; sinon `ftm_zone_create` + `map_calls++`.
  Dans tous les cas : splice en tête de `zones[kind]` **et** `ftm_zone_map_insert`.
- `heap_release_zone_if_free` : après l'unlink, `ftm_zone_map_remove(zone)` ; si LARGE →
  `evicted = ftm_large_cache_put(zone)`, détruire l'évincée s'il y en a une
  (`unmap_calls++`) ; sinon (TINY/SMALL surnuméraire) destruction comme avant.
- `ftm_heap_reset` : détruire les zones listées, puis `ftm_large_cache_flush()` puis
  `ftm_zone_map_reset()`, puis zéro des compteurs (`large_cache_hits` compris).
- `ftm_heap_find_zone` : `if (ftm_zone_map_is_active()) return ftm_zone_map_lookup(ptr);`
  puis le parcours linéaire existant en fallback.

## Pièges rencontrés dans le prototype (les points de review à surveiller)

1. **`zone->prev` sale à la réutilisation** : une zone sortie du cache a des `next/prev`
   de la liste de cache. `heap_push_zone` doit poser `zone->prev = NULL` explicitement
   (le code d'origine comptait sur `ftm_zone_create` pour ça).
2. **Boucle infinie potentielle dans les sondages** : si la table n'a plus une seule
   case vide (vivants + tombstones = capacité), un probe `while (page != 0)` ne termine
   jamais. Borner chaque boucle de sondage à `FTM_ZONE_MAP_CAPACITY` itérations.
3. **Réutilisation des tombstones à l'insert** : sonder jusqu'à « vide ou même page » en
   mémorisant la première tombstone rencontrée ; insérer dans la tombstone si la page
   n'existe pas déjà — sinon la table se remplit de tombstones et dégénère.
4. **`zone_map_live`** ne compte que les cases vierges consommées (pas les tombstones
   réutilisées), sinon la saturation se déclenche trop tôt.
5. **Une zone cachée est retirée de la map** (au `put`) et réinsérée au `take` (via
   `heap_push_zone`). Invariant : la map ne contient que des zones **listées**. Un free
   d'un pointeur dans une zone cachée → lookup NULL → ignoré (fuite, pas crash), même
   sémantique qu'avant.
6. **Ordre dans `heap_release_zone_if_free`** : unlink de la liste → remove de la map →
   put au cache. Le `put` peut renvoyer une évincée : elle a déjà été retirée de la map
   lors de SON put, ne pas re-remove.
7. **Split dans une zone LARGE réutilisée** : le fit factor autorise une zone jusqu'à 2×
   trop grande → `ftm_block_split` peut créer un bloc libre résiduel dans une zone
   LARGE. C'est bénin : jamais réutilisé (l'alloc LARGE ne scanne plus), et recoalescé
   au free (la zone redevient « fully free » et repart au cache). À savoir expliquer.
8. **Alignement page** : le calcul de clé suppose des zones alignées page — vrai pour
   mmap ET pour le fake port (`_Alignas(4096)` + tailles de zones multiples de page).
   Ne pas utiliser de masque `& ~(page-1)` sans vérifier que page est puissance de 2 ;
   `ptr - ptr % page_size` est sûr partout.
9. **`_Static_assert` capacité puissance de deux** dans `ftm_types.h`.

## Plan en petits pas (ordre conseillé pour la session)

1. **Baseline chiffrée** : `./configure.sh --cflags="-DFTM_SMALL_MAX=2048" && make` puis
   bench glibc / glibc+`MALLOC_MMAP_THRESHOLD_=1` / ft_malloc sur `large`. Noter les
   chiffres (ils dépendent de la machine).
2. **Pas A (O(1) alloc)** seul → re-bench. Effet attendu : modeste seul (le syscall
   domine encore), mais c'est le pas qui fait comprendre le diagnostic.
3. **Types + constantes** du cache (struct, config, prototypes) → compile.
4. **`ftm_large_cache.c`** fonction par fonction (take, put, flush) → câblage minimal
   dans heap (push/release/reset) → `make test` → re-bench. Gros gain attendu ici.
5. **Types + constantes** de la page map → `ftm_zone_map.c` (insert_page, insert,
   remove, lookup, reset) → câblage `ftm_heap_find_zone` → `make test` → re-bench.
6. **Test dédié** `tests/test_large_cache.c` : réutilisation effective (même pointeur,
   `fake_port_map_count` stable, `large_cache_hits == 1`), borne d'éviction (allouer/
   libérer MAX+8 zones → `large_cache_count == MAX`), lookups map (trouve small et
   large, NULL après release d'une LARGE), zéro fuite (`map_count == unmap_count` après
   `ftm_heap_reset`).
7. **Validation finale** : 16/16 tests (fuzz compris), LD_PRELOAD ls/python3/git,
   les 3 profils bench.

## Résultats de référence du prototype (même machine que la mesure, m=2048)

| profil | glibc | baseline | prototype |
|--------|------:|---------:|----------:|
| small  |  22   |  240     | 236 (neutre) |
| mixed  |  49   |  267     | ~272 (neutre, bruit) |
| large  |  89   | 4065     | **~320 (×12,7)** |
| large, glibc `MALLOC_MMAP_THRESHOLD_=1` | 4582 | — | — |

Si l'implémentation de Charles retombe dans ces eaux-là (small/mixed neutres, large
divisé par ~10 ou mieux), c'est gagné. S'il veut comparer au prototype ligne à ligne,
le patch `perf-large-cache.patch` fait foi — mais ne pas le coller d'office : le but
est qu'il l'écrive.

## Arguments de soutenance (à faire formuler par Charles en fin de session)

- L'écart ×45 initial vient de la **stratégie** glibc (pas de mmap sous 128 KB), pas
  d'une implémentation supérieure — preuve : `MALLOC_MMAP_THRESHOLD_=1`.
- Aucune contrainte du sujet cassée : map et cache vivent dans `g_heap` (une seule
  variable globale), zéro métadonnée allouée dynamiquement, et le cache **réduit** les
  munmap — demandé explicitement par le sujet.
- C'est la « piste future » citée en D27, réalisée et mesurée.
