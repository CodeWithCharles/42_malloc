# Prochains petits pas

## 🟨 Phase 5 — ftm_alloc + malloc (en cours)
Le coeur. `src/core/ftm_heap.c` : g_heap (variable globale unique), init paresseuse,
routage par classe, first-fit sur zones existantes, SPLIT d'un bloc trop grand, creation
de zone a la demande, chainage des zones dans g_heap.zones[kind]. Hooks ftm_on_alloc (vides).
lock/unlock (no-op) autour. Puis le shim src/malloc.c → ftm_alloc.

DECISION A PRENDRE : politique malloc(0). Reco = pointeur unique liberable (glibc), pas NULL.

**Criteres** : 200 malloc(64) → 200 adresses distinctes, alignees 16, non chevauchantes,
dans exactement 2 zones TINY. Pas de __attribute__((constructor)). Jamais de printf.

## Phase 6 — free + coalescing (B4)
## Phase 7 — realloc
