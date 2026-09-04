# Journal des décisions d'architecture

> On n'efface jamais une entrée. Si une décision change, on ajoute une entrée qui
> référence l'ancienne.

## ~~D1 — Python comme implémentation de référence~~ — ❌ ANNULÉE (2026-09-01)
Superseded by **D7**. Charles a tranché : pas de Python.

---

## D2 — Une seule couche connaît la plateforme (2026-09-01)
**Décision.** Un contrat `core/ftm_port.h` de 9 fonctions : `ftm_page_size`,
`ftm_map_pages`, `ftm_unmap_pages`, `ftm_lock`, `ftm_unlock`, `ftm_write`, `ftm_memcpy`,
`ftm_memset`, `ftm_fatal`. Trois implémentations : `port/posix/`, `tests/fake_port.c`,
et plus tard `port/kernel/`.

**Pourquoi.** Bénéfice immédiat sur le projet malloc : le `fake_port` distribue la mémoire
depuis un pool statique, donc les adresses sont **déterministes** et la sortie de
`show_alloc_mem` devient testable au caractère près ; et on peut simuler un échec de `mmap`
à volonté pour vérifier la contrainte « jamais d'UB ». Effet de bord : `core/` reste
portable ailleurs.

---

## D3 — Liste de blocs doublement chaînée, en ordre d'adresse (2026-09-01)
**Décision.** Dans chaque zone, tous les blocs — alloués **compris** — forment une liste
doublement chaînée en ordre d'adresse croissante.

**Pourquoi.** Le bonus « défragmentation » exige de connaître les voisins physiques d'un
bloc libéré. Avec cet ordre, la fusion est O(1). Une free-list classique rendrait le
coalescing O(n) ou impossible. Décidé dès le début car non-rétrofittable.

---

## D4 — Taille d'en-tête calculée, alignement `2 * sizeof(void*)` (2026-09-01)
**Décision.** `FTM_ALIGNMENT = 2 * sizeof(void *)` (16 sur x86-64, 8 sur i386) et
`FTM_BLOCK_HDR = ALIGN_UP(sizeof(t_block), FTM_ALIGNMENT)`.

**Pourquoi.** L'exemple de `show_alloc_mem` du sujet montre un écart de `0x20` (32) entre
l'adresse de zone et le premier payload — ce qui correspond exactement à `sizeof(t_block)`
sur x86-64. Mais en i386 (KFS-3) la même structure fait 16 octets. Hardcoder 32 casserait
le portage. Un `_Static_assert` vérifie la divisibilité par l'alignement.

---

## D5 — `n=128`, `m=1024`, `N` et `M` dérivés (2026-09-01)
**Décision.** `FTM_TINY_MAX=128`, `FTM_SMALL_MAX=1024`. Les tailles de zone ne sont pas des
constantes : `FTM_ZONE_SIZE(maxalloc, page) = ALIGN_UP(ZONE_HDR + 100*(BLOCK_HDR+maxalloc), page)`.

**Pourquoi.** La contrainte « ≥ 100 allocations par zone » devient garantie par
construction. Re-tuner `n`/`m` en phase 13 ne peut pas casser la conformité.

**Statut.** Valeurs initiales, à re-tuner avec des mesures (bench phase 13).

---

## D6 — Les hooks de bonus existent dès la phase 5 (2026-09-01)
**Décision.** `ftm_lock()`/`ftm_unlock()` et les hooks `ftm_on_alloc/on_free/on_error`
sont appelés depuis `ftm_heap.c` dès la première version, alors qu'ils ne font rien.

**Pourquoi.** Les bonus B1 (thread-safe) et B2 (env vars) deviennent le remplacement d'une
implémentation, pas une chirurgie dans tout le code.

---

## D7 — C (+ ASM ciblé), pas de Python (2026-09-01) — *remplace D1*
**Décision.** Tout le projet en C. De l'ASM à trois endroits seulement : `rdtsc` pour le
bench (phase 13), remontée de `%rbp` pour l'historique de `show_alloc_mem_ex` (phase 12),
et éventuellement `rep movsb` sur le chemin chaud de `realloc` — **seulement si le bench
prouve un gain**. L'ASM vit sous `arch/x86_64/`, jamais d'`#ifdef` d'architecture dans le
code métier.

**Pourquoi.** Le livrable du sujet est une `.so` interposable par des binaires existants ;
seul un langage compilé nativement sans runtime peut le produire.

**Corollaire.** Le chemin obligatoire reste en C pur + `libft`. L'ASM est cantonné au
bonus et à la perf, pour ne rien avoir à justifier sur la partie notée en premier.

---

## D8 — Deux cibles de build : `.so` (livrable) et `.a` (tests) (2026-09-01)
**Décision.** `libft_malloc_$(HOSTTYPE).so` = `core/` + `port/posix/` + `src/malloc.c`
(exporte `malloc`/`free`/`realloc`). `libftm_core.a` = `core/` + `tests/fake_port.c`
(expose `ftm_alloc`/`ftm_release`, pas `malloc`).

**Pourquoi.** Si les tests unitaires linkent la `.so`, ils remplacent le `malloc` de la
libc et le moindre `printf` de test transite par l'allocateur en cours de debug —
ingérable. Avec le `.a` et le préfixe `ftm_`, aucune collision de symbole : les tests
peuvent utiliser `printf`, `assert`, tout.

**Bénéfice secondaire.** C'est exactement le même `core/` que KFS-3 linkera.

---

## D9 — `core/` ne connaît que `"ftm_stdint.h"` (2026-09-01)
**Décision.** Aucun `#include <...>` de la libc dans `core/`. Une indirection
`ftm_stdint.h` fournie par chaque port : `<stdint.h>`/`<stddef.h>` côté POSIX,
`"types.h"` de KFS côté kernel.

**Pourquoi.** Garantir mécaniquement qu'aucun `malloc` de la libc ne se glisse dans le
cœur : si `core/*.c` compile sans aucun header système, c'est prouvé, pas espéré. Le
`-Icore -Iport` du Makefile suffit à faire résoudre `ftm_stdint.h`.

**Note.** `<stdint.h>` et `<stddef.h>` sont des headers *freestanding* — fournis par le
compilateur, pas par la libc — donc l'indirection ne coûte rien fonctionnellement. Elle
laisse en prime la porte ouverte à un environnement où `size_t` est défini autrement
(c'est le cas de `~/kfs-1/includes/types.h`, qui fait `typedef u32 size_t`).

---

## D10 — Initialisation paresseuse, pas de constructeur ELF (2026-09-01)
**Décision.** Pas de `__attribute__((constructor))`. L'arena s'initialise au premier appel,
sous lock, via un drapeau `g_heap.initialized`.

**Pourquoi.** En `LD_PRELOAD`, l'ordre des constructeurs entre bibliothèques n'est pas
garanti : `malloc` peut être appelé par le loader ou par le constructeur d'une autre lib
avant que le tien ne tourne.

**Corollaire.** Interdiction absolue d'appeler `printf` depuis la lib — `printf` alloue,
donc récursion infinie. D'où `ftm_fmt.c` (formatage maison) + `ftm_write` → `write(2)`.

---

## D11 — L'arena est un paramètre, pas un `static` implicite (2026-09-01)
**Décision.** En interne, `ftm_alloc(t_heap *a, size_t n)`. Le `static t_heap g_heap`
du shim reste l'unique instance exposée.

**Pourquoi.** Testabilité : chaque test unitaire travaille sur une **arena neuve et
isolée**, au lieu de subir l'état laissé par le test précédent. Sans ça, l'ordre des tests
devient significatif et le débogage des phases 6-7 (coalescing, realloc) vire au cauchemar.
Coût : un paramètre.

*(Le sujet autorise « a global variable to manage your allocations » — une seule instance
est exposée, la conformité est respectée.)*

---

## D12 — Système de build calqué sur woody_woodpacker (2026-09-01)
**Décision.** On reprend l'architecture de build de `~/woody_woodpacker` :
- `Makefile` (cibles + règles) + `Makefile.vars` (variables, `include Makefile.cfg`)
  + `Makefile.msg` (macros de log `qcmd`/`bcmd`/`msg`/`rmsg`/`emsg`, bascule `MAKE_FULL_LOG`)
  + `Makefile.cfg` **généré** par `./configure.sh` (autoconf-like : `DEBUG`, `OPTIMIZE`,
  `OBJDIR`, `OUTDIR`, `ARCH`, `CMOREFLAGS`, `LDMOREFLAGS`).
- Objets nommés `$(OBJDIR)/%.c.o`, deps `%.c.d` via `-MMD`, `-include $(DEPS)`.
- Sources sous `src/` (avec sous-dossiers `core/`, `port/`, `arch/`), headers sous
  `include/`. `$(wildcard src/*.c) $(wildcard src/**/*.c)`.
- `tests/` avec son propre `Makefile` + `run_tests.sh`.

**Pourquoi.** C'est le standard que Charles utilise déjà ; cohérence entre ses projets,
et le mécanisme `configure` + `.cfg` gère proprement debug/release sans éditer le Makefile.

**Deux adaptations assumées vs woody :**
1. woody produit un **exécutable** linké `-pie`. Nous produisons une **bibliothèque
   partagée** `libft_malloc_$(HOSTTYPE).so` (règle de link `-shared`, + symlink
   `libft_malloc.so`). Donc `BIN`/`BIN_PATH` deviennent `NAME`/`NAME_PATH`, et
   `LDFLAGS` porte `-shared` au lieu de `-pie`.
2. woody n'a pas notre contrainte D8 (symboles `malloc` vs `ftm_*`). Les tests unitaires
   vivent donc dans `tests/` avec leur **propre** build (archive `libftm_core.a` en
   `ftm_*` + fake port), séparé du build racine — au lieu des tests d'intégration
   « compile un binaire, packe-le, lance-le » de woody. Les tests d'intégration
   `LD_PRELOAD` (phase 14) reprendront, eux, le style `run_tests.sh`.

On ne reprend PAS de woody : `resources/` (blobs embarqués), `stub/`, le double build
32/64 bits de packing. `ARCH` reste dans `configure` mais ne sert qu'à choisir
`arch/x86_64` vs `arch/i386` et `-m32/-m64` (utile plus tard, pas en phase 0).

---

## D13 — libft intégrée via `thirdparty/libft.dep` (2026-09-01)
**Décision.** La libft de Charles (`git@github.com:CodeWithCharles/42_libft_full`) est
un **sous-module git** sous `thirdparty/libft`, branchée par un fichier
`thirdparty/libft.dep` conforme au contrat de la macro `get-lib-info` de woody :
`LIBA_libft`, `INCDIR_libft`, `LIBDIR_libft`, `DEP_LD_LIBS_libft`, et les cibles
`MAKE_RULE_libft` / `CLEAN_RULE_libft` / `FCLEAN_RULE_libft` (qui délèguent à
`$(MAKE) -C thirdparty/libft`).

**Pourquoi.** C'est exactement l'usage prévu par le `thirdparty/` de woody (aujourd'hui
vide, `LIBS :=`). Le sous-module satisfait aussi l'exigence du sujet malloc : « submit
your folder libft including its own Makefile ».

**Point ouvert.** Le sujet dit « at the root of your repository ». `thirdparty/libft`
n'est pas strictement la racine. À trancher : soit un symlink `libft -> thirdparty/libft`,
soit déplacer le sous-module à `./libft` et pointer le `.dep` sur `../libft`. Décision
reportée, sans impact sur la phase 0.

**Point à vérifier côté libft.** Son Makefile doit compiler en `-fPIC` (sinon le link de
la `.so` casse : `relocation R_X86_64_32S ... recompile with -fPIC`) et exposer au moins
`ft_memcpy`, `ft_memmove`, `ft_memset`, `ft_bzero`, `ft_putstr_fd`/équivalent d'écriture.

---

## D14 — Style de code : anglais, commentaires au minimum (2026-09-01)
**Décision.** Tout le code en anglais (identifiants, messages, commentaires). Commentaires
réduits au strict minimum : le code doit se documenter lui-même par des noms explicites
qui « crient » leur intention. Les rares commentaires restants sont en anglais.

**Portée.** Les commentaires que Claude fournit dans les extraits sont **facultatifs** :
Charles décide de les garder ou non ; ils ne sont pas un critère de validation. Les
extraits privilégient donc des noms parlants (`ftm_block_split`, `zone_is_fully_free`,
`round_up_to_alignment`) plutôt que des blocs de commentaires.

---

## D15 — Nommage : `t_heap`, `FTM_*_HEADER_SIZE`, préfixe FTM (2026-09-01)
**Décision.** Le conteneur global (les 3 listes de zones + état) s'appelle **`t_heap`**
(`s_heap`, instance `g_heap`), pas `t_heap`. Les tailles d'en-tête s'écrivent en clair :
`FTM_BLOCK_HEADER_SIZE`, `FTM_ZONE_HEADER_SIZE` (fini `HDR`). Le vérificateur devient
`ftm_check_heap`. Préfixe `FTM`/`ftm_` = ft_malloc, sur tout symbole interne.

**Pourquoi.** « arena » traîne deux faux sens : le multi-arena de la glibc (plusieurs
contextes par thread — on n'en a qu'un) et l'idiome « un gros bloc qu'on double ». `t_heap`
dit ce que c'est sans ambiguïté. `HDR` violait D14 (les noms doivent crier). Remplace la
terminologie de [[D11]] (`t_heap` → `t_heap`) et [[D8]] (garde `ftm_*` pour les tests).

---

## D16 — malloc(0) renvoie un pointeur unique libérable (2026-09-02)
**Décision.** `malloc(0)` ne renvoie pas `NULL` mais un pointeur valide, unique et
libérable — on aligne la demande à la plus petite taille possible (`size=0 → 1 → 16`).

**Pourquoi.** Comportement de la libc de référence (glibc). Beaucoup de programmes réels
font `free(malloc(0))` en supposant un pointeur non-NULL ; renvoyer `NULL` casse `vim` &
consorts. Confirmé par Charles (« on se base sur la lib de référence »).

**Idem à prévoir :** `realloc(p, 0)` (phase 7) suivra la même logique de référence
(free + pointeur minimal).

---

## D17 — ftm_heap_reset & param heap : ajustement de D11 (2026-09-02)
**Décision.** Plutôt que de faire circuler un `t_heap *` dans toutes les signatures
(idée initiale de [[D11]]), on garde un `static t_heap g_heap` unique et on expose
`ftm_heap_reset()` + `ftm_heap_instance()` pour les tests.

**Pourquoi.** L'isolation des tests (chaque test repart d'un heap vierge) était le seul
vrai besoin de D11 une fois KFS-3 hors périmètre. Un reset suffit, et ça garde le sujet
satisfait (une seule variable globale). Signatures plus simples.

---

## D18 — ftm_check_heap compilé sous -DFTM_DEBUG (2026-09-02)
**Décision.** Le vérificateur d'invariants `ftm_check_heap()` (écrit dès la phase 6, pas
la 13) est défini seulement sous `-DFTM_DEBUG`. Les tests compilent avec ce flag ; la
`.so` de release ne l'embarque pas. Prototype déclaré en permanence dans `ftm_internal.h`.

---

## D19 — realloc(ptr, 0) = free(ptr) puis pointeur minimal (2026-09-02)
**Décision.** `realloc(ptr, 0)` libère `ptr` et renvoie un pointeur minimal valide et
libérable (équivalent `malloc(0)`), pas `NULL`. Cohérent avec [[D16]].

**Pourquoi.** Symétrie avec la politique `malloc(0)` et comportement de la lib de référence.

---

## D20 — show_alloc_mem affiche la taille DEMANDÉE (option 2) (2026-09-02)
**Décision.** On ajoute `size_t request_size` à `t_block` et `show_alloc_mem` affiche cette
taille (celle passée à malloc), pas la taille alignée. La plage affichée est
`payload → payload + request_size`. En-tête de bloc : 32 → **48 octets** (align 16).

**Pourquoi.** Charles veut l'affichage factuel. Note : `show_alloc_mem` n'est PAS POSIX
(fonction inventée par le sujet) ; son cousin standard `malloc_usable_size` renvoie la
taille utilisable — on aurait pu s'en réclamer (option 1), mais l'option 2 est plus
parlante. L'exemple du sujet n'aligne d'ailleurs pas ses payloads (il se contredit), donc
ses adresses ne sont pas une spec. `request_size` est purement cosmétique : `ftm_check_heap`
et toute la géométrie continuent d'utiliser `payload_size` (aligné). N et M restent dérivés,
donc la contrainte « ≥ 100 allocs » s'adapte automatiquement au nouvel en-tête.

---

## D21 — Objectif POSIX maximal (2026-09-02)
**Décision.** La phase 9 vise la conformité POSIX la plus large possible :
- `errno = ENOMEM` sur tout échec d'alloc (malloc/calloc/realloc), `EINVAL` pour
  `posix_memalign` mal appelé — posé **dans le shim** (`src/malloc.c`), jamais dans `core/`
  (errno est un concept userspace, cf. D2) ;
- famille alignée : `posix_memalign`, `aligned_alloc` (+ extensions `memalign`, `valloc`,
  `pvalloc`, `malloc_usable_size`, `reallocarray`) ;
- validation de pointeur dans free/realloc (double-free, pointeur étranger, milieu de bloc) ;
- `realloc(p,0)` : POSIX le laisse implementation-defined (UB en C23) → on garde D19
  (free + pointeur minimal), c'est un des choix autorisés.

**Limite connue (à défendre) :** un alignement demandé > 16 (ex. `aligned_alloc(4096,…)`)
oblige à sur-allouer et décaler le payload dans le bloc, avec l'offset stocké juste avant
le pointeur rendu pour retrouver l'en-tête. Seul point réellement délicat.

---

## D22 — Stratégie leaks : compteurs map/unmap, pas valgrind (2026-09-02)
**Décision.** On vérifie l'absence de fuite de zones via les compteurs `map_count`/
`unmap_count` du fake port : après workload + free total + `ftm_heap_reset()`, on doit
avoir `map_count == unmap_count`. Test dédié `tests/test_leak.c`.

**Pourquoi.** `valgrind` sous `LD_PRELOAD` ne teste PAS notre allocateur : valgrind
remplace lui-même malloc/free plus bas que LD_PRELOAD, notre `.so` n'est pas exercé. Nos
compteurs sont la mesure fiable, à la granularité de la zone.

---

## D23 — Famille alignée : sur-allocation + tag avant le pointeur (2026-09-02)
**Décision.** `posix_memalign`/`aligned_alloc`/`memalign`/`valloc`/`pvalloc` :
- alignement ≤ 16 → `ftm_alloc` normal (déjà 16-aligné) ;
- alignement > 16 → sur-allouer `size + alignment + sizeof(t_align_tag)`, placer un pointeur
  aligné dedans, et écrire un `t_align_tag {magic, base}` juste AVANT ce pointeur.
`free`/`realloc`/`malloc_usable_size` détectent le tag (`FTM_ALIGN_MAGIC`) quand la
validation normale échoue, et retrouvent le bloc `base`. errno=EINVAL/ENOMEM selon POSIX.

## D24 — Verrou pthread dans le port, atfork, tests via wrap manuel (2026-09-02)
ftm_lock/unlock reels dans src/port/ftm_lock_pthread.c (mutex statique + pthread_atfork via
pthread_once). Verrou pris par le shim, pas par core. Archive de test compile aussi ce
fichier (stubs vides retires de fake_port.c). Build -pthread.

## D25 — Debug via env : contrat 10e fonction ftm_debug_load (2026-09-02)
Config debug = t_debug static dans core/ftm_debug.c (getter ftm_debug()). Chargee par la
10e fonction du contrat ftm_debug_load() : POSIX lit getenv (src/port/ftm_env.c), fake port
= no-op (tests pilotent ftm_debug() a la main). Appelee 1x depuis heap_init_if_needed.
Vars : FT_MALLOC_SCRIBBLE (0xDE au free), FT_MALLOC_PERTURB=n (au alloc), FT_MALLOC_GUARD
(canari dans [request_size, payload_size), guard force +ALIGNMENT a l'alloc), FT_MALLOC_ABORT
(ftm_fatal sur erreur). Hooks ftm_on_alloc/on_free deja poses en phase 5 -> zero ligne de
logique d'alloc modifiee (anticipation D6 validee). ftm_heap.c inclut desormais ftm_port.h.

## D26 — show_alloc_mem_ex : hexdump + stats + history (2026-09-02)
show_alloc_mem_ex reutilise le parcours de show_alloc_mem via drapeau statique g_extended
dans print_block. Hexdump : min(request_size, 64) octets, format "16 hex | ASCII".
Stats : zones, blocs, free bytes, largest free, map_calls, munmap_calls.
History : ring buffer 256 entrees dans core/ftm_history.c, alimente par les hooks
ftm_on_alloc/on_free (A/F, ptr, size). Le fuzz phase 13 (fait en 9) reste vert avec les
hooks actifs = anticipation D6 confirmee.

## D27 — Tuning final : m=2048 (2026-09-02)
**Décision.** `FTM_SMALL_MAX = 2048` (au lieu de 1024 initial). `FTM_TINY_MAX = 128` conservé.

**Mesures** (ns/op, bench 3 profils, LD_PRELOAD, page 4KB, x86_64 Linux) :

| profil                           | glibc | m=1024 | m=2048 | m=4096 |
|----------------------------------|------:|-------:|-------:|-------:|
| small  (1-128 B, no realloc)     |  12   |  177   |  **161** |  175   |
| mixed  (0-2048 B, 30% realloc)   |  32   |  250   |  **183** |  195   |
| large  (2-10 KB, no realloc)     |  53   | 3051   |  **3116**| 5362   |

**Pourquoi 2048.** Optimum sur les 3 profils. Amelioration nette mixed (-27%) et small (-9%),
regression negligeable large (+2%). m=4096 gagnait sur mixed mais degradait large de +76%
(thrashing de zones SMALL de 400KB avec des allocs de 2-4KB).

**Non fait, cite en soutenance comme piste :** cache LRU des zones LARGE liberees (au lieu
de munmap immediat + remmap au prochain malloc). Gain estime -40 a -60% sur large. Non
implemente pour ne pas prendre le risque d'un bug tardif.

**Ratio face a glibc reste x3 a x15 : defendable.** glibc a tcache (per-thread lock-free),
seuil mmap dynamique, et arenas multiples — tous incompatibles avec la contrainte du sujet
"une variable globale + une pour le thread-safe".

## D28 — Le briefing se trompe : le scan LARGE n'échouait PAS à 100 % (2026-09-04)

**Constat mesuré (pas 2 de la session, A/B sur la machine de Charles).**

| variante                 | ns/op (médiane 3 runs) | mmap   | munmap | syscalls |
|--------------------------|-----------------------:|-------:|-------:|---------:|
| avec scan (état d'avant) |                  3 418 | 17 190 | 17 167 |   34 357 |
| sans scan (pas A seul)   |                  6 055 | 50 147 | 50 124 |  100 271 |

Le pas A **seul** est une régression de +77 %, pas un gain « modeste » comme l'annonce
`mentor-large-perf.md`. Le nombre de syscalls est multiplié par 2,9.

**Pourquoi le briefing a tort.** Il postule qu'une zone LARGE vivante est occupée « par
construction (1 bloc) ». Faux, à cause de deux effets cumulés :
1. `ftm_zone_total_size` arrondit à la page → `ftm_block_split` laisse un résidu libre de
   0 à ~4 Ko dans **chaque** zone LARGE (ex. demande 5008 → zone 8192 → bloc 8112 →
   résidu 3056).
2. Si ce résidu est servi à une 2e demande LARGE, la zone porte 2 allocations ; libérer
   l'une ne la rend pas « fully free », donc pas de munmap, et elle laisse un bloc libre
   réutilisable. Ces zones s'accumulent en **cache accidentel** — il absorbait 2/3 des
   allocations LARGE.

**Décision.** On garde le pas A et on enchaîne sur le cache (pas B), qui est le mécanisme
de réutilisation *explicite* et O(1) censé remplacer ce cache accidentel. Mais on ne prend
plus pour argent comptant que A est un gain : **à la fin de la session, mesurer A+B+C
contre B+C seuls** (le `&& kind != FTM_LARGE` est un flip d'une ligne) et trancher sur
les chiffres. Si B+C sans A est meilleur, on retire A.

**Note de méthode.** La variance du bench est de l'ordre de 10 % (baseline mesurée à 3 120
au pas 1, re-mesurée à 3 418 au pas 2, même binaire) : ne pas conclure sur un écart < 15 %
sans plusieurs runs.

## D29 — Le pas A est validé PAR LA MESURE, une fois le cache en place (2026-09-04)

Résolution de la question ouverte de [[D28]]. Médianes sur 3 runs, machine de Charles :

| variante                  | large | small | mixed |
|---------------------------|------:|------:|------:|
| baseline (ni A ni cache)  |  3418 |   176 |   202 |
| cache seul (sans A)       |  1212 |   159 |   179 |
| **cache + A (retenu)**    | **1031** | 155 |   187 |

Le pas A vaut **−77 % isolé** mais **+15 % avec le cache** (intervalles disjoints :
1191-1219 sans A contre 1016-1062 avec). L'hypothèse d'antagonisme est confirmée : le scan
occupe les résidus → les zones ne deviennent jamais « fully free » → le cache est affamé.
A et le cache ne s'additionnent pas, le second **remplace** le premier, en O(32) au lieu
de O(~256).

**Décision : on garde le pas A.** Gain cumulé à ce stade ×3,3 sur la baseline, témoins
`small`/`mixed` neutres. Reste à traiter : `ftm_heap_find_zone` en O(n) (pas 5).

## D30 — Test fantôme : run_tests.sh itère sur les binaires, pas sur les sources (2026-09-04)

`make test` rapportait `PASS test_large_cache` alors que `tests/test_large_cache.c`
n'existe pas sur master. `tests/build/test_large_cache` était un binaire résiduel de la
branche exploratoire `large_opti` (daté du 03/09 18:22 contre 04/09 12:03 pour les
autres) : `git checkout` ne nettoie pas `tests/build/`. Le suite tournait donc à
15 tests réels + 1 faux positif entièrement lié contre du code disparu.

**Correctif immédiat** : suppression du binaire orphelin.
**Correctif structurel à faire** : `run_tests.sh` doit dériver la liste des tests depuis
les sources `test_*.c` et non depuis le contenu de `build/`. Un test dont la source
disparaît doit disparaître du rapport.

## D31 — Page map : résultat final ×14,2 sur le profil LARGE (2026-09-04)

Table de hachage statique (open addressing, sondage linéaire, tombstones) dans `g_heap`,
indexant **chaque page de chaque zone listée** → `ftm_heap_find_zone` en O(1).
8192 entrées × 16 o = 128 Ko en BSS. Repli automatique sur le parcours linéaire si
saturation (`zone_map_disabled`, tout-ou-rien par zone).

**Résultats (médianes, machine de Charles)** :

| profil  | glibc | glibc mmap_thr=1 | baseline | + cache + A | **+ page map** | gain   |
|---------|------:|-----------------:|---------:|------------:|---------------:|-------:|
| `large` |  56.9 |             3737 |     3418 |        1031 |        **240** | **×14,2** |
| `small` |     — |                — |      176 |         155 |            166 | neutre |
| `mixed` |     — |                — |      202 |         187 |            191 | neutre |

**Meilleur que le prototype de référence** (~320 ns/op annoncés dans le briefing).
Face à la glibc placée sous la même contrainte mmap, ft_malloc est désormais **15,6× plus
rapide** ; l'écart ×4,2 restant avec sa config par défaut ne mesure que sa liberté de ne
pas faire de syscall sous 128 Ko.

**Aucune contrainte du sujet cassée** : cache et map vivent dans `g_heap` (une seule
variable globale), zéro métadonnée allouée dynamiquement, et le cache *réduit* le nombre
de `munmap` — ce que le sujet demande explicitement. La garantie « no segv » est conservée
(la map ne remplace pas la validation de pointeur, elle accélère seulement la localisation
de la zone).

## D32 — Étape 1 : compteur de zones, historique conditionnel, et cache TINY/SMALL ABANDONNÉ (2026-09-04)

**Fait.**
- `count_zones` (O(n) sur le chemin du `free`) remplacé par `size_t zone_count[]` dans
  `t_heap`, maintenu au push et au release. Piège rencontré : le décrément doit être placé
  **avant** le bloc `if (kind == FTM_LARGE)`, qui contient un `return` quand la zone part
  au cache — sinon il est sauté dans le cas le plus fréquent. Attrapé par
  `test_free_coalesce`.
- `ftm_history_record` conditionné à un nouveau drapeau `t_debug.history`
  (`FT_MALLOC_HISTORY`) : il tournait à chaque alloc et chaque free même quand
  `show_alloc_mem_ex` n'était jamais appelé.
- Bug latent corrigé dans `ftm_env.c` : `getenv(x) != NULL` testait la *présence*, donc
  `FT_MALLOC_GUARD=0` **activait** le mode. Remplacé par un helper `env_flag` (valeur non
  vide et différente de `"0"`).
- `tests/test_show_ex.c` active désormais `history` : l'en-tête `--- history ---` étant
  imprimé inconditionnellement, le test passait sans qu'aucune entrée ne soit enregistrée.

**Effet mesuré** : neutre à cette échelle (`small` médiane 171 contre ~166 avant, dans le
bruit). Ces gains portent sur quelques ns d'un budget de 170 — justes à faire, pas
observables tant que le `first-fit` domine.

**Cache TINY/SMALL : ABANDONNÉ, sur mesure.** Un profil `sawtooth` (300 allocs de 128 o
puis 300 free, en boucle) a été ajouté au bench pour exhiber le thrashing de zones
supposé. Résultat inverse de l'hypothèse :

| profil     | glibc | ft_malloc | ratio |
|------------|------:|----------:|------:|
| `small`    |  11.6 |     ~170  | ×14,7 |
| `sawtooth` |  10.4 |   **142.6** | **×13,7** |

`strace` : 2030 syscalls pour 200 000 ops (1 %), même ordre que `large` après cache.
Aucun thrashing. Le motif « allouer en masse puis tout libérer » est au contraire
**favorable** au first-fit : après la vague de `free`, tout se recoalesce en gros blocs,
donc l'allocation suivante trouve immédiatement. Le coût de l'implémentation (renommage de
`ftm_large_cache.c`, trois listes, trois compteurs, retouche de `test_large_cache.c`) n'est
pas justifié par un gain nul. Le profil `sawtooth` est conservé dans le bench comme témoin.

## D33 — Free list explicite : ×3,1 sur `small`, ×2,1 sur `mixed` (2026-09-04)

**Écart au plan.** `perf-roadmap.md` prévoyait une free list **globale par kind** dans
`t_heap`. Retenu à la place : **une free list par zone** (`t_block *free_list` dans
`t_zone`). Trois raisons, toutes vérifiées à l'écriture :
1. **Sûreté** — quand une zone part au cache ou est détruite, sa liste meurt avec elle.
   Aucun pointeur ne survit vers une zone non listée : le piège n°1 annoncé dans la
   roadmap (bloc orphelin servi depuis une zone que la page map ne reconnaît plus)
   disparaît par construction, il n'y a rien à délier dans `heap_release_zone_if_free`.
2. **Simplicité** — tous les appelants de `split`/`coalesce_next` ont déjà la zone en
   main, donc passage par paramètre plutôt qu'un état global de plus.
3. **Même gain** — 2 zones × ~11 blocs libres = 22 maillons parcourus dans les deux
   variantes, contre ~280 avant.

Prix payé : `heap_reserve_block` continue d'itérer sur les zones pour TINY/SMALL
(négligeable, 2 zones).

**Les trois contrats** qui rendent le câblage mécanique :
- un bloc libre d'une zone listée est **toujours** dans `zone->free_list` (donc
  `ftm_free_list_find` ne teste plus `is_free`) ;
- `ftm_block_split(zone, block, n)` suppose `block` **hors** liste, et y pousse le résidu ;
- `ftm_block_coalesce_next(zone, block)` **délie elle-même** le voisin absorbé
  (l'ordre compte : délier avant de modifier les tailles, sinon on manipule un nœud
  logé dans un payload déjà absorbé).

**Résultats (médianes, 3 runs)** :

| profil  | glibc | avant | **après** | gain   | ratio vs glibc     |
|---------|------:|------:|----------:|-------:|-------------------:|
| `small` |  11.6 |  ~170 |    **55** | **×3,1** | ×4,7 (était ×14,7) |
| `mixed` |  29.3 |  ~191 |    **91** | **×2,1** | ×3,1 (était ×6,5)  |
| `large` |  55.6 |  ~230 |       216 | neutre | ×3,9               |

`large` neutre : attendu, son chemin ne consulte aucune free list depuis le pas A (D29).

**Changement de comportement observable** : `FT_MALLOC_SCRIBBLE` n'empoisonne plus les
16 premiers octets d'un bloc libéré — ils portent le chaînage, écrit après le poison.
`tests/test_debug.c` vérifie désormais à partir de l'offset 16. `tests/test_zone.c` a dû
délier explicitement dans `test_find_free` : marquer un bloc utilisé sans le délier crée
un état que le code de production ne produit jamais (« libre ⟺ chaîné »).

## D34 — Invariant free list dans ftm_check_heap, et un trou de couverture révélé (2026-09-04)

`check_zone` compte les blocs libres via le parcours **par adresse** (source de vérité) et
`check_free_list` compare cette cardinalité à la longueur de la liste. Une seule
comparaison détecte les trois défaillances : bloc jamais poussé (liste plus courte),
bloc poussé deux fois ou jamais délié (plus longue), bloc alloué resté dans la liste
(test `is_free`). La borne `seen <= expected` + `block == NULL` final détecte les
**cycles** — sans elle, un `next_free` incohérent ferait boucler le test indéfiniment
au lieu d'échouer.

**Trou de couverture rencontré.** Première version livrée avec `check_free_list` écrite
mais jamais appelée (`check_zone` avait gardé son ancien `return`) : 16/16 verts, fuzz
200k, et **zéro vérification** de la structure qu'on venait d'écrire. Seul le warning
`defined but not used` l'a signalé — et encore, uniquement dans le build des tests, car
`ftm_check.c` est intégralement sous `#ifdef FTM_DEBUG` donc vide en release.

**À faire** : ajouter `-Werror` aux `CFLAGS` de `tests/Makefile`. Le build des tests est
aujourd'hui moins exigeant que celui de la `.so`, alors que c'est lui qui compile le code
de vérification. Même esprit que D30 : ne pas laisser un signal faible passer pour un
succès.

## D35 — Re-tuning du cache LARGE après l'étape 2 : cap 32 → 64 (2026-09-04)

**Hypothèse de départ (fausse).** Le `best-fit` linéaire de `ftm_large_cache_take` parcourt
jusqu'à `MAX_ZONES` en-têtes situés sur des pages distinctes → un cache miss par maillon →
un cap plus grand devrait *ralentir*. Le balayage montre l'inverse, de façon monotone.

| cap  | ns/op | syscalls / 200k ops | mémoire retenue (pire cas) |
|-----:|------:|--------------------:|---------------------------:|
|    4 |  1311 |                   — |                     ~32 Ko |
|    8 |   788 |                   — |                     ~64 Ko |
|   16 |   441 |                   — |                    ~128 Ko |
|   32 |   220 |                4923 |                    ~256 Ko |
|**64**|**121**|                1061 |                    ~512 Ko |
|  128 |   114 |                 581 |                      ~1 Mo |
|  256 |   112 |                 453 |                      ~2 Mo |
|  512 |   111 |                 368 |                      ~4 Mo |

Ce n'est pas le coût du parcours qui domine mais le **taux de hit** : plus de zones en
cache = moins de syscalls. Point d'inflexion à **64** (temps divisé par deux vs 32) ;
au-delà on double la mémoire pour 6 %.

**Décision : `FTM_LARGE_CACHE_MAX_ZONES` passe de 32 à 64.** Le cap est un plafond, pas une
réservation : un programme qui ne libère jamais 64 zones LARGE n'en retient jamais 64,
donc relever le défaut ne pénalise pas les petits programmes.

**Leçon de méthode.** Le balayage de D28 (avant page map et free list) donnait 32 optimal
et 64/128 *moins bons*. Les structures ayant rendu le CPU par opération bien moins cher,
le poids relatif du syscall a augmenté et l'optimum s'est déplacé. **Un paramètre réglé se
re-règle après chaque changement structurel** — il n'y a pas de valeur juste dans l'absolu.

**Conséquence sur la suite.** L'étape 4.2 (`MADV_DONTNEED` sur les zones en cache) change
de statut : ce n'est plus un confort mémoire, c'est ce qui permettrait de monter le cap
bien au-delà de 64 sans en payer le RSS. À traiter en priorité.

## D36 — `madvise` : mesuré AVANT implémentation, et repoussé (2026-09-04)

Microbenchmark sur une zone de 8 Ko, 50 000 cycles :

| régime                                        | ns/cycle |
|-----------------------------------------------|---------:|
| `mmap` + touch + `munmap` (notre cache miss)  |    6 678 |
| `MADV_DONTNEED` + re-touch                    |    3 427 |
| `MADV_FREE` + re-touch                        |    1 389 |
| réutilisation pure (notre cache hit)          |    **0** |

**Le point décisif : notre taux de hit est déjà de 99,5 %** (à cap=64 : ~530 `mmap` pour
~100 000 allocations). Madviser toutes les zones du cache convertirait 99,5 % de hits
**gratuits** en hits à 1 389 ns → ~700 ns/op au lieu de 116, soit une **régression ×6**.
La piste telle que décrite dans `perf-roadmap.md` §4.2 est invalide.

**Seul design viable : cache à deux étages** — chaud non-madvisé (hits gratuits) + froid
madvisé remplaçant la destruction. Mais son plafond est connu et mesuré : `cap=512` sans
madvise donne 111 ns/op contre 121 à cap=64, donc **le gain vitesse maximal est de 8 %**,
au prix d'un cache à deux niveaux, d'une 11e fonction de port et d'un invariant de plus.

**Décision : repoussé.** Ce n'est pas une optimisation de vitesse mais de mémoire — elle
donnerait la performance d'un cache de 512 zones au coût RSS d'un cache de 64. Sans
problème de RSS avéré (le cap est un plafond, pas une réservation), le rapport
complexité/gain ne le justifie pas. À rouvrir si la mémoire retenue devient un sujet.

**À retenir pour la soutenance** : `MADV_FREE` est 2,5× moins cher que `MADV_DONTNEED`
(1 389 vs 3 427) parce qu'il est paresseux — le noyau ne récupère les pages que sous
pression mémoire. C'est la primitive que privilégie jemalloc.

## D37 — BUG : la page map se désactive irréversiblement sur charge longue (2026-09-04)

Découvert en préparant l'étape 4.3 (réduction de `FTM_ZONE_MAP_CAPACITY`).

**Symptôme.** Probe de 2 000 000 d'opérations LARGE, jeu de pages vivantes ~900 :

| capacité | `zone_map_live` final | MAX_LIVE | map active ? |
|---------:|----------------------:|---------:|:------------:|
|     1024 |                   766 |      768 |  ❌ désactivée |
|     2048 |                  1536 |     1536 |  ❌ désactivée |
|     4096 |                  3072 |     3072 |  ❌ désactivée |
|     8192 |                  4096 |     6144 |  ✅ active     |

Une fois désactivée, `ftm_heap_find_zone` retombe **définitivement** sur le parcours
linéaire O(n) : régression progressive, irréversible, et invisible sur un bench de
200 000 opérations (d'où l'absence de falaise dans le balayage initial, qui ne montrait
un effondrement qu'à 512/1024).

**Cause.** `zone_map_live` compte les **cases vierges consommées** et n'est jamais
décrémenté au retrait ; les pierres tombales ne sont jamais récupérées. Le compteur mesure
donc l'usure cumulée de la table, pas sa charge réelle. Il atteint mécaniquement le seuil,
quel que soit le nombre de pages réellement vivantes (900 ici).

**L'étape 4.3 telle que planifiée aurait aggravé le bug** : passer la capacité à 2048 pour
gagner 96 Ko de BSS aurait fait arriver la falaise 4× plus tôt. Optimisation annulée au
profit du correctif.

**Correctif retenu.**
1. `zone_map_live` compte les entrées **réellement vivantes** : décrémenté dans
   `map_remove_page`, incrémenté à l'insertion (y compris en réutilisant une tombstone).
2. Nouveau compteur `zone_map_tombstones`.
3. Quand `live + tombstones + pages > MAX_LIVE`, **reconstruction en place** : on vide la
   table et on ré-insère toutes les zones depuis `heap->zones[kind]`. Aucun stockage
   temporaire nécessaire — les listes de zones sont la source de vérité. O(pages), rare.
4. `zone_map_disabled` ne se déclenche plus que si `live + pages > MAX_LIVE` **après**
   reconstruction, c'est-à-dire uniquement s'il y a génuinement trop de pages vivantes.

**Leçon.** Un compteur qui ne descend jamais n'est pas un indicateur de charge. Le bug
était invisible aux 16 tests et au bench ; seul un probe long l'a exposé. À retenir pour
la validation : les structures à état accumulé exigent un test de **durée**, pas seulement
de correction.

## D38 — Étape 4.3 : capacité de la page map 8192 → 4096 (2026-09-04)

Après le correctif D37, balayage avec probe compilé à la même capacité que le code testé
(le piège de la première mesure : `tests/Makefile` ne lit pas `CMOREFLAGS`, donc l'archive
et le probe restaient à la capacité par défaut pendant que seule la `.so` variait).

| capacité | BSS    | `zone_map_live` (2M ops) | active | marge |
|---------:|-------:|-------------------------:|:------:|------:|
|      512 |   8 Ko |                        — |   ❌   |     — |
|     1024 |  16 Ko |                        — |   ❌   |     — |
|     2048 |  32 Ko |                      690 |   ✅   | ×2,2  |
| **4096** |**64 Ko**|                     690 |   ✅   | **×4,4** |
|     8192 | 128 Ko |                      690 |   ✅   | ×8,9  |

**Décision : `FTM_ZONE_MAP_CAPACITY` = 4096.** BSS divisée par deux (`sizeof(t_heap)`
131 Ko → ~66 Ko), marge ×4,4 sur la charge observée, perf identique (120 vs 118 ns/op sur
`large`, dans le bruit). 512 et 1024 se désactivent pour la bonne raison : leur MAX_LIVE
(384, 768) est réellement inférieur au pic de pages vivantes.

**Limitation connue, assumée.** `zone_map_disabled` reste **définitif** : une fois posé, on
n'insère plus, donc aucune reconstruction n'a lieu et la map ne se rétablit jamais, même
si la charge redescend. Un pic transitoire coûterait la map pour toute la vie du process.
Le repli (parcours linéaire) reste **correct**, seulement plus lent. Le correctif serait
simple dans son principe (`map_rebuild()` recalcule déjà `live` depuis les listes de zones,
il suffirait de lever le drapeau si ça tient) mais la question du *quand* tenter la
reconstruction ajoute de la machinerie. Avec une capacité dimensionnée à 4× la charge, le
cas est jugé assez improbable pour être documenté plutôt que codé. À rouvrir si un profil
réel le déclenche.

## D39 — Étape 3 (boundary tags) ANNULÉE : arithmétique faite avant d'écrire (2026-09-04)

Calcul des formats d'en-tête possibles, `FTM_ALIGNMENT` = 16 :

| format                        | champs                                  | sizeof | en-tête | zone TINY | allocs/zone |
|-------------------------------|-----------------------------------------|-------:|--------:|----------:|------------:|
| actuel                        | payload, request, next, prev, flags      |     40 |  **48** |   5 pages |         116 |
| sans `request_size`           | payload, next, prev, flags               |     32 |  **32** | **4 pages** |       102 |
| boundary tags + flags séparés | payload, prev_size, flags                |     24 |  **32** |   4 pages |         102 |
| boundary tags packés          | payload\|flags, prev_size                |     16 |  **16** |   4 pages |         113 |

**Les boundary tags ne rapportent rien.** Remplacer `next`/`prev` par de l'arithmétique
fait passer la structure de 32 à 24 octets, mais l'arrondi à 16 réabsorbe intégralement le
gain : même en-tête de 32 que la simple suppression de `request_size`, pour une réécriture
de `ftm_block.c`, `ftm_zone.c`, `ftm_check.c` et `ftm_show.c`.

**La version 16 octets casse le sujet.** Elle exige d'empaqueter les drapeaux dans les bits
bas de la taille, donc de sacrifier `FTM_BLOCK_MAGIC` (24 bits significatifs aujourd'hui).
Or `ftm_block_is_valid` est ce qui rend sûr le `free` d'un pointeur au milieu d'un bloc :
sans lui, on interprète des octets arbitraires comme un en-tête et on libère un bloc
fantôme — exactement le comportement glibc, et exactement ce que le sujet interdit
(« in no way can your function lead to undefined behaviour or segv »). Un magic sur 4 bits
laisserait passer un pointeur invalide sur seize. → déplacé dans `perf-hors-sujet.md`.

**Ce qui reste de l'étape 3** : la suppression de `request_size` seule capture tout le gain
mémoire réel — en-tête 48 → 32, zone TINY 5 → 4 pages (−20 %), surcoût d'une alloc de
128 o de 37,5 % à 25 %. Une ligne, zéro risque. Contrepartie : `show_alloc_mem` afficherait
la taille utilisable (sémantique `malloc_usable_size(3)`) au lieu de la taille demandée,
soit l'inverse de [[D20]]. **Décision de Charles.**

## D40 — V1 mesuré : garder les zones cachées dans la page map (−35 % sur `large`) (2026-09-04)

**Observation.** Sur un cache **hit** LARGE, la zone était retirée de la page map au `put`
puis ré-insérée au `take` — 4 à 6 opérations de hachage par cycle alloc/free pour une zone
qui n'a pas changé d'adresse.

**Prototype (copie du dépôt en scratchpad, dépôt de Charles non modifié)** : ne retirer de
la map qu'au moment de la **destruction** réelle (donc après l'éviction du cache), et ne
pas ré-insérer une zone qui revient du cache.

Résultat : `large` **129 → 84 ns/op (−35 %)**, 15/16 tests verts.

**Le seul test qui tombe est `test_large_cache:96`**, qui assertait
`ftm_zone_map_lookup(large) == NULL` après un `free` — c'est-à-dire l'ancien invariant
« la map ne contient que des zones listées ». C'est cet invariant qu'on change
délibérément ; le test doit suivre le code, pas l'inverse.

**Sûreté de l'invariant relâché.** Un `free` d'un pointeur situé dans une zone *cachée*
trouve désormais la zone au lieu de renvoyer `NULL`. Il reste sans danger :
`ftm_pointer_is_allocated` échoue (le bloc unique d'une zone cachée est libre), puis
`ftm_aligned_base` échoue à son tour, et l'appel est ignoré — même issue qu'avant, par un
chemin différent. `release_block` n'est jamais atteint, donc aucun risque de manipuler une
zone hors de `zones[kind]`.

## D41 — Baseline vitesse + mémoire avant la série V1/V2/V4/V5/M1/M2/M3 (2026-09-04)

Bench instrumenté : `VmPeak` (pic virtuel) et `VmHWM` (pic résident) lus depuis
`/proc/self/status` via `open`/`read` (pas `stdio`, qui rentrerait dans notre propre
malloc pendant la mesure). Médianes sur **5 runs** — 3 runs s'étaient révélés insuffisants,
la dispersion atteignant 20 %.

| profil  | ns/op (médiane/5) | VmPeak   | VmHWM     |
|---------|------------------:|---------:|----------:|
| `small` |          **54,8** | 2 708 Ko | ~1 556 Ko |
| `mixed` |          **97,8** | 2 688 Ko | ~1 788 Ko |
| `large` |         **133,4** | 5 720 Ko | ~4 580 Ko |

**Le bench n'écrit jamais dans la mémoire allouée**, donc `VmHWM` ne mesure que les pages
touchées par l'allocateur lui-même (en-têtes de blocs et de zones). C'est suffisant pour
comparer des variantes, mais ça ne reflète pas la charge d'un vrai programme.

**Bimodalité sur `small`** : deux groupes nets (52-55 et 64-65), probablement de la mise à
l'échelle de fréquence. Un gain inférieur à 15 % y restera indécidable.

## D42 — M1 est finalement faisable SANS compromis : le delta dans les bits hauts de `flags`

Question de Charles : comment la glibc affiche-t-elle la taille demandée si elle ne la
stocke pas ? **Réponse : elle ne l'affiche pas.** `malloc_usable_size(3)` renvoie la taille
*utilisable* (sa doc précise « may be more than the requested size ») ; `mallinfo()` et
`malloc_stats()` ne manipulent que des agrégats de tailles de chunks. L'information est
perdue au retour de `malloc`. Seuls des **outils de debug** la conservent, en assumant le
surcoût : `mcheck` (glibc) ajoute un en-tête, ASan la range dans ses redzones, valgrind
tient des structures fantômes.

**Troisième voie retenue.** Ne pas stocker `request_size` mais le **delta**
`payload_size − request_size`, borné par ~100 (arrondi d'alignement + refus de split +
marge de `FT_MALLOC_GUARD`). Le champ `flags` est un `uintptr_t` dont **les bits 32-63 sont
libres** (état sur 0-7, magic sur 8-31). Le delta y tient largement.

Résultat : `t_block` = `payload_size` + `next` + `prev` + `flags` = 32 octets → en-tête
**32 au lieu de 48**, zone TINY 5 → 4 pages (−20 %), et `show_alloc_mem` affiche toujours
la taille demandée exacte via `payload_size - delta`. **Aucun compromis.**

Adaptation nécessaire : `ftm_block_is_valid` teste `(flags & ~FTM_STATE_MASK) ==
FTM_BLOCK_MAGIC`, ce qui exige des bits hauts nuls. Il faudra masquer explicitement les
bits 8-31 (`FTM_MAGIC_MASK = 0xFFFFFF00`). M1 remis dans la file, après V1/V2/V4/V5.

## D43 — V1 implémenté : zones cachées conservées dans la page map (2026-09-04)

| profil  | baseline | V1        | écart               | VmPeak          |
|---------|---------:|----------:|---------------------|-----------------|
| `small` |     54,8 |      57,4 | +4,8 % (bruit)      | 2708 → 2676 Ko  |
| `mixed` |     97,8 |      92,9 | −5,1 %              | 2688 → 2656 Ko  |
| `large` |    133,4 |  **89,7** | **−32,7 %**         | 5720 → 5688 Ko  |

Médianes sur 5 runs. 16/16 tests. **La mémoire baisse légèrement partout** : on ne retient
aucune zone de plus, on supprime seulement 4 à 6 opérations de hachage par cycle
alloc/free LARGE. Le +4,8 % sur `small` tombe dans la bimodalité identifiée en D41 — ce
profil ne touche jamais au cache LARGE, il ne peut structurellement pas être affecté.

Ratios face à la glibc : ×5,0 / ×3,2 / **×1,6**.

`test_large_cache` documente désormais le nouvel invariant : une zone cachée **reste dans
la map** (retrouvable en O(1) au retour) mais quitte `zones[kind]` et entre dans le cache.
Le test exerce aussi le `free` d'un pointeur dont la zone est en cache.

## D44 — V2 (free lists ségrégées) ANNULÉE : ×7 sur le parcours, 0 % sur le temps (2026-09-04)

**Implémentée intégralement, mesurée, puis remisée** (`git stash`, récupérable).

Les bins font exactement ce qu'on attendait d'eux sur la recherche :

| profil  | maillons/appel avant | après | réduction |
|---------|---------------------:|------:|----------:|
| `small` |                 5,66 |  0,40 |       ×14 |
| `mixed` |                23,16 |  3,25 |        ×7 |
| `large` |                 1,00 |  0,49 |        ×2 |

Et le temps n'a pas bougé — pire, il a régressé :

| profil  | V1   | V2   | écart |
|---------|-----:|-----:|-------|
| `small` | 57,4 | 55,2 | −3,9 % (bruit) |
| `mixed` | 92,9 | 93,5 | **+0,7 % — nul** |
| `large` | 89,7 | 99,0 | **+10,3 %** |
| VmPeak `large` | 5688 Ko | 5764 Ko | +76 Ko |

**Le parcours de free list n'a jamais été le goulot.** L'estimation « ~1,5 ns par maillon »
qui fondait la prévision de −38 % sur `mixed` (catalogue V2) était fausse : une zone TINY
fait 20 Ko et tient en L1/L2, donc le processeur préfetche et spécule à travers 23 maillons
pour un coût quasi nul. **Une réduction ×7 d'un parcours O(n) peut ne rien rapporter — la
complexité algorithmique n'est pas le coût.**

**Et V2 ajoute du travail sur les chemins chauds**, ce qui explique le −10 % sur `large` :
`ftm_block_split` appelle désormais `ftm_free_list_release` (test des deux voisins) là où un
`push` suffisait, et surtout `ftm_bin_index` est appelé au `push` **et** au `unlink`, sa
partie logarithmique étant une boucle qui tourne 4-5 fois pour un payload LARGE. Optimiser
`ftm_bin_index` avec `__builtin_clzl` ramènerait au mieux au niveau de V1, avec 240 octets
de plus par en-tête de zone et un fichier bien plus complexe. Pour un gain nul.

**Réserve honnête** : ces conclusions valent pour nos trois profils, où une zone porte
~22 blocs libres. Un programme très fragmenté (des centaines de blocs libres par zone)
pourrait rendre les bins rentables. On n'invente pas une charge pour justifier du code —
mais le stash est là si une mesure future le demande.

## D45 — LA libft ÉTAIT COMPILÉE EN -O0 : `calloc` ×26 (2026-09-04)

Découvert en cherchant à quantifier V4 (`calloc` sans `memset` sur zone neuve). Un profil
`calloc` ajouté au bench (mêmes tailles que `large`, pour comparaison directe) donne
**5065 ns/op contre 90,6 pour `large`** — un facteur 56, très au-delà du coût d'un `memset`
de 6 Ko.

**Cause racine.** `thirdparty/libft/Makefile` : `CFLAGS := -Wall -Wextra -Werror -fPIC -g`.
**Aucune optimisation.** Le désassemblage de `ft_memset.o` montre chaque variable rechargée
depuis la pile à chaque itération d'une boucle octet-par-octet *descendante* (donc hostile
au préfetcheur). Une bibliothèque de debug linkée dans une bibliothèque de production.

Personne ne l'avait vu parce que les trois profils du bench n'appellent jamais `calloc` :
`ft_memset` n'était sur aucun chemin chaud mesuré. `ft_memcpy` l'était (chemin `realloc`
de `mixed`), mais son coût s'y noyait.

**Quatre variantes mesurées :**

| variante libft                              | calloc | large | mixed | small | symboles libc |
|---------------------------------------------|-------:|------:|------:|------:|---------------|
| `-O0` (état d'alors)                         |   5065 |  90,6 |  92,1 |  57,9 | aucun         |
| `-O2`                                        |    151 |  82,0 |  77,7 |  54,9 | ⚠️ `memset`, `memcpy` |
| `-O2 -fno-tree-loop-distribute-patterns`     |    845 |  80,2 |  76,3 |  53,0 | aucun         |
| **`-O2` idem + `ft_memset`/`ft_memcpy` mot à mot** | **195** | **78,0** | **70,7** | **53,5** | **aucun** |

**Le `-O2` seul est un piège** : la reconnaissance d'idiome de GCC convertit la boucle de
`ft_memset` en **appel à `memset(3)`** (`nm -u` montre `U memset` et `U memcpy`). Or ni l'un
ni l'autre n'est dans la liste des fonctions autorisées du sujet, et un correcteur qui fait
`nm -D` sur la `.so` les verrait. D'où `-fno-tree-loop-distribute-patterns`, qui garde la
boucle — au prix de la vectorisation, d'où les 845 ns.

**Configuration retenue** : `-O2 -fno-strict-aliasing -fno-tree-loop-distribute-patterns`
plus `ft_memset`/`ft_memcpy` réécrits **mot à mot** (`unsigned long` par pas de 8, reliquat
octet par octet, sens ascendant). On récupère 96 % de la performance du SIMD de la glibc
sans en emprunter une ligne.

Note : `-fno-strict-aliasing` est nécessaire car `-O2` révèle une violation latente dans
`src/ft_printf/printers/pointer_printer.c` (`n = *(long *)&ptr;`) que `-Werror` bloque.

**Réserve** : la version mot à mot fait des accès non alignés (`*(unsigned long *)(out + i)`).
Sans danger sur x86-64 et i386 ; sur une architecture à alignement strict il faudrait
aligner la tête d'abord.

**Décision en attente de Charles** : la libft est un sous-module partagé avec ses autres
projets. Modifier son Makefile et ses deux fichiers les affecte tous (positivement, mais
c'est son appel). Alternative : implémenter `ftm_memset`/`ftm_memcpy` optimisés dans
`port/` et ne plus déléguer à la libft — local au projet, mais on perd l'argument
« j'utilise ma libft ».

**V4 (`calloc` sans `memset` sur zone neuve) devient sans objet** : avec un `memset` correct
et un taux de hit de 99,5 % sur le cache LARGE, il ne se déclencherait que sur 0,5 % des
allocations pour économiser ~100 ns. À classer sans suite.

## D46 — État final après D45 (2026-09-04)

Médianes sur 5 runs (`large` re-mesuré, un outlier à 136 écarté) :

| profil   | début de session | final     | gain      | glibc | ratio |
|----------|-----------------:|----------:|----------:|------:|------:|
| `small`  |            176,3 |  **55,2** |    ×3,2   |  11,6 |  ×4,8 |
| `mixed`  |            201,9 |  **70,2** |    ×2,9   |  29,3 |  ×2,4 |
| `large`  |          3 119,9 |  **83,1** | **×37,5** |  55,6 |  ×1,5 |
| `calloc` |          ~5 065  | **201,0** | **×25**   | 117,6 |  ×1,7 |

`VmPeak` inchangé partout (2676 / 2656 / 5688 Ko). 16/16 tests.

**Symboles non définis de la `.so`** (vérifiés `nm -D`) : `abort`, `__errno_location`,
`getenv`, `getrlimit`, `mmap`, `munmap`, `pthread_mutex_lock/unlock`, `pthread_once`,
`__register_atfork`, `sysconf`, `write`. Aucun `memset`/`memcpy`. Tout est soit dans la
liste autorisée, soit justifiable au titre des bonus.

## D47 — M1 fait : en-tête 48 → 32, mais les accesseurs DOIVENT être `static inline` (2026-09-04)

`request_size` supprimé de `t_block` ; la taille demandée est reconstituée depuis le
**delta** `payload_size − request` rangé dans les bits 32-63 de `flags` (état sur 0-7,
magic sur 8-31, cf. D42). `ftm_block_is_valid` masque désormais explicitement
`FTM_MAGIC_MASK = 0xFFFFFF00` au lieu de `~FTM_STATE_MASK`.

Résultat : `sizeof(t_block)` 40 → 32, en-tête **48 → 32**, zone TINY **5 → 4 pages**
(−20 %), densité 23,2 → 25,5 allocations par page (+10 %), zone SMALL 52 → 51 pages.
102 allocations par zone TINY, toujours ≥ 100. **Et `show_alloc_mem` affiche toujours la
taille demandée exacte.**

**Piège majeur rencontré.** Première version avec les accesseurs définis dans
`ftm_align.c` : **régression de 16 à 39 % sur les quatre profils.**

| profil   | avant M1 | accesseurs = fonctions | accesseurs `static inline` |
|----------|---------:|-----------------------:|---------------------------:|
| `small`  |     55,2 |                   63,9 |                   **57,4** |
| `mixed`  |     70,2 |                   84,9 |                   **72,6** |
| `large`  |     83,1 |                  103,4 |                   **83,7** |
| `calloc` |    201,0 |                  278,9 |                  **195,2** |

Ce n'est **pas** le coût de l'appel (un appel ne vaut pas 78 ns) mais la **barrière
d'optimisation** : un appel vers une autre unité de compilation force GCC à supposer que
la fonction peut modifier n'importe quelle mémoire, donc il recharge tout autour du site,
spille des registres et abandonne ses hypothèses. Remplacer un accès à un champ par un
appel opaque, sur un chemin exécuté à chaque `malloc`, coûte bien plus que l'appel.

**Correctif** : les deux accesseurs passent en `static inline` dans `ftm_internal.h`. Le
sujet dispensant de la Norme (« clean code even without norm »), c'est légitime et c'est
l'idiome C standard. M1 devient alors neutre en vitesse (+3-4 % sur `small`/`mixed`, dans
la bimodalité mesurée en D41) pour un gain mémoire réel.

**Limite de portabilité assumée** : le packing suppose `sizeof(uintptr_t) >= 8`
(`_Static_assert` ajouté). Sur i386, `flags >> 32` serait un comportement indéfini ; il
faudrait déplacer le delta en bits 8-15 — il tient dans 8 bits, ne dépassant jamais 94 —
et resserrer le magic sur 16-31. Seul endroit du projet qui suppose des pointeurs 64 bits.

### D47 (suite) — chiffres finaux de M1, mesurés au propre

Première série polluée (load average ~0,7, VS Code + Firefox actifs) : dispersion de 106 à
249 ns/op sur `large`. **Leçon de protocole : vérifier `uptime` avant une campagne de
mesure.** Re-mesure sur 7 runs, machine calme :

| profil   | avant M1 | après M1 | écart   | VmPeak            |
|----------|---------:|---------:|---------|-------------------|
| `small`  |     55,2 |     59,0 | +6,9 %  | 2676 → **2664** Ko |
| `mixed`  |     70,2 |     71,7 | +2,1 %  | 2656 → **2648** Ko |
| `large`  |     83,1 |     86,9 | +4,6 %  | 5688 → **5680** Ko |
| `calloc` |    201,0 |    197,5 | −1,7 %  | 5688 → **5680** Ko |

**Arbitrage assumé : ~4 % de vitesse contre 20 % de mémoire sur les zones TINY.**
Le surcoût vient du décalage/masque/OU de `ftm_block_set_request` à chaque allocation, là
où l'ancien code faisait un simple store — 1 à 2 ns sur un budget de 55.

Le gain mémoire n'apparaît qu'à 8-12 Ko dans le bench parce qu'il ne maintient que deux ou
trois zones vivantes. Le vrai gain est structurel et invisible ici : zone TINY 5 → 4 pages,
densité 23,2 → 25,5 allocations par page. Un programme allouant beaucoup de petits objets
mappe 20 % de mémoire en moins. **Le bench mesure mal l'empreinte** — c'est une limite à
connaître, pas un argument contre M1.

**Décision : M1 conservé.** `test_show` et `test_show_ex` passent, donc la fidélité de
l'affichage de la taille demandée est préservée — c'était la condition de Charles.

## D48 — M3 (`FIT_FACTOR`) : impasse dans les deux sens, on garde 2 (2026-09-04)

**Facteur plus serré (3/2 = 1,5)** : `large` 86,9 → 114,2 (**+31 %**), `calloc` 197,5 →
220,0 (+11 %), pour **80 Ko** de `VmPeak` économisés. `strace` : 1687 syscalls contre 1061,
soit **+59 %** — un critère de réutilisation plus strict rejette davantage de zones du
cache. `calloc` souffre doublement : chaque miss donne une zone fraîchement mappée dont le
`memset` doit fauter toutes les pages.

**Facteur plus large (3)** : comparaison entrelacée (3, 2, 3, 2) sur 7 runs pour neutraliser
la dérive machine — `large` médianes 93,9 / **85,8** / 120,8 / **81,8**, VmPeak 5948 contre
5680 Ko. Le facteur 2 gagne sur la vitesse **et** la mémoire, et il est bien plus stable
(80-94 contre 81-166). Contre-intuitif mais explicable : une zone de 12 Ko servie à une
demande de 4 Ko est consommée pour rien, ses 8 Ko restants gaspillés, et elle n'est plus
disponible pour la grosse demande suivante. Un cache trop permissif se dégrade en retenant
des zones mal appariées.

**Décision : `FIT_FACTOR` reste à 2.** Le réglage d'origine était déjà optimal.

**Deux acquis conservés malgré l'annulation :**
1. Le facteur s'exprime désormais en **fraction entière** (`FIT_NUM`/`FIT_DEN`). La
   tentative à 1,5 avait introduit un littéral **flottant** sur le chemin chaud :
   `needed * 1.5` déclenchait entier → double, multiplication FP, reconversion tronquée à
   chaque allocation LARGE, et `SIZE_MAX / 1.5` rendait le garde-fou d'overflow approximatif
   (`SIZE_MAX` ne tient pas exactement dans un `double`). Coût mesuré : `calloc` 292 → 220
   à comportement identique. La fraction rend ce piège impossible.
2. **Garde `[ -f libft_malloc.so ]` avant toute campagne de mesure.** Un build échoué rend
   `LD_PRELOAD` inopérant avec un simple avertissement, et on mesure la glibc sans le voir
   (constaté : `large` à 49 ns, `VmPeak` 4368 Ko). Même mode de défaillance que le test
   fantôme de [[D30]].

**Note de protocole** : `zsh` ne découpe pas les variables non quotées (`set -- $f` avec
`f="2 1"` donne `$1="2 1"`). Utiliser `${=f}` ou éviter les paires. Déjà rencontré en début
de session sur la boucle `LD_PRELOAD`.

## D49 — M2 et V5 annulées par la mesure (2026-09-04)

### V5 — index O(1) des zones ayant de la place : sans objet
Instrumentation de `heap_reserve_block` sur 200 000 opérations :
**1,34 zone parcourue par appel** (`small`), **1,22** (`mixed`). Un index O(1)
économiserait un tiers d'itération. La première zone de la liste convient presque toujours :
c'est la plus récemment créée, donc celle qui a de la place. Aucun gisement.

### M2 — réutiliser les résidus LARGE : gisement réel mais **virtuel**
Profil `large`, état à la fin du workload : 254 zones vivantes, 508 blocs (un résidu par
zone), **1 577 440 o utiles contre 958 208 o en résidus, soit 37,8 %** de l'empreinte LARGE.

Mais `VmPeak` 5 680 Ko contre `VmHWM` 4 480 Ko : **1 200 Ko mappés et jamais résidents**,
et les résidus sont très majoritairement là-dedans. `ftm_block_split` n'écrit que l'en-tête
du résidu (32 o, une ligne de cache) ; les kilo-octets suivants ne sont jamais touchés, donc
le noyau ne leur attribue aucune page physique. **Le gaspillage est de l'espace d'adressage
virtuel, pas de la mémoire réelle.**

Et le récupérer coûterait le cache : servir une 2e allocation depuis le résidu remet deux
allocations dans une zone LARGE, donc libérer l'une ne la rend plus « fully free », donc
elle n'entre plus dans le cache. C'est le mécanisme que le pas A a supprimé, mesuré en
[[D29]] (1212 contre 1031 ns/op à l'époque) et dont le cache vaut aujourd'hui **×37** sur ce
profil.

**Décision : M2 abandonnée.** Échanger un mécanisme qui vaut ×37 contre de l'adressage
virtuel gratuit sur 64 bits n'a pas de sens.

**Réserve** : ce raisonnement dépend du 64 bits. Sur i386 (KFS-3), un mégaoctet d'espace
d'adressage sur 4 Go n'est plus négligeable et l'arbitrage pourrait s'inverser. Noté dans
`perf-hors-sujet.md`.

## D50 — Banc de mesure `bench/` pour la soutenance (2026-09-04)

Trois pièces : des **drapeaux d'ablation** dans `ftm_config.h`, `bench/run.sh` (médianes),
`bench/Makefile` (orchestration).

**Drapeaux** : `FTM_ENABLE_LARGE_CACHE`, `FTM_ENABLE_ZONE_MAP`, `FTM_ENABLE_LARGE_FASTPATH`.
Implémentés en **macros de façade** (`FTM_CACHE_TAKE/PUT`, `FTM_MAP_INSERT/REMOVE/ACTIVE`,
`FTM_SCANS_ZONES`) et non en `#if` dans le corps des fonctions : le code métier reste
identique et lisible. Astuce : désactivé, `FTM_CACHE_PUT(zone)` renvoie la zone elle-même,
donc `if (evicted == NULL) return;` tombe droit sur la destruction — le comportement « sans
cache » émerge sans branchement conditionnel. Les cinq combinaisons compilent.

**Cibles** : `demo` (la séquence de soutenance), `ablation` (désactive une optimisation à la
fois), `sweep-m` / `sweep-cache` / `sweep-map` / `sweep-fit` (justifier les valeurs
retenues), `everything`. Chaque cible se termine par `restore` (rebuild depuis
`Makefile.cfg`), et les flags passent en **variable de ligne de commande**
(`CMOREFLAGS=...`), qui prime sur `Makefile.cfg` sans l'écraser. `run.sh` vérifie la
présence de la `.so` avant de mesurer — la garde issue de D48.

**Sortie de `demo`** (RUNS=3, OPS=100000, machine chargée) :

```
                                    small     mixed     large    calloc     VmPeak
glibc (par defaut)                  13.69     40.11     53.74    123.56    4368 KB
glibc (meme contrainte mmap)      3719.36   3369.28   3776.58   3806.11    4764 KB
ft_malloc (etat initial)            49.49     60.29   2981.86   3389.70    4864 KB
ft_malloc (final)                   58.71     73.82     91.02    211.24    5684 KB
```

La ligne « même contrainte mmap » est plus forte qu'anticipé : la glibc s'effondre sur
**tous** les profils, pas seulement `large` — 3719 ns sur `small` contre 59 pour nous.

**Limite à connaître** : `tests/Makefile` a ses propres `CFLAGS` et ne lit pas
`CMOREFLAGS`. L'archive de test est donc toujours construite dans la configuration de
référence ; un « 16/16 » sous ablation ne valide pas la variante. Sans conséquence pour le
banc (qui passe par `LD_PRELOAD`), mais à ne pas mal lire.

**Point à savoir défendre** : `small` et `mixed` sont *meilleurs* à l'état initial (49 et 60
contre 59 et 74) — c'est le coût de M1, assumé : 4 % de vitesse contre 20 % de mémoire sur
les zones TINY (D47).

### D50 (suite) — banc appliqué, et une piste qu'il produit immédiatement

Appliqué au dépôt : drapeaux d'ablation dans `include/ftm_config.h`, six substitutions par
macros de façade dans `src/core/ftm_heap.c`, `bench/run.sh` et `bench/Makefile` créés.
Les cinq combinaisons d'ablation compilent, 16/16 tests, `LD_PRELOAD` OK sur ls / git /
python3, 18 symboles publics inchangés.

**Sortie de `make -C bench ablation`** (RUNS=3, OPS=100000) :

| variante                 | small | mixed |   large |  calloc | VmPeak  |
|--------------------------|------:|------:|--------:|--------:|---------|
| référence                |  59,1 |  74,9 | **101,9** |   222,8 | 5684 Ko |
| sans cache LARGE         |  73,4 |  79,4 |  5017,5 |  4885,8 | 5080 Ko |
| sans page map            |  **53,3** |  **62,2** |   953,6 |  1172,9 | 5684 Ko |
| sans fast-path LARGE     |  59,3 |  72,5 |   631,0 |   794,5 | 5284 Ko |
| sans rien (état initial) |  50,1 |  70,2 |  2986,7 |  3552,8 | 4864 Ko |

**Deux enseignements immédiats.**

1. **« Sans rien » (2987) bat « sans cache » seul (5017)** — l'antagonisme de [[D28]]/[[D29]]
   rendu visible en une ligne : retirer le cache en gardant le fast-path donne le pire des
   deux mondes, ni cache explicite ni cache accidentel. Excellente démonstration pour la
   soutenance.

2. **PISTE NOUVELLE — la page map coûte sur `small` et `mixed`.** Désactivée, ils gagnent
   **10 %** (59,1 → 53,3) et **17 %** (74,9 → 62,2) ; elle ne paie que sur `large`. Logique :
   pour deux zones TINY, un parcours linéaire bat un hachage, et l'insertion/retrait de
   chaque page à la création de zone coûte. **Idée à mesurer** : n'indexer que les zones
   LARGE, et laisser `ftm_heap_find_zone` retomber sur le parcours linéaire pour TINY/SMALL
   (leurs listes font 1 à 2 éléments, cf. D49). Gain potentiel : 10-17 % sur small/mixed
   sans rien perdre sur large. Le banc a produit cette piste dès sa première utilisation.

**Nettoyage au passage** : `Makefile.cfg` traînait `-DFTM_LARGE_CACHE_FIT_NUM=4
-DFTM_LARGE_CACHE_FIT_DEN=1`, reliquat d'expérience contredisant [[D48]]. Le banc n'en était
pas affecté (il passe ses propres `CMOREFLAGS` en ligne de commande, qui priment sur
`Makefile.cfg`), mais le build quotidien tournait avec un fit factor de 4. Reconfiguré à
`--cflags="-DFTM_SMALL_MAX=2048"`.

**Note** : `FTM_ZONE_MAP_CAPACITY` vaut **2048** dans le code, alors que D38 avait retenu
4096. Marge ×2,2 au lieu de ×4,4 sur la charge observée — viable, mais moins de coussin
avant la désactivation. À trancher.
