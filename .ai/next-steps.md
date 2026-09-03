# Prochains petits pas

## 🟨 Phase 9 — Robustesse + POSIX max (en cours, DERNIERE de l'obligatoire)
1. Validation de pointeur (ftm_guard.c) : free/realloc d'un pointeur etranger, double-free,
   pointeur au milieu d'un bloc → detectes, jamais de crash. Marqueur "magic" dans le header.
2. errno = ENOMEM sur echec alloc (dans le shim src/malloc.c, PAS dans core).
3. Famille alignee : posix_memalign, aligned_alloc, memalign, valloc, pvalloc,
   malloc_usable_size, reallocarray. Over-allocation + offset stocke avant le pointeur.
4. free() valide que le pointeur est a nous (sinon no-op) → survit a posix_memalign externe.

Cibles POSIX : cf. decisions D19/D21. Point delicat = alignement > 16 (over-alloc + offset).

## Point de controle : OBLIGATOIRE PARFAIT
Apres phase 9 : relire tout, LD_PRELOAD sur ls/vim, fuzz court. Puis bonus (10-12) et 13.
