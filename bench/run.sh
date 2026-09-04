#!/bin/bash
# usage : run.sh "<label>" "<cflags>"
#   PRELOAD=0            mesure la glibc (ni rebuild ni LD_PRELOAD)
#   ENVVARS="A=1 B=2"    variables d'environnement passees au binaire
#   OPS / RUNS / PROFILES  surchargeables

label="$1"
cflags="$2"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OPS="${OPS:-200000}"
RUNS="${RUNS:-7}"
PROFILES="${PROFILES:-small mixed large calloc}"
PRELOAD="${PRELOAD:-1}"

if [ "$PRELOAD" = "1" ]; then
	make -C "$ROOT" re CMOREFLAGS="$cflags" >/dev/null 2>&1
	if [ ! -f "$ROOT/libft_malloc.so" ]; then
		printf "%-34s BUILD FAILED\n" "$label"
		exit 1
	fi
	preload="LD_PRELOAD=$ROOT/libft_malloc.so"
else
	preload=""
fi

median() { sort -n | awk '{v[NR]=$1} END {printf "%.2f", v[int((NR+1)/2)]}'; }

printf "%-34s" "$label"
for profile in $PROFILES; do
	value=$(for _ in $(seq "$RUNS"); do
		env $preload $ENVVARS "$ROOT/bench_alloc" "$profile" "$OPS" \
			| awk '/ns\/op/ {print $3}'
	done | median)
	printf "%10s" "$value"
done
peak=$(env $preload $ENVVARS "$ROOT/bench_alloc" large "$OPS" \
	| awk '/VmPeak/ {print $2}')
printf "%10s KB\n" "$peak"
