# Prochains petits pas

## 🟨 Phase 0 — le socle (en cours)
Contenu fourni en markdown dans la conversation : arborescence, `.gitignore`,
`includes/ft_malloc.h`, `src/malloc.c` (stubs), `libft/` minimale, `tests/test_smoke.c`,
`tests/fake_port.c` (placeholder), `bench/bench_alloc.c` (placeholder), `Makefile` racine.

**Critères de fin :**
1. `make` produit `libft_malloc_$(HOSTTYPE).so` + le symlink `libft_malloc.so`
2. `nm -D libft_malloc.so | grep -E ' T (malloc|free|realloc)'` sort 3 lignes
3. `make test` affiche `PASS build/tests/test_smoke`
4. `make re` et `make fclean` fonctionnent
5. Toucher `includes/ft_malloc.h` puis `make` → seul `src/malloc.o` se recompile
   (validation des dépendances `-MMD -MP`)

**Points de vigilance :**
- `libft` doit être compilée en `-fPIC`, sinon le link de la `.so` échoue avec
  « relocation R_X86_64_32S ... recompile with -fPIC »
- `-fno-tree-loop-distribute-patterns` dans la libft : sans ça, GCC -O2 transforme la
  boucle de `ft_memcpy` en appel à `memcpy`
- Vérifier le nom produit : si `HOSTTYPE` est exporté par le shell, on obtient
  `x86_64` au lieu de `x86_64_Linux`

## Phase 1 — le contrat
`core/ftm_port.h` (9 fonctions), `core/ftm_config.h` (constantes dérivées),
`core/ftm_types.h` (`t_block`, `t_zone`, `t_arena`), `port/ftm_stdint.h`.

**Critère de fin** : `gcc -c core/*.c` passe sans aucune implémentation du port.
C'est le test d'étanchéité de la couche.

## Phase 2 — port POSIX + fake port
`mmap`/`munmap`/`sysconf`/`getrlimit`/`write(2)`, `ftm_lock` encore vide, et
`tests/fake_port.c` sur un pool statique + `fake_fail_after(n)`.

---

*Ne pas attaquer une phase avant que la précédente soit verte.*
