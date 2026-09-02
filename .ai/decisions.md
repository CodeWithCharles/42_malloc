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
