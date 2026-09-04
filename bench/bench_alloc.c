/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   bench_alloc.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/03 16:45:54 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/04 16:49:35 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#define _POSIX_C_SOURCE	199309L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>

#define SLOT_COUNT	512

typedef struct s_slot
{
	void	*ptr;
	size_t	size;
} t_slot;

static unsigned long	g_rng = 0x9E3779B97F4A7C15UL;

static unsigned long	rng(void)
{
	g_rng ^= g_rng << 13;
	g_rng ^= g_rng >> 7;
	g_rng ^= g_rng << 17;
	return (g_rng);
}

static void	report_memory(void)
{
	char	buf[4096];
	char	*found;
	long	vmpeak;
	long	vmhwm;
	int	fd;
	ssize_t	n;

	vmpeak = 0;
	vmhwm = 0;
	fd = open("/proc/self/status", O_RDONLY);
	if (fd < 0)
		return ;
	n = read(fd, buf, sizeof(buf) - 1);
	close(fd);
	if (n <= 0)
		return ;
	buf[n] = '\0';
	found = strstr(buf, "VmPeak:");
	if (found != NULL)
		vmpeak = strtol(found + 7, NULL, 10);
	found = strstr(buf, "VmHWM:");
	if (found != NULL)
		vmhwm = strtol(found + 6, NULL, 10);
	printf("    VmPeak %8ld KB    VmHWM %8ld KB\n", vmpeak, vmhwm);
}

static size_t	pick_size_small(void)
{
	return (1 + rng() % 128);
}

static size_t	pick_size_mixed(void)
{
	unsigned long	bucket;

	bucket = rng() % 100;
	if (bucket < 70)
		return (1 + rng() % 128);
	if (bucket < 95)
		return (129 + rng() % 896);
	return (1025 + rng() % 1024);
}

static size_t	pick_size_large(void)
{
	return (2048 + rng() % 8192);
}

static double	elapsed_ns(struct timespec start, struct timespec stop)
{
	double	seconds;
	double	nanos;

	seconds = (double)(stop.tv_sec - start.tv_sec);
	nanos = (double)(stop.tv_nsec - start.tv_nsec);
	return (seconds * 1e9 + nanos);
}

static void	run_profile(size_t (*pick)(void), size_t ops, int with_realloc)
{
	t_slot			slots[SLOT_COUNT];
	struct timespec	start;
	struct timespec	stop;
	size_t			step;
	size_t			idx;

	memset(slots, 0, sizeof(slots));
	clock_gettime(CLOCK_MONOTONIC, &start);
	step = 0;
	while (step < ops)
	{
		idx = rng() % SLOT_COUNT;
		if (slots[idx].ptr == NULL)
		{
			slots[idx].size = pick();
			slots[idx].ptr = malloc(slots[idx].size);
		} else if (with_realloc && rng() % 3 == 0)
		{
			slots[idx].size = pick();
			slots[idx].ptr = realloc(slots[idx].ptr, slots[idx].size);
		}
		else
		{
			free(slots[idx].ptr);
			slots[idx].ptr = NULL;
		}
		step++;
	}
	clock_gettime(CLOCK_MONOTONIC, &stop);
	idx = 0;
	while (idx < SLOT_COUNT)
	{
		if (slots[idx].ptr != NULL)
			free(slots[idx].ptr);
		idx++;
	}
	printf("\n    %10zu ops    %10.2f ns/op    %10.2f ms total\n",
		ops, elapsed_ns(start, stop) / (double)ops,
		elapsed_ns(start, stop) / 1e6);
	report_memory();
}

static void	run_sawtooth(size_t ops)
{
	void			*held[300];
	struct timespec	start;
	struct timespec	stop;
	size_t			step;
	size_t			index;

	clock_gettime(CLOCK_MONOTONIC, &start);
	step = 0;
	while (step < ops)
	{
		index = 0;
		while (index < 300)
			held[index++] = malloc(128);
		index = 0;
		while (index < 300)
			free(held[index++]);
		step += 600;
	}
	clock_gettime(CLOCK_MONOTONIC, &stop);
	printf("\n    %10zu ops    %10.2f ns/op    %10.2f ms total\n",
		ops, elapsed_ns(start, stop) / (double)ops,
		elapsed_ns(start, stop) / 1e6);
	report_memory();
}

int	main(int argc, char **argv)
{
	size_t	ops;

	if (argc < 3)
	{
		fprintf(stderr, "usage: %s <small|mixed|large> <ops>\n", argv[0]);
		return (1);
	}
	ops = (size_t)strtoul(argv[2], NULL, 10);
	printf("ops,total_ns,ns_per_op\n");
	if (strcmp(argv[1], "small") == 0)
		run_profile(pick_size_small, ops, 0);
	else if (strcmp(argv[1], "mixed") == 0)
		run_profile(pick_size_mixed, ops, 1);
	else if (strcmp(argv[1], "large") == 0)
		run_profile(pick_size_large, ops, 0);
	else if (strcmp(argv[1], "sawtooth") == 0)
		run_sawtooth(ops);
	else
	{
		fprintf(stderr, "unknown profile: %s\n", argv[1]);
		return (1);
	}
	return (0);
}