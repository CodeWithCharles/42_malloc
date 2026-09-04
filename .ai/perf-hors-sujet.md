# Pistes de performance qui CASSENT le sujet

> **Statut : non implémentées, et non implémentables dans le cadre du rendu 42.**
> Ce document existe pour deux raisons : savoir répondre en soutenance à « pourquoi
> êtes-vous encore N fois plus lent que la glibc ? », et garder la porte ouverte
> pour KFS-3, où ces contraintes n'existent plus.
>
> Contrainte violée dans tous les cas : *« You are allowed a global variable to manage
> your allocations and one for the thread-safe »* ou *« You must use the mmap(2) and
> munmap(2) syscall to claim and return the memory zones »*.

## Point de repère : où on en est

État au 2026-09-04, après la session d'optimisation LARGE (cf. D28→D31) :

| profil  | glibc | ft_malloc | écart |
|---------|------:|----------:|------:|
| `small` |  11.6 |    ~166   | ×14   |
| `mixed` |  29.3 |    ~191   | ×6,5  |
| `large` |  55.6 |    ~230   | ×4,2  |

Mesures annexes utiles, prises sur la machine de Charles :
- `pthread_mutex_lock` + `unlock` non contendu : **4,1 ns** (2 % du budget → le verrou
  n'est PAS le goulot, contrairement à l'intuition) ;
- une zone TINY porte ~140 blocs dont **8 % libres** → le `first-fit` linéaire explique
  quasiment tout le plateau de `small`/`mixed` (traité dans `perf-roadmap.md`, piste
  conforme au sujet) ;
- glibc forcée dans notre régime (`MALLOC_MMAP_THRESHOLD_=1`) sur `large` : **3737 ns/op**,
  soit **15,6× plus lente que nous**.

---

## 1. Cache par thread (tcache) — le seul vrai levier restant

**Ce que fait la glibc.** Depuis la 2.26, chaque thread garde une pile LIFO de blocs
récemment libérés, par classe de taille (64 classes, jusqu'à 7 blocs chacune). Un `malloc`
sous 1 Ko devient un `pop` : pas de verrou, pas de recherche, pas de coalescing. C'est
l'intégralité de l'explication des 11,6 ns/op.

**Pourquoi c'est interdit.** Il faut du stockage thread-local (`__thread` / TLS), donc
autant de structures que de threads — bien au-delà de « une variable globale + une pour le
thread-safe ».

**Gain attendu.** `small` de ~166 à 30-40 ns/op. C'est l'écart résiduel, presque en totalité.

**Corollaire.** Supprimer le mutex n'a d'intérêt que dans ce cadre : à 4,1 ns sur un budget
de 166 il ne pèse rien ; sur un budget de 35 il pèserait 12 %.

---

## 2. Arènes multiples

**Ce que fait la glibc.** Jusqu'à `8 × nb_cpu` arènes indépendantes, chacune avec son
verrou. Un thread bloqué sur une arène en essaie une autre.

**Pourquoi c'est interdit.** Même raison : plusieurs contextes d'allocation globaux.

**Gain attendu.** Nul en mono-thread. Décisif au-delà de ~8 threads en contention réelle
— notre `test_thread` (8 threads) ne mesure que la cohérence, pas le débit.

---

## 3. Tas contigu par `brk`/`sbrk`

**Ce que fait la glibc.** Son arène principale vient de `brk`, pas de `mmap`. Son seuil
`mmap` démarre à 128 Ko et monte dynamiquement jusqu'à 32 Mo. Une allocation de 2-10 Ko
n'entraîne donc **aucun syscall** : elle est découpée dans le tas déjà obtenu.

**Pourquoi c'est interdit.** Le sujet impose explicitement `mmap`/`munmap`. (Le sujet se
moque d'ailleurs de `brk` dans son chapitre « Let's laugh a little ».)

**Gain attendu.** C'est exactement la source de l'écart sur `large`. Notre cache de zones
(D31) contourne le problème sans le supprimer : on ne refait plus de syscall en régime
établi, mais la première allocation de chaque taille en paie un.

**Argument de soutenance associé.** `MALLOC_MMAP_THRESHOLD_=1` place la glibc sous notre
contrainte : elle tombe à 3737 ns/op contre nos ~230. L'écart ×4,2 en sa faveur en
configuration par défaut mesure sa **liberté**, pas sa supériorité.

---

## 4. `free` sans validation de pointeur

**Ce que fait la glibc.** Elle déréférence l'en-tête à `ptr - 16` et fait confiance.
D'où le fait qu'un `free(pointeur_invalide)` y provoque un crash ou une corruption.

**Pourquoi c'est interdit.** Le sujet exige *« In no way can your function lead to
undefined behaviour or segv »*. Notre `ftm_heap_find_zone` + `ftm_pointer_is_allocated`
garantissent qu'on ne déréférence jamais un en-tête avant d'avoir prouvé que le pointeur
tombe dans une zone connue.

**Gain attendu — attention, périmé.** Une expérience menée sur la branche exploratoire
`large_opti` (avant la page map) donnait 1010 → 143 ns/op sur `large`, soit 86 % du temps
consacré à la validation. **Ce chiffre n'est plus valable** : depuis D31, `find_zone` est
en O(1) via la page map, donc l'essentiel de ce coût est déjà récupéré *sans* sacrifier la
garantie. Le gain résiduel d'un `free` non validé serait aujourd'hui marginal.

**À retenir pour la soutenance.** C'est le meilleur exemple du projet : une garantie de
sûreté qui semblait coûter 86 % s'est révélée être un problème d'algorithme, pas un
compromis sûreté/vitesse. On a gardé la garantie ET la vitesse.

---

## 5. En-tête de bloc à 16 octets (boundary tags packés)

**Ce que fait la glibc.** Son en-tête de chunk fait 16 octets : `prev_size` + `size`, les
drapeaux logés dans les bits bas de `size` (toujours aligné, donc les 4 bits bas sont
libres). Pas de magic, pas de validation.

**Pourquoi c'est interdit.** Il n'y a plus de place pour `FTM_BLOCK_MAGIC` (24 bits
significatifs chez nous), or `ftm_block_is_valid` est ce qui rend sûr le `free` d'un
pointeur au milieu d'un bloc. Sans lui, on interprète des octets arbitraires comme un
en-tête et on libère un bloc fantôme — précisément le comportement glibc, et précisément
ce que le sujet interdit. Un magic sur 4 bits laisserait passer un pointeur invalide sur
seize.

**Gain réel — modeste.** En-tête 32 → 16 : la zone TINY reste à 4 pages (l'arrondi page
absorbe la différence), seule la densité progresse, 102 → 113 allocations par zone, soit
+11 %. Sur une alloc de 128 octets, le surcoût passe de 25 % à 12,5 %.

**Verdict.** Sacrifier la garantie « no segv » pour 11 % de densité n'est pas un échange
raisonnable — d'autant que le sujet en fait une exigence explicite. Cf. D39.

---

## 6. Pour KFS-3

Aucune de ces contraintes ne s'applique dans un kernel :
- un `kfree` sur pointeur invalide qui déclenche un `kernel_panic` est le comportement
  **attendu**, pas un défaut ;
- les caches per-CPU sont la norme des allocateurs slab (Linux : `kmem_cache_cpu`) ;
- il n'y a ni `brk` ni `mmap`, juste l'allocateur de frames physiques — donc la question
  du seuil ne se pose pas dans les mêmes termes.

La page map écrite en D31 est littéralement une table inversée page → zone, avec sondage
linéaire et pierres tombales : c'est le même exercice que la structure de pagination de
KFS-3, en plus simple.
