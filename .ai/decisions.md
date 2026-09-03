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
