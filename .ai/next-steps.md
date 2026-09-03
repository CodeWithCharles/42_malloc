# Prochains petits pas

## ✅ PARTIE OBLIGATOIRE COMPLETE (phases 0-9)
malloc/free/realloc/calloc + famille alignee POSIX + show_alloc_mem, interposable
LD_PRELOAD (bash, vim, ls OK). 12 tests unitaires verts. errno, validation pointeur,
coalescing, zones recyclees, pas de leak (compteurs equilibres).

## 🔒 Point de controle "OBLIGATOIRE PARFAIT" (a faire avant les bonus)
Le sujet : les bonus ne sont evalues QUE si l'obligatoire est parfait.
1. Relecture code (clean, noms qui crient, zero commentaire superflu, D14).
2. LD_PRELOAD sur ls / bash / vim / git / python3 sans crash.
3. Fuzz court + ftm_check_heap ( debut de phase 13, en avance).
4. Verifier Makefile : make / re / fclean / recompilation incrementale / symlink / HOSTTYPE.
5. Verifier norme "clean code" meme sans norminette.

## Ensuite — BONUS (dans l'ordre)
- Phase 10 : thread-safe (ftm_lock/unlock reels, pthread_mutex). B1.
- Phase 11 : variables d'env de debug (FT_MALLOC_*). B2.
- Phase 12 : show_alloc_mem_ex (historique + hexdump). B3.
- Phase 13 : invariants + fuzz massif + bench + tuning n/m.
- Phase 14 : integration reelle finale.
