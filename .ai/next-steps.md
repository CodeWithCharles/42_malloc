# ROADMAP FINIE

> **Session en cours sur cette branche** : re-coder l'optimisation LARGE en mode
> `collab-pairing-mentor`. Tout le briefing est dans `.ai/mentor-large-perf.md` —
> c'est LE document à suivre, les étapes ci-dessous (relecture, soutenance) restent
> valables pour le rendu final sur master.

## Etat
- 15 phases sur 15 ✅
- 14 tests unitaires verts (dont fuzz 200k ops)
- 3 bonus valides (thread-safe, env vars debug, show_alloc_mem_ex)
- Bench + tuning : m=2048 retenu (D27)
- LD_PRELOAD OK sur bash, vim, ls, git, python3

## Reste a faire (par toi)
1. **Relecture code D14** : noms qui crient, zero commentaire superflu.
2. **Norminette adaptee** : le sujet dit "clean code even without norm — if it's ugly you get 0".
3. **Commit + push final** : le repo doit etre propre, .gitmodules OK, thirdparty/libft en submodule.
4. **Preparation soutenance** :
   - le tableau D27 (tuning chiffre) — c'est LA valeur ajoutee
   - defendre malloc(0) / realloc(p,0) : POSIX les laisse implementation-defined
   - defendre l'archi core/port : testabilite (fake_port deterministe), pas KFS-3
   - le pthread_atfork est un bonus du bonus a mentionner
   - le canari FT_MALLOC_GUARD detecte de VRAIS overflows (demo p[8]=1 → SIGABRT)
   - piste future : cache LRU des zones LARGE

## Post-soutenance eventuel
- KFS-3 : re-ouvrir la porte via le port kernel (cf. annexe roadmap §8)
- Cache LARGE si le retour de soutenance le suggere
