# **************************************************************************** #
#                          Banc de mesure ft_malloc                            #
# **************************************************************************** #

ROOT		:= ..
RUN		:= ./run.sh
BASE		:= -DFTM_SMALL_MAX=2048
RULE		:= ---------------------------------------------------------------------------------------

export OPS
export RUNS
export PROFILES

all: header reference glibc restore

header:
	@printf "\n%-34s%10s%10s%10s%10s%13s\n" "" "small" "mixed" "large" "calloc" "VmPeak"
	@printf "%s\n" "$(RULE)"

reference: | binary
	@$(RUN) "ft_malloc (reference)" "$(BASE)"

glibc: | binary
	@PRELOAD=0 $(RUN) "glibc" ""
	@PRELOAD=0 ENVVARS="MALLOC_MMAP_THRESHOLD_=1" $(RUN) "glibc (regime mmap force)" ""

# ---- ablation : desactiver une optimisation a la fois ---------------------- #
ablation: header reference | binary
	@$(RUN) "sans cache de zones LARGE" "$(BASE) -DFTM_ENABLE_LARGE_CACHE=0"
	@$(RUN) "sans page map (find O(n))" "$(BASE) -DFTM_ENABLE_ZONE_MAP=0"
	@$(RUN) "sans fast-path LARGE" "$(BASE) -DFTM_ENABLE_LARGE_FASTPATH=0"
	@$(RUN) "sans rien (etat initial)" "$(BASE) -DFTM_ENABLE_LARGE_CACHE=0 -DFTM_ENABLE_ZONE_MAP=0 -DFTM_ENABLE_LARGE_FASTPATH=0"
	@$(MAKE) --no-print-directory restore

# ---- balayages de parametres ---------------------------------------------- #
sweep-m: header | binary
	@for m in 1024 2048 4096; do \
		$(RUN) "SMALL_MAX = $$m" "-DFTM_SMALL_MAX=$$m"; \
	done
	@$(MAKE) --no-print-directory restore

sweep-cache: header | binary
	@for c in 4 16 32 64 128; do \
		$(RUN) "cache LARGE = $$c zones" "$(BASE) -DFTM_LARGE_CACHE_MAX_ZONES=$$c"; \
	done
	@$(MAKE) --no-print-directory restore

sweep-map: header | binary
	@for k in 1024 2048 4096 8192; do \
		$(RUN) "page map = $$k entrees" "$(BASE) -DFTM_ZONE_MAP_CAPACITY=$$k"; \
	done
	@$(MAKE) --no-print-directory restore

sweep-fit: header | binary
	@$(RUN) "fit factor 3/2" "$(BASE) -DFTM_LARGE_CACHE_FIT_NUM=3 -DFTM_LARGE_CACHE_FIT_DEN=2"
	@$(RUN) "fit factor 2/1" "$(BASE) -DFTM_LARGE_CACHE_FIT_NUM=2 -DFTM_LARGE_CACHE_FIT_DEN=1"
	@$(RUN) "fit factor 3/1" "$(BASE) -DFTM_LARGE_CACHE_FIT_NUM=3 -DFTM_LARGE_CACHE_FIT_DEN=1"
	@$(MAKE) --no-print-directory restore

# ---- la demo de soutenance ------------------------------------------------- #
demo: header | binary
	@PRELOAD=0 $(RUN) "glibc (par defaut)" ""
	@PRELOAD=0 ENVVARS="MALLOC_MMAP_THRESHOLD_=1" $(RUN) "glibc (meme contrainte mmap)" ""
	@$(RUN) "ft_malloc (etat initial)" "$(BASE) -DFTM_ENABLE_LARGE_CACHE=0 -DFTM_ENABLE_ZONE_MAP=0 -DFTM_ENABLE_LARGE_FASTPATH=0"
	@$(RUN) "ft_malloc (final)" "$(BASE)"
	@$(MAKE) --no-print-directory restore

everything: all ablation sweep-m sweep-cache sweep-map sweep-fit

binary:
	@$(MAKE) --no-print-directory -C $(ROOT) bench_alloc

restore:
	@$(MAKE) --no-print-directory -C $(ROOT) re >/dev/null 2>&1

.PHONY: all header reference glibc ablation sweep-m sweep-cache sweep-map \
	sweep-fit demo everything binary restore
