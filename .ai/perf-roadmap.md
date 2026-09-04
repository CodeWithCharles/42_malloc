# Roadmap performance & mémoire — pistes CONFORMES au sujet

> Suite de la session d'optimisation LARGE (D28→D31). Toutes les pistes de ce document
> respectent les contraintes du sujet : métadonnées dans les zones, une seule variable
> globale (`g_heap`), `mmap`/`munmap` uniquement, garantie « no segv » préservée.
> Les pistes qui cassent le sujet sont dans `perf-hors-sujet.md`.
>
> **Mode de travail** : `collab-pairing-mentor`. Claude guide et review, Charles code.
> Discipline de mesure héritée de D28 : **3 runs minimum, ne rien conclure sous 15 %**,
> et vérifier les témoins (`small`/`mixed`/`large`) à chaque étape.

## État de départ (2026-09-04)

| profil  | glibc | ft_malloc | écart |
|---------|------:|----------:|------:|
| `small` |  11.6 |    ~166   | ×14   |
| `mixed` |  29.3 |    ~191   | ×6,5  |
| `large` |  55.6 |    ~230   | ×4,2  |

Mémoire : en-tête de bloc **48 octets** (`sizeof(t_block)` = 40 + 8 de padding),
en-tête de zone 32, `sizeof(t_heap)` = **131 160 octets** de BSS (dominé par la page map).
Zone TINY = 5 pages (20 480 o). Surcoût d'une alloc TINY de 128 o : **37,5 %**.

## Le diagnostic mesuré

Une zone TINY porte **~140 blocs dont seulement 8 % sont libres** (22 libres / 280 au
total sur le profil `small`). `ftm_zone_find_free` parcourt donc ~140 maillons par
allocation, dont 92 % sont alloués et ne peuvent structurellement pas convenir. La zone
tient en L2, à ~1,5 ns le maillon → **~200 ns, soit la totalité du budget actuel**.

Le `first-fit` linéaire est le goulot de `small` et `mixed`. Ce n'est pas le verrou
(mesuré à 4,1 ns la paire lock/unlock, soit 2 % du budget).

---

## Étape 1 — Trois gains gratuits (rapide, sans risque)

### 1.1 `count_zones` est O(n) sur le chemin du `free`
`heap_release_zone_if_free` appelle `count_zones(zone->kind) <= 1`, qui parcourt toute la
liste de zones de ce type, à chaque fois qu'une zone devient entièrement libre.

**Design.** `size_t zone_count[FTM_ZONE_KIND_COUNT]` dans `t_heap`, incrémenté dans
`heap_push_zone`, décrémenté dans `heap_release_zone_if_free` après le délien. Remettre à
zéro dans `ftm_heap_reset`. Supprimer `count_zones`.

**Piège.** Le compteur doit être décrémenté **une seule fois** par zone sortie de
`zones[kind]` — attention au chemin LARGE où la zone part au cache puis, éventuellement,
une *autre* zone est évincée et détruite : l'évincée n'était déjà plus dans `zones[kind]`,
elle ne doit pas re-décrémenter.

### 1.2 Le hook d'historique tourne toujours
`ftm_history_record` est appelé à chaque `ftm_on_alloc` et chaque `ftm_on_free`, même
quand `show_alloc_mem_ex` ne sera jamais appelé.

**Design.** Ajouter `bool history` à `t_debug`, activé par `FT_MALLOC_HISTORY` dans
`ftm_env.c` (la variable est déjà prévue dans la roadmap d'origine, phase 11), et
conditionner l'appel dans `ftm_debug.c`.

**Piège.** `tests/test_show_ex.c` vérifie la présence de `--- history ---` : soit il
active le drapeau via `ftm_debug()->history = true`, soit il accepte une section vide.
À trancher au moment de l'implémentation.

### 1.3 Étendre le cache de zones à TINY/SMALL — ❌ ABANDONNÉ (mesuré, cf. D32)
Aujourd'hui seules les zones LARGE sont mises en cache ; une zone TINY/SMALL
surnuméraire est `munmap`ée immédiatement. Une charge qui oscille autour d'une frontière
de zone refait donc un syscall à chaque oscillation.

**Design.** Le mécanisme existe déjà (`ftm_large_cache.c`). Deux options : un cache par
`kind` (trois listes, trois compteurs), ou un cache unique filtrant sur `zone->kind` en
plus de la taille. Recommandation : **un cache par kind**, c'est plus simple à raisonner
et le `fit factor` n'a pas le même sens pour des zones TINY (toutes de taille identique).

**Verdict mesuré (2026-09-04).** Un profil `sawtooth` (300 allocs puis 300 free en boucle)
a été ajouté au bench pour exhiber le thrashing supposé : ft_malloc y fait **142,6 ns/op**,
donc *mieux* que sur `small` (~170), avec seulement 2030 syscalls pour 200 000 ops (1 %).
Aucun thrashing. Le motif est au contraire favorable au first-fit (tout se recoalesce
après la vague de free). **Piste abandonnée** : coût réel, gain nul. Profil `sawtooth`
conservé comme témoin.

**Validation étape 1.** 16/16 tests, 3 profils inchangés ou légèrement meilleurs.

---

## Étape 2 — Free list explicite (LE gros morceau)

**Objectif.** Ne plus parcourir que les blocs **libres**. Dans la mesure ci-dessus :
22 maillons au lieu de 280. Facteur ~12 sur la recherche.

**Idée.** Un bloc libre a un payload inutilisé par définition : on y range les deux
pointeurs de chaînage. Le payload minimal est `FTM_ALIGNMENT` = 16 octets, soit exactement
deux pointeurs — ça tombe juste, mais c'est à vérifier par un `_Static_assert`.

**Design proposé.**

```c
typedef struct s_free_node
{
	t_block	*next_free;
	t_block	*prev_free;
}	t_free_node;

_Static_assert(sizeof(t_free_node) <= FTM_ALIGNMENT,
	"free list node must fit in the smallest payload");
```

Liste **par kind, globale au heap** (et non par zone) : `t_block *free_list[FTM_ZONE_KIND_COUNT]`
dans `t_heap`. Conséquence : `heap_reserve_block` ne parcourt plus les zones du tout pour
TINY/SMALL — il parcourt directement la liste des blocs libres. Le `zone` d'un bloc est
retrouvé en O(1) par la page map quand on en a besoin.

Accès au nœud : `(t_free_node *)ftm_block_payload(block)`.

**Opérations à câbler.**

| endroit | action |
|---|---|
| `ftm_zone_create` | le bloc unique initial → `push` dans `free_list[kind]` |
| `heap_reserve_block` | parcourir `free_list[kind]` (first-fit), `unlink` le bloc retenu |
| `ftm_block_split` | le résidu créé est libre → `push` |
| `ftm_release` / `release_block` | le bloc libéré → `push` |
| `ftm_block_coalesce_next` | `unlink` le voisin absorbé avant la fusion |
| coalescing avec `prev` | `unlink` le bloc courant, `prev` reste dans la liste |
| `heap_release_zone_if_free` | `unlink` le bloc unique de la zone avant cache/destruction |
| `ftm_large_cache_flush` / `ftm_heap_reset` | remettre `free_list[]` à `NULL` |

**Maintenir la liste aussi pour LARGE**, même si l'allocation LARGE ne la consulte jamais
(pas A) : ça garde un invariant uniforme et évite un cas particulier dans `ftm_check_heap`.

**Nouvel invariant pour `ftm_check_heap`** (le filet indispensable) :
- tout bloc libre d'une zone listée appartient à `free_list[zone->kind]`, exactement une fois ;
- tout bloc de `free_list[k]` est libre et appartient à une zone listée de kind `k` ;
- `next_free`/`prev_free` sont cohérents (`b->next_free->prev_free == b`).

**Pièges anticipés.**
1. **Le bloc d'une zone qui part au cache doit être délié.** Sinon la free list pointe
   dans une zone non listée → un `malloc` ultérieur servirait une adresse d'une zone
   « inactive » que la page map ne reconnaît plus, et son `free` serait ignoré → fuite
   silencieuse. C'est le piège n°1 de cette étape.
2. **Ordre dans le coalescing.** `unlink` le voisin **avant** de modifier les tailles,
   sinon on perd le pointeur.
3. **Le payload d'un bloc libre n'est plus « libre » pour le debug.** `FT_MALLOC_SCRIBBLE`
   remplit le payload libéré de `0xDE` — il écraserait les pointeurs de chaînage.
   Il faut soit scribbler à partir de `sizeof(t_free_node)`, soit désactiver la free list
   quand scribble est actif. Recommandation : **scribbler à partir de l'offset 16**, et
   documenter que les 16 premiers octets d'un bloc libéré ne sont pas empoisonnés.
4. **`FT_MALLOC_GUARD`** écrit un canari dans `[request_size, payload_size)` d'un bloc
   **alloué** — pas de conflit, les deux ne coexistent jamais sur le même bloc.

**Gain attendu.** `small` de ~166 vers 60-90 ns/op. Pas 11,6 (ça, c'est le tcache, hors
sujet), mais un facteur 2 à 3.

**Validation.** 16/16 + `test_fuzz` en priorité (il est le seul à générer des séquences
alloc/free/realloc assez tordues pour casser une free list mal maintenue). Prévoir
d'ajouter des `CHECK(ftm_check_heap())` dans `test_free_coalesce`.

---

## Étape 3 — Boundary tags (structurel, le plus formateur)

**Objectif.** Passer l'en-tête de bloc de 48 à **16 octets**, et rendre la navigation
arithmétique au lieu du pointer chasing.

**Design.**

```c
typedef struct s_block
{
	size_t	size_and_flags;   /* taille alignée 16 → 4 bits bas libres pour les flags */
	size_t	prev_size;        /* taille du bloc physiquement précédent (boundary tag) */
}	t_block;
```

- `next = (unsigned char *)block + FTM_BLOCK_HEADER_SIZE + payload_size` — calcul, pas
  déréférencement ;
- `prev = (unsigned char *)block - prev_size - FTM_BLOCK_HEADER_SIZE` ;
- fin de zone détectée par comparaison d'adresse avec `zone + total_size`, plus par
  `next == NULL`.

**Ce qu'on y gagne.** Une alloc TINY de 128 o coûte 144 au lieu de 176 (**−18 %**), la
zone TINY passe de 5 à 4 pages, et le parcours des blocs devient séquentiel donc
prefetch-friendly.

**Ce que ça coûte.** Réécriture de `ftm_block.c`, `ftm_zone.c` (itération), `ftm_check.c`,
`ftm_show.c`, et de la détection de fin de liste partout. C'est une chirurgie du cœur.

**Impact sur D3.** La règle « liste doublement chaînée ordonnée par adresse » disparaît en
tant que telle, mais sa **justification** (coalescing O(1) avec les deux voisins) est
préservée : les boundary tags donnent exactement la même propriété. À consigner comme
décision si on la franchit.

**Prérequis.** Ne surtout pas attaquer avant que l'étape 2 soit verte et commitée.
Branche dédiée, les 16 tests comme filet.

---

## Étape 4 — Mémoire

### 4.1 Supprimer `request_size` (−16 octets par bloc) — **décision de Charles requise**
`sizeof(t_block)` vaut 40, arrondi à 48 : 8 octets sont déjà du padding pur. Retirer
`request_size` ramène la structure à 32 octets pile, donc l'en-tête à **32**.
Zone TINY : 5 → 4 pages (**−20 %**). Surcoût d'une alloc de 128 o : 37,5 % → 25 %.

**Contrepartie.** `show_alloc_mem` afficherait la taille *utilisable* et non la taille
demandée — c'est-à-dire l'inverse de D20, choisi explicitement par Charles. Note : la
taille utilisable est la sémantique de `malloc_usable_size(3)`, donc défendable.
**À trancher avant de coder.** Sans objet si on fait l'étape 3 (les boundary tags
suppriment le champ de toute façon).

### 4.2 `MADV_DONTNEED` sur les zones en cache — ⏸ REPOUSSÉ (mesuré, cf. D36)
Le cache retient jusqu'à 32 zones (~400 Ko de RSS jamais rendus). Un
`madvise(zone, total_size, MADV_DONTNEED)` au moment du `put` libère les pages
**physiques** en gardant le mapping virtuel : au `take`, pas de `mmap`, juste des défauts
de page à la première écriture. C'est ce que fait jemalloc.

**Conformité.** `madvise` n'est pas dans la liste autorisée de la partie obligatoire, mais
le sujet autorise d'autres fonctions *« as long as their use is justified during your
defence »*. La justification est directe : on rend la mémoire sans perdre le cache.
**À isoler dans `port/`** (D2) : `ftm_release_pages(void *addr, size_t length)` comme 11ᵉ
fonction du contrat, no-op dans le fake port.

**Mesuré le 2026-09-04 (D36) — invalide en l'état.** Le taux de hit du cache est déjà de
99,5 %, donc madviser toutes les zones convertirait des hits gratuits (0 ns) en hits à
1 389 ns : régression ×6. Seul un cache à deux étages (chaud non-madvisé + froid madvisé)
tiendrait, et son plafond mesuré est de 8 % de gain vitesse. Repoussé : c'est une
optimisation mémoire, à rouvrir si le RSS devient un problème réel.

### 4.3 Redimensionner la page map
`FTM_ZONE_MAP_CAPACITY` = 8192 entrées × 16 o = 128 Ko de BSS, pour indexer ~24 Mo de
zones à 3/4 de charge. Surdimensionné pour un programme ordinaire. 2048 entrées
(32 Ko, ~6 Mo indexables) suffiraient largement, avec repli linéaire au-delà.
**À mesurer** : vérifier que `zone_map_disabled` ne se déclenche jamais sur les 3 profils
ni sur `LD_PRELOAD` de `python3`/`git`.

### 4.4 Resserrer `FTM_LARGE_CACHE_FIT_FACTOR`
À 2, une zone réutilisée peut être deux fois trop grande. À 1,5 on serre la mémoire au
prix du taux de hit. **À mesurer**, c'est un simple `--cflags`.

### 4.5 Réutiliser les résidus des zones LARGE
Le pas A (D29) a échangé jusqu'à 4 Ko de résidu par zone LARGE contre de la vitesse.
Indexer les zones LARGE **partiellement libres** dans une structure O(1) récupérerait la
mémoire sans réintroduire le scan O(n). C'est la vraie réponse « meilleur des deux
mondes », et c'est la plus grosse pièce de cette étape.

---

## Ordre recommandé

```
Étape 1 (1.1 + 1.2 faits, 1.3 abandonné sur mesure — D32)
Étape 2 (free list)        → mesurer          ← le gros gain vitesse
Étape 4.3 + 4.4 (réglages) → mesurer          ← quasi gratuit
Étape 4.2 (madvise)        → mesurer
Étape 3 (boundary tags)    → mesurer          ← le gros gain mémoire, le plus risqué
Étape 4.5 (résidus LARGE)  → mesurer
```

L'étape 4.1 devient sans objet si on fait l'étape 3 : à trancher au moment d'arriver là.

## Protocole de validation, à chaque étape

1. `make re && make test` → 16/16, `test_fuzz` en particulier ;
2. 3 runs par profil, comparer les **médianes**, ne rien conclure sous 15 % ;
3. `LD_PRELOAD` sur `ls`, `python3`, `git` ;
4. consigner le chiffre dans `decisions.md` — y compris (surtout) si c'est une régression.
