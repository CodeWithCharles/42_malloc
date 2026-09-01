# 42_malloc

Allocateur mémoire dynamique en C. Livrable : `libft_malloc_$HOSTTYPE.so`,
interposable par LD_PRELOAD. Bonus prévus dès l'architecture.

## Contexte de session
Lire `.ai/roadmap.md`, `.ai/state.md`, `.ai/decisions.md`, `.ai/next-steps.md` avant
toute proposition. Mode de travail : `collab-pairing-mentor` — Claude guide et review,
ne modifie aucun fichier hors de `.ai/`.

## Règles d'architecture non négociables
1. Aucun appel système hors de `port/`.
2. Aucune sortie hors de `core/ftm_show.c` (qui passe par `ftm_write`).
   Jamais de printf/snprintf : printf alloue → récursion infinie.
3. Aucun `#include <...>` système dans `core/` — uniquement `"ftm_stdint.h"`.
4. Aucune allocation dynamique pour les métadonnées : tout vit dans les zones.
5. Toutes les constantes de taille dans `core/ftm_config.h`, N et M **calculés**.
6. Les blocs d'une zone : liste doublement chaînée ordonnée par adresse,
   blocs alloués compris (indispensable au coalescing O(1)).

## Commandes
make          # libft_malloc_$(HOSTTYPE).so + symlink
make test     # tests unitaires sur libftm_core.a
make bench
make re / fclean