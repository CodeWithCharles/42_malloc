# Prochains petits pas

## 🟨 Phase 8 — show_alloc_mem (en cours)
`src/core/ftm_fmt.c` : formatage maison (hex, decimal) — PAS de printf (printf alloue).
`src/core/ftm_show.c` : parcours zones triees par adresse croissante, blocs ALLOUES only,
format exact du sujet, Total = somme des payloads alloues. Une seule ftm_write par ligne.
Test compare la sortie au caractere pres (fake_port → adresses deterministes).

Invariant acquis en phase 7 : classe d'une allocation vivante == classe de sa zone
(garantit l'affichage sous le bon en-tete TINY/SMALL/LARGE).

## Phase 9 — robustesse + famille de symboles (aligned_alloc, posix_memalign, ...)
## Point de controle : obligatoire PARFAIT avant les bonus
