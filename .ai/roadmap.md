# Roadmap — `ft_malloc` (C + ASM ciblé)

> Document vivant. Chaque phase = un petit pas stable, testable, commitable.
>
> **Mode de travail** : je guide, tu codes. Signatures et structure d'abord, puis
> implémentation en markdown, puis review. Je ne touche à aucun fichier hors de `.ai/`.
>
> **Périmètre** : ce document couvre **le projet `malloc`**. KFS-3 n'apparaît qu'en
> annexe §8, sous forme de quelques règles d'hygiène gratuites à respecter maintenant
> pour ne rien s'interdire plus tard.

---

## 1. Ce que le sujet impose

### Obligatoire
| # | Contrainte | Où ça atterrit |
|---|---|---|
| 1 | `void *malloc(size_t)`, `void free(void*)`, `void *realloc(void*, size_t)` | `src/malloc.c` (shim mince) |
| 2 | `mmap(2)` / `munmap(2)` **uniquement** | `port/ftm_port_posix.c` |
| 3 | Zéro `malloc` libc en interne | par construction : `core/` ne linke rien |
| 4 | Zones TINY/SMALL pré-allouées, minimiser `mmap`/`munmap` | `core/ftm_heap.c` |
| 5 | Taille de zone = multiple de `sysconf(_SC_PAGESIZE)` | `core/ftm_config.h` |
| 6 | ≥ 100 allocations par zone | `ftm_config.h` (**calculé**, pas écrit en dur) |
| 7 | TINY `1..n` → zones `N` ; SMALL `n+1..m` → zones `M` ; LARGE `>m` → mmap dédié | `core/ftm_heap.c` |
| 8 | `n, m, N, M` choisis et justifiables | `.ai/decisions.md` |
| 9 | `show_alloc_mem()` trié par adresses croissantes | `core/ftm_show.c` |
| 10 | Mémoire **alignée** | `core/ftm_align.h` |
| 11 | Jamais d'UB, jamais de segv | `core/ftm_guard.c` + tests |
| 12 | 1 variable globale (+1 pour le thread-safe) | `g_heap` dans `ftm_heap.c` |
| 13 | Code propre même sans Norme — *« if it's ugly you will get 0 »* | review continue |
| 14 | `libft_malloc_$HOSTTYPE.so` + symlink `libft_malloc.so` | `Makefile` |
| 15 | Makefile avec règles usuelles, recompilation incrémentale | `Makefile` |
| 16 | `libft/` livrée avec son propre Makefile | sous-Makefile appelé par la racine |

Fonctions autorisées dans la partie obligatoire : `mmap`, `munmap`, `sysconf(_SC_PAGESIZE)`,
`getrlimit`, ta `libft` (dont `write`), et libpthread.

### Bonus — architecturés dès le départ, pas greffés après
| # | Bonus | Décision anticipée |
|---|---|---|
| B1 | Thread-safe (pthread) | `ftm_lock()`/`ftm_unlock()` dans le contrat de port **dès la phase 1**, implémentés en no-op, puis en `pthread_mutex`. Aucun `#ifdef THREAD_SAFE` disséminé. |
| B2 | Variables d'env de debug | Hooks `ftm_on_alloc/on_free/on_error` appelés depuis `ftm_heap.c` **dès la phase 5**, vides au début. |
| B3 | `show_alloc_mem_ex()` : historique + hexdump | `ftm_show.c` séparé de `ftm_heap.c` dès la phase 8 ; l'historique consomme les hooks B2. |
| B4 | « Défragmentation » | **Non-négociable dès la phase 4** : liste de blocs doublement chaînée **en ordre d'adresse**, blocs alloués compris. Coalescing en O(1). Non-rétrofittable. |

> ⚠️ *« The bonus part will only be assessed if the mandatory part is PERFECT. »*
> D'où l'ordre de travail du §7 : phases 0→9 puis le filet de sécurité (invariants + fuzz),
> **avant** de toucher aux bonus.

### Un détail que le sujet trahit
```
TINY : 0xA0000
0xA0020 - 0xA004A : 42 bytes
0xA006A - 0xA00BE : 84 bytes
```
`0xA0020 - 0xA0000 = 0x20 = 32` → en-tête de 32 octets. `0xA004A - 0xA0020 = 0x2A = 42` →
l'adresse de fin est la fin du payload. `0xA006A - 0xA004A = 32` → confirmé.

On ne le **hardcode** pas pour autant : la taille d'en-tête sera
`ALIGN_UP(sizeof(t_block), ALIGNMENT)`, calculée. Voir §3.

---

## 2. Architecture

Deux objectifs pilotent le découpage : **faire les bonus sans réécrire**, et **pouvoir
tester sérieusement**. Une troisième propriété tombe gratuitement (portabilité) — voir §8.

```
        ┌──────────────────────────────────────────────┐
        │  src/malloc.c                                │  ← exporte malloc/free/realloc
        │  shim mince : lock, appel core, unlock       │
        └──────────────────────────────────────────────┘
                              │
        ┌──────────────────────────────────────────────┐
        │  core/     algorithme pur                    │  ← zéro libc, zéro syscall,
        │  align · block · zone · arena · show · debug │     zéro printf
        └──────────────────────────────────────────────┘
                              │  ftm_port.h  (9 fonctions)
        ┌──────────────────────────────────────────────┐
        │  port/           │  tests/fake_port.c        │
        │  mmap, sysconf,  │  pool statique,           │
        │  write(2), mutex │  adresses déterministes   │
        └──────────────────────────────────────────────┘
```

### Le contrat — `core/ftm_port.h`

Neuf fonctions. C'est **tout** ce que `core/` sait du monde extérieur.

```c
#ifndef FTM_PORT_H
# define FTM_PORT_H

# include "ftm_stdint.h"

/* --- mémoire brute ---------------------------------------------- */
size_t   ftm_page_size(void);
void    *ftm_map_pages(size_t nbytes);              /* NULL si échec */
void     ftm_unmap_pages(void *addr, size_t nbytes);

/* --- exclusion mutuelle (B1) ------------------------------------ */
void     ftm_lock(void);
void     ftm_unlock(void);

/* --- sortie (show_alloc_mem) ------------------------------------ */
void     ftm_write(const char *buf, size_t n);

/* --- utilitaires mémoire (délégués à ta libft) ------------------ */
void    *ftm_memcpy(void *dst, const void *src, size_t n);
void    *ftm_memset(void *dst, int c, size_t n);

/* --- erreur irrécupérable --------------------------------------- */
void     ftm_fatal(const char *msg);

#endif
```

Ce que ça t'apporte **tout de suite**, sur le projet malloc lui-même :

1. **`tests/fake_port.c`** distribue la mémoire depuis un `static char g_pool[16M]` en bump.
   Les adresses sont donc **déterministes** → tu peux comparer la sortie de
   `show_alloc_mem` caractère par caractère à une chaîne attendue. Impossible avec `mmap`.
2. Tu peux simuler un **échec de `mmap`** à volonté (`fake_port` renvoie `NULL` au N-ième
   appel) et vérifier que `malloc` renvoie proprement `NULL`. C'est la contrainte n°11 du
   sujet, testée pour de vrai.
3. `ftm_memcpy`/`ftm_memset` délégués à ta `libft` → tu satisfais le
   *« if you are clever, you will use your library for your malloc »* sans que `core/`
   dépende de quoi que ce soit.

### Deux cibles de build, et c'est le point le plus important du Makefile

| Cible | Contenu | Sert à |
|---|---|---|
| `libft_malloc_$(HOSTTYPE).so` | `core/` + `port/` + `src/malloc.c` | **le livrable** : exporte `malloc`/`free`/`realloc`, `LD_PRELOAD`-able |
| `libftm_core.a` | `core/` + `tests/fake_port.c` | **les tests unitaires** : expose `ftm_alloc()`/`ftm_release()`, pas `malloc` |

**Pourquoi c'est vital.** Si tes tests linkent la `.so`, ils remplacent le `malloc` de la
libc : le moindre `printf` ou `assert` de ton test passe alors par l'allocateur que tu es
en train de debugger. Tu ne sauras jamais si le crash vient du test ou du code.

Avec le `.a` et le préfixe `ftm_`, aucune collision de symbole. Tes tests utilisent
`printf`, `assert`, `valgrind`, tout ce que tu veux. C'est ce qui rend les phases 3 à 9
réellement débuggables.

---

## 3. Chiffrage

```c
/* core/ftm_config.h — tout est DÉRIVÉ, rien n'est magique */

#define FTM_ALIGNMENT     (2 * sizeof(void *))                          /* 16 sur x86-64 */
#define FTM_ALIGN_UP(x,a) (((x) + ((a) - 1)) & ~((a) - 1))

#define FTM_BLOCK_HEADER_SIZE     FTM_ALIGN_UP(sizeof(t_block), FTM_ALIGNMENT)  /* 32 */
#define FTM_ZONE_HEADER_SIZE      FTM_ALIGN_UP(sizeof(t_zone),  FTM_ALIGNMENT)  /* 32 */

#define FTM_TINY_MAX      128     /* n */
#define FTM_SMALL_MAX     1024    /* m */
#define FTM_MIN_ALLOCS    100     /* contrainte du sujet */

/* N et M : calculés au runtime, car dépendants de ftm_page_size() */
#define FTM_ZONE_SIZE(maxalloc, pagesz) \
    FTM_ALIGN_UP(FTM_ZONE_HEADER_SIZE + FTM_MIN_ALLOCS * (FTM_BLOCK_HEADER_SIZE + (maxalloc)), (pagesz))
```

Valeurs résultantes sur x86-64 / page 4096 :

| | max alloc | calcul | zone |
|---|---|---|---|
| TINY  | 128  | `32 + 100*(32+128)`  = 16 032  → | **16 384** (4 pages, ~101 allocs) |
| SMALL | 1024 | `32 + 100*(32+1024)` = 105 632 → | **106 496** (26 pages, ~100 allocs) |
| LARGE | > 1024 | `ALIGN_UP(ZONE_HDR + BLOCK_HDR + size, page)` | zone dédiée, 1 bloc |

**Pourquoi ces valeurs** (à savoir défendre) :
- `n = 128` : couvre l'immense majorité des petites allocs (nœuds de liste, petites
  strings). Plus petit → trop de TINY basculent en SMALL. Plus grand → gaspillage interne.
- `m = 1024` : au-delà, les allocs sont assez rares et grosses pour qu'un `mmap` dédié soit
  acceptable, et `M` reste à 26 pages — raisonnable à pré-allouer.
- `N` et `M` **dérivés** : la contrainte « ≥ 100 allocs » est garantie par le calcul, pas
  par une constante magique. Tu peux re-tuner `n`/`m` en phase 13 sans jamais risquer de
  casser la conformité. C'est un vrai point de défense.

⚠️ Aucune de ces valeurs ailleurs que dans `ftm_config.h`. Un `_Static_assert` vérifiera
`FTM_BLOCK_HEADER_SIZE % FTM_ALIGNMENT == 0` en phase 3.

---

## 4. Où l'ASM gagne sa place

Pas d'ASM décoratif. Trois endroits justifiables :

| Fichier | Ce que ça fait | Justification | Phase |
|---|---|---|---|
| `arch/x86_64/ftm_rdtsc.S` | compteur de cycles | Bench de la phase 13, sans dépendre de `clock_gettime` (qui n'est pas dans les fonctions autorisées). | 13 |
| `arch/x86_64/ftm_frame.S` | remonte la chaîne `%rbp` | `show_alloc_mem_ex` : « quel site a alloué ce bloc ». Très efficace en soutenance. | 12 |
| `arch/x86_64/ftm_memcpy.S` | `rep movsb` / `rep stosb` | Chemin chaud de `realloc`. À faire **après** le bench, et seulement s'il prouve un gain. | 13 |

**Deux garde-fous :**
1. Le **chemin obligatoire reste du C pur + `libft`**. L'ASM vit dans le bonus et la perf.
   Le sujet autorise d'autres moyens *« as long as their use is justified during your
   defence »* — autant ne rien avoir à justifier sur la partie notée en premier.
2. `arch/x86_64/` est un dossier à part, sélectionné par le Makefile. Pas d'`#ifdef`
   d'architecture au milieu du code métier.

Syntaxe AT&T, fichiers `.S` (passés au préprocesseur), assemblés par `$(CC) -c` — comme
dans ton `kfs-1`, pour ne pas changer d'habitude.

*(Un spinlock ASM `lock cmpxchg` est possible en phase 10, mais `pthread_mutex` est
explicitement autorisé par le sujet et suffit. Cf. §8.)*

---

## 5. Arborescence  (calquée sur woody_woodpacker)

```
42_malloc/
├── .ai/                        # contexte de session (seul dossier que Claude ecrit)
│
├── configure.sh                # genere Makefile.cfg (DEBUG/OPTIMIZE/ARCH/OBJDIR/...)
├── Makefile                    # cibles + regles de build
├── Makefile.vars               # variables (include Makefile.cfg) + macro get-lib-info
├── Makefile.msg                # macros de log : qcmd / bcmd / msg / rmsg / emsg
├── Makefile.cfg                # GENERE par ./configure — git-ignore
│
├── thirdparty/
│   ├── .gitkeep
│   ├── libft.dep               # branche la libft dans le build (contrat get-lib-info)
│   └── libft/                  # SOUS-MODULE git (42_libft_full), son propre Makefile
│
├── include/                    # tous les headers
│   ├── ft_malloc.h             #   API publique
│   ├── ftm_port.h              #   le contrat (9 fonctions)
│   ├── ftm_config.h            #   n, m, N, M, alignement — tout derive
│   ├── ftm_types.h             #   t_block, t_zone, t_heap, flags
│   ├── ftm_internal.h          #   prototypes internes
│   └── ftm_stdint.h            #   indirection vers <stdint.h> (cf. D9)
│
├── src/
│   ├── malloc.c                #   shim : malloc/free/realloc/calloc → core
│   ├── core/                   # ⭐ algo pur · zero libc · zero syscall · zero printf
│   │   ├── ftm_align.c  ftm_block.c  ftm_zone.c  ftm_heap.c
│   │   ├── ftm_guard.c  ftm_debug.c  ftm_history.c
│   │   ├── ftm_fmt.c   ftm_show.c   ftm_check.c
│   ├── port/                   #   seule couche qui touche l'OS
│   │   ├── ftm_port_posix.c    #     mmap, munmap, sysconf, getrlimit, write(2), abort
│   │   ├── ftm_lock_pthread.c  #     B1
│   │   └── ftm_env.c           #     B2 : getenv → drapeaux
│   └── arch/x86_64/            #   ASM cible (rdtsc, frame walk, memcpy) — phases 12/13
│
├── tests/                      # build SEPARE (symboles ftm_*, cf. D8)
│   ├── Makefile                #   construit libftm_core.a (core + fake_port) puis les tests
│   ├── run_tests.sh            #   runner facon woody
│   ├── fake_port.c             #   port sur pool statique, adresses deterministes
│   ├── test_align.c  test_zone.c  test_alloc.c  test_free_coalesce.c
│   ├── test_realloc.c  test_show.c  test_errors.c  test_thread.c  test_env.c
│   ├── test_fuzz.c             # ⭐ oracle + ftm_check_heap() a chaque etape
│   └── integration/            #   scripts LD_PRELOAD (ls, vim, prog multithread) — phase 14
│
├── bench/  bench_alloc.c
└── README.md
```

**Ce qu'on reprend de woody :** le trio `Makefile`/`.vars`/`.msg` + `configure.sh` +
`Makefile.cfg` genere, le mecanisme `thirdparty/*.dep`, le nommage `%.c.o`/`%.c.d`, les
macros de log, `tests/` avec `run_tests.sh`.

**Ce qu'on adapte (cf. D12) :** on link une `.so` (`-shared`) au lieu d'un exe (`-pie`) ;
les tests unitaires ont leur build propre a cause de D8. On NE reprend PAS `resources/`,
`stub/`, ni le double build 32/64 de packing.

## 6. Les phases

Légende : ⬜ à faire · 🟨 en cours · ✅ fait

---

### ✅ Phase 0 — Socle du repo  (build facon woody)
**Objectif** : `./configure && make` produit `libft_malloc_$(HOSTTYPE).so` + le symlink,
et `make -C tests` fait passer un test smoke. Aucun allocateur encore.

Livrables :
- `configure.sh` + `Makefile` + `Makefile.vars` + `Makefile.msg` (adaptes de woody)
- `thirdparty/libft.dep` + sous-module `thirdparty/libft`
- `include/ft_malloc.h`, `src/malloc.c` (stubs)
- `tests/Makefile` + `tests/run_tests.sh` + `tests/test_smoke.c` + `tests/fake_port.c` (vide)
- `.gitignore` (dont `Makefile.cfg`, `build/`, `*.so`, `*.a`)

Le fallback `HOSTTYPE` exige par le sujet vit dans `Makefile.vars` :
```make
ifeq ($(HOSTTYPE),)
    HOSTTYPE := $(shell uname -m)_$(shell uname -s)
endif
NAME := libft_malloc_$(HOSTTYPE).so
LINK := libft_malloc.so
```

**Done quand :**
1. `./configure` ecrit `Makefile.cfg`
2. `make` → `.so` + symlink `libft_malloc.so`
3. `nm -D libft_malloc.so | grep -E ' T (malloc|free|realloc)'` → 3 lignes, rien d'autre
4. `make -C tests` → `PASS test_smoke`
5. `make re`, `make fclean` OK ; toucher un header ne recompile que le `.c` concerne (`-MMD`)

**Pieges :**
- `libft` doit etre compilee en `-fPIC` sinon le link `.so` casse (R_X86_64_32S).
- `-fno-builtin-malloc -fno-builtin-free -fno-builtin-realloc` : sans eux, `-O2` peut
  casser silencieusement ta future implementation.
- `-fvisibility=hidden` + `FTM_PUBLIC` : seuls malloc/free/realloc/calloc/show_* sortent.
- Si le shell exporte `HOSTTYPE`, on obtient `x86_64` au lieu de `x86_64_Linux` — les deux
  sont conformes, mais sache lequel tu produis.

### ✅ Phase 1 — Le contrat et les types
**Objectif** : figer les interfaces avant d'écrire la moindre logique.

- `core/ftm_port.h` (les 9 fonctions, cf. §2)
- `core/ftm_config.h` (constantes dérivées, cf. §3)
- `core/ftm_types.h` :
  ```c
  typedef struct s_block {
      size_t           size;      /* taille du PAYLOAD */
      struct s_block  *next;      /* bloc suivant EN ADRESSE, dans la zone */
      struct s_block  *prev;
      uintptr_t        flags;     /* FTM_FREE | FTM_CANARY | ... */
  } t_block;

  typedef struct s_zone {
      struct s_zone   *next;
      struct s_zone   *prev;
      size_t           total_size;
      uintptr_t        kind;      /* FTM_TINY | FTM_SMALL | FTM_LARGE */
  } t_zone;

  typedef struct s_heap {
      t_zone  *zones[3];          /* indexé par kind */
      size_t   mmap_calls;
      size_t   munmap_calls;
      int      initialized;
  } t_heap;
  ```
- `port/ftm_stdint.h`

**Done quand** : `core/` compile seul (`gcc -c core/*.c`) sans aucune implémentation du
port. C'est le test d'étanchéité de la couche : si ça râle sur un symbole libc, c'est
qu'un `#include <string.h>` s'est glissé quelque part.

**Note sur `t_heap`** : en interne, toutes les fonctions prennent `t_heap *a`. Le
`static t_heap g_heap` du shim est l'unique instance exposée (conforme au sujet :
« a global variable to manage your allocations »). Coût : un paramètre. Bénéfice
immédiat : **chaque test unitaire travaille sur une arena neuve et isolée**, au lieu de
subir l'état laissé par le test précédent.

---

### ✅ Phase 2 — Port POSIX + fake port
- `ftm_map_pages` → `mmap(NULL, n, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0)`,
  `MAP_FAILED` → `NULL`
- `ftm_page_size` → `sysconf(_SC_PAGESIZE)`, mis en cache dans un `static`
- `getrlimit(RLIMIT_AS)` / `RLIMIT_DATA` : refuser proprement plutôt que de laisser `mmap`
  échouer en boucle
- `ftm_write` → `write(2)`
- `ftm_lock`/`ftm_unlock` → **vides** (B1 en phase 10)
- `tests/fake_port.c` : bump allocator sur `static char g_pool[16 * 1024 * 1024]`,
  + un compteur `fake_fail_after(n)` pour simuler l'échec de mapping

**Done quand** : un test mappe 4 pages, écrit dedans, relit, démappe.

---

### ✅ Phase 3 — Alignement et en-têtes
- `ftm_align_up()`, `ftm_size_class(size)` → `TINY|SMALL|LARGE`
- Accesseurs : `ftm_block_payload(b)`, `ftm_payload_block(p)`, `ftm_block_end(b)`
- `ftm_block_is_free()` / `ftm_block_set_free()` via `flags`
- `_Static_assert(FTM_BLOCK_HEADER_SIZE % FTM_ALIGNMENT == 0, "header misaligned")`

**Done quand** : test aux valeurs limites — 0, 1, `n`, `n+1`, `m`, `m+1`, `SIZE_MAX`.

**Piège n°1 du projet** : `ALIGN_UP(SIZE_MAX, 16)` déborde silencieusement et donne 0.
Toute addition sur une taille venant de l'utilisateur doit être précédée d'un contrôle
d'overflow :
```c
if (size > SIZE_MAX - FTM_BLOCK_HEADER_SIZE - FTM_ALIGNMENT)
    return (NULL);
```

---

### ✅ Phase 4 — Zone
- `ftm_zone_create(a, kind)` → `ftm_map_pages` + un unique bloc libre couvrant tout
- `ftm_zone_destroy(a, z)` → retire de la liste + `ftm_unmap_pages`
- Itération sur les blocs **en ordre d'adresse**
- `ftm_zone_find_free(z, size)` → first-fit (best-fit évalué en phase 13, mesures à l'appui)

**Done quand** : une zone TINY neuve contient exactement 1 bloc libre de
`N - ZONE_HDR - BLOCK_HDR` octets, et l'itération le retrouve.

---

### ✅ Phase 5 — `ftm_alloc()` puis `malloc()`
- Routage par classe, first-fit sur les zones existantes, création de zone si besoin
- **Split** d'un bloc trop grand — mais pas si le reste ne peut pas contenir
  `BLOCK_HDR + ALIGNMENT` (sinon tu fabriques des blocs inutilisables)
- Appel des hooks `ftm_on_alloc()` — vides, mais présents
- `ftm_lock()`/`ftm_unlock()` autour — no-op, mais présents
- `src/malloc.c` : le shim, `__attribute__((visibility("default")))`

**Done quand** : 200 `malloc(64)` renvoient 200 adresses distinctes, alignées sur 16,
non chevauchantes, dans exactement 2 zones TINY.

**Pièges :**
- `malloc(0)` : décide maintenant. Recommandation : un pointeur unique et libérable
  (comportement glibc), pas `NULL`. À noter dans `decisions.md`.
- **Initialisation paresseuse, pas de `__attribute__((constructor))`.** En `LD_PRELOAD`,
  l'ordre des constructeurs n'est pas garanti et ton `malloc` peut être appelé avant le
  tien. Un drapeau `g_heap.initialized` sous lock suffit.
- **Ne jamais appeler `printf` depuis la lib** : `printf` alloue → récursion infinie.
  D'où `ftm_fmt.c` en phase 8.

---

### ✅ Phase 6 — `ftm_release()` + coalescing (bonus B4)
- Retrouver la zone d'un pointeur : parcours des trois listes
- Marquer libre, puis fusionner avec `prev` **et** `next` s'ils sont libres — d'où l'ordre
  par adresse décidé en phase 4
- `ftm_unmap_pages` d'une zone entièrement libre, **mais garder toujours ≥ 1 zone TINY et
  1 SMALL** : sinon un cycle malloc/free repart en syscall à chaque tour, et le sujet
  demande explicitement de limiter les appels à `munmap()`
- `free(NULL)` = no-op strict

**Done quand** : `malloc` ×3 → free du 2ᵉ → free du 1ᵉʳ → free du 3ᵉ ⇒ la zone est revenue
à **un seul** bloc libre pleine taille. Le test compte les blocs, pas seulement l'absence
de crash.

**C'est la phase qui décide de la qualité du projet.** Écris `ftm_check_heap()`
(phase 13) juste après : tous les bugs suivants deviennent évidents au lieu d'être
mystérieux.

---

### ✅ Phase 7 — `ftm_resize()` / `realloc()`
Dans l'ordre :
1. `realloc(NULL, n)` ≡ `malloc(n)`
2. `realloc(p, 0)` → décision à consigner (glibc : free + pointeur minimal)
3. Taille ≤ actuelle → split en place si le reste est exploitable
4. Taille > actuelle **et bloc suivant libre et suffisant** → absorption en place, zéro
   copie. Gros gain, à montrer en soutenance.
5. Sinon → `alloc` + `ftm_memcpy(min(old,new))` + `release`
6. Changement de classe (TINY→SMALL→LARGE) → forcément le cas 5

Puis, presque gratuitement : `calloc(nmemb, size)` avec détection d'overflow sur la
multiplication, et `reallocarray`.

**Done quand** : un test écrit un motif reconnaissable, réalloue en montant puis en
descendant, et vérifie que les octets survivants sont intacts.

---

### ✅ Phase 8 — `show_alloc_mem()`
- Zones triées par adresse croissante, blocs **alloués** uniquement
- Format strict du sujet ; `Total` = somme des payloads alloués, **pas** la taille des zones
- `ftm_fmt.c` : ton propre `hex()` et `udec()` écrivant dans un buffer sur la pile, puis
  **un seul** `ftm_write()` par ligne. Pas de `printf`, pas de `snprintf`.

**Done quand** : sur le `fake_port` (adresses déterministes), la sortie est comparée
caractère par caractère à une chaîne attendue.

---

### ✅ Phase 9 — Robustesse + POSIX (validation, errno, famille alignée)
*« In no way can your function lead to undefined behaviour or segv. »*

Cas à couvrir dans `ftm_guard.c` :
- `free` d'un pointeur jamais alloué → détecté, pas de crash
- double `free` → détecté via le flag du header
- `free` au milieu d'un bloc → détecté
- `malloc(SIZE_MAX)` → `NULL` proprement
- échec de `mmap` (simulé par `fake_fail_after`) → `NULL`, pas de crash

**Le piège qui fait planter `LD_PRELOAD=./libft_malloc.so vim`** : tu remplaces `malloc`,
`free`, `realloc`… mais pas `aligned_alloc`, `posix_memalign`, `memalign`, `valloc`,
`malloc_usable_size`. Un programme qui appelle `posix_memalign` obtient un pointeur de la
glibc, puis le passe à **ton** `free`. Deux réponses :
- **(a)** implémenter toute la famille ;
- **(b)** valider dans `free()` que le pointeur appartient à une de tes zones, et ignorer
  sinon (fuite, mais pas de crash).

Fais **(a) et (b)**. Peu de gens gèrent ça, et c'est exactement ce qui distingue un projet
« ça marche sur mes tests » d'un projet « ça marche sur `vim` ».

**Done quand** : `test_errors.c` couvre les 5 cas et `LD_PRELOAD` sur `ls -la` passe.

---

### 🔒 Point de contrôle — la partie obligatoire doit être PARFAITE ici
Avant d'attaquer les bonus : phases 0→9 vertes, plus le `ftm_check_heap()` et le fuzz de
la phase 13 (au moins la version courte). Les bonus ne sont pas évalués si l'obligatoire
ne l'est pas.

---

### ⬜ Phase 10 — Bonus B1 : thread-safe
- `port/ftm_lock_pthread.c` : un `pthread_mutex_t` global initialisé **statiquement** avec
  `PTHREAD_MUTEX_INITIALIZER` — pas de `pthread_mutex_init` dynamique, qui recréerait un
  problème d'ordre d'initialisation
- Réfléchis à la granularité (mutex global vs une arena par thread) : c'est **la** question
  de soutenance sur ce bonus. Le mutex global est le bon choix par défaut ; sache dire
  pourquoi, et ce que coûterait l'alternative.

**Done quand** : 8 threads × 50 000 opérations aléatoires ⇒ `ftm_check_heap()` vert et
aucun chevauchement de pointeurs.

**Piège** : `fork()`. Si un thread tient le lock au moment du fork, l'enfant hérite d'un
mutex verrouillé à vie. `pthread_atfork()` règle ça — bonus du bonus, mais ça marque.

---

### ✅ Phase 11 — Bonus B2 : variables d'environnement
| Variable | Effet |
|---|---|
| `FT_MALLOC_SCRIBBLE=1` | remplit le payload libéré de `0xDE` → détecte l'use-after-free |
| `FT_MALLOC_PERTURB=<n>` | remplit le payload alloué de `n` → détecte l'usage non initialisé |
| `FT_MALLOC_GUARD=1` | canaries avant/après le payload, vérifiées au `free` |
| `FT_MALLOC_LOG=<fd>` | trace chaque opération (un **fd**, pas un chemin : `open` + buffering allouerait) |
| `FT_MALLOC_ABORT=1` | `ftm_fatal()` au lieu de tolérer une erreur détectée |
| `FT_MALLOC_HISTORY=<n>` | garde les `n` dernières opérations pour `show_alloc_mem_ex` |

Lecture **une seule fois**, à l'init, dans `port/ftm_env.c`. `getenv` n'alloue pas sur
glibc — vérifie-le quand même, c'est le bon réflexe.

Les hooks posés en phase 5 deviennent simplement actifs : **aucune ligne de `ftm_heap.c`
ne change**. C'est le test que l'anticipation des bonus a fonctionné.

⚠️ `FT_MALLOC_GUARD` change la taille réelle des blocs → ça passe par `ftm_align.c`, pas
par un bricolage dans `ftm_heap.c`.

---

### ✅ Phase 12 — Bonus B3 : `show_alloc_mem_ex()`
- Journal circulaire des N dernières opérations (`ftm_history.c`, alimenté par les hooks)
- Hexdump : `offset | 16 octets hex | ASCII`
- Statistiques : nb de zones, nb de blocs, fragmentation (plus grand bloc libre / total
  libre), compteurs `mmap`/`munmap`
- `arch/x86_64/ftm_frame.S` : remontée de la chaîne `%rbp` → « quel site a alloué ce bloc »
  (les binaires de test se compilent avec `-fno-omit-frame-pointer`)

---

### ✅ Phase 13 — Bench + tuning (m=2048 retenu)
- **`ftm_check_heap()`** — la fonction la plus rentable du projet :
  - les blocs d'une zone couvrent exactement la zone, sans trou ni chevauchement
  - `b->next->prev == b` pour tout bloc
  - **aucun couple de blocs libres adjacents** (⇒ le coalescing est correct)
  - tout payload aligné sur `FTM_ALIGNMENT`
  - compilée conditionnellement (`-DFTM_DEBUG`) pour ne pas plomber la release
- **Fuzz avec oracle** : un tableau `{ptr, size, seed}` en parallèle ; à chaque opération
  aléatoire on relit le contenu et on appelle `ftm_check_heap()`
- **Bench** (`rdtsc`) : nb de syscalls, cycles/op, fragmentation, sur trois profils
  (beaucoup de petits / mix / beaucoup de gros). **C'est ici qu'on re-tune `n` et `m`** avec
  des chiffres, et qu'on tranche first-fit vs best-fit.
- Comparaison contre la glibc sur le même bench — tu ne gagneras pas, mais savoir *de
  combien* et *pourquoi* est excellent en soutenance.

**Done quand** : 1 000 000 d'opérations aléatoires sans une seule violation d'invariant.

---

### ✅ Phase 14 — Intégration réelle (validée au fil de l'eau)
- `LD_PRELOAD=./libft_malloc.so ls -la`
- puis `vim`, `python3 -c 'print(1)'`, `git status`
- puis un programme multithread maison
- `ltrace` pour vérifier qu'aucun `malloc` libc ne passe
- Vérifier la recompilation incrémentale (toucher un `.h` → seuls les `.o` concernés
  se recompilent)
- Relire tout le code avec l'œil « clean code » du sujet

**Done quand** : les quatre binaires tournent sans crash ni fuite anormale.

---

## 7. Ordre de travail

```
0 → 1 → 2 → 3 → 4 → 5 → 6 → [ftm_check_heap() ici] → 7 → 8 → 9 → 13(court)
                                                         ↑ obligatoire PARFAIT
                                                    → 10 → 11 → 12      ← bonus
                                                    → 13(bench+tuning)
                                                    → 14
```

---

## 8. Annexe — hygiène gratuite (utile plus tard, hors périmètre ici)

Quatre règles que l'architecture ci-dessus impose **déjà** pour de bonnes raisons propres
au projet malloc. Elles ont un effet de bord : `core/` reste compilable dans un
environnement sans libc. Si tu veux un jour réutiliser cet allocateur ailleurs (KFS-3), il
suffira d'écrire une autre implémentation des 9 fonctions de `ftm_port.h`.

1. **Aucun appel système hors de `port/`** — raison malloc : c'est ce qui permet le
   `fake_port` et donc des tests déterministes.
2. **Aucune sortie hors de `ftm_show.c`** (via `ftm_write`) — raison malloc : `printf`
   alloue, donc récursion infinie.
3. **Aucun `#include <...>` de la libc dans `core/`** — raison malloc : garantir qu'aucun
   `malloc` libc ne se glisse dans le code.
4. **Aucune allocation dynamique pour les métadonnées** : tout vit *dans* les zones —
   raison malloc : c'est littéralement la contrainte n°3 du sujet.

Rien à faire de plus maintenant. On verra KFS-3 le moment venu.
