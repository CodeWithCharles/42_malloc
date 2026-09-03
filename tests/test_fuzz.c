/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test_fuzz.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/03 12:48:03 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/03 12:50:00 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ftm_internal.h"
#include "ftm_port.h"
#include "fake_port.h"

#include <stdio.h>

#define CHECK(cond) do { \
	if (!(cond)) { \
		printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); \
		return (1); \
	} \
} while (0)

#define SLOTS	256
#define	EPOCH	2000
#define EPOCHS	100

typedef struct s_slot
{
	unsigned char	*ptr;
	size_t			size;
	unsigned char	seed;
} t_slot;

static t_slot			g_slots[SLOTS];
static unsigned long	g_rng = 0x9E3779B97F4A7C15UL;

static unsigned long	rng(void)
{
	g_rng ^= g_rng << 13;
	g_rng ^= g_rng >> 7;
	g_rng ^= g_rng << 17;
	return (g_rng);
}

static void	fill(unsigned char *ptr, size_t n, unsigned char seed)
{
	size_t	i;

	i = 0;
	while (i < n)
	{
		ptr[i] = (unsigned char)(seed * 31 + i);
		i++;
	}
}

static int	verify(unsigned char *ptr, size_t n, unsigned char seed)
{
	size_t	i;

	i = 0;
	while (i < n)
	{
		if (ptr[i] != (unsigned char)(seed * 31 + i))
			return (0);
		i++;
	}
	return (1);
}

static size_t	random_size(void)
{
	unsigned long	bucket;

	bucket = rng() % 100;
	if (bucket < 5)
		return (0);
	if (bucket < 70)
		return (1 + rng() % 128);
	if (bucket < 92)
		return (129 + rng() % 896);
	return (1025 + rng() % 3072);
}

static int	do_allocate(int idx)
{
	size_t			size;
	unsigned char	seed;
	unsigned char	*ptr;

	size = random_size();
	seed = (unsigned char)rng();
	ptr = ftm_alloc(size);
	CHECK(ptr != NULL);
	fill(ptr, size, seed);
	g_slots[idx].ptr = ptr;
	g_slots[idx].size = size;
	g_slots[idx].seed = seed;
	return (0);
}

static int	do_reallocate(int idx)
{
	size_t			new_size;
	size_t			keep;
	unsigned char	*ptr;
	unsigned char	seed;

	new_size = random_size();
	ptr = ftm_resize(g_slots[idx].ptr, new_size);
	CHECK(ptr != NULL);
	keep = g_slots[idx].size;
	if (new_size < keep)
		keep = new_size;
	CHECK(verify(ptr, keep, g_slots[idx].seed));
	seed = (unsigned char)rng();
	fill(ptr, new_size, seed);
	g_slots[idx].ptr = ptr;
	g_slots[idx].size = new_size;
	g_slots[idx].seed = seed;
	return (0);
}

static int	run_epoch(void)
{
	int	step;
	int	idx;

	step = 0;
	while (step < EPOCH)
	{
		idx = (int)(rng() % SLOTS);
		if (g_slots[idx].ptr == NULL)
		{
			if (do_allocate(idx))
				return (1);
		}
		else
		{
			CHECK(verify(g_slots[idx].ptr, g_slots[idx].size,
					g_slots[idx].seed));
			if (rng() % 2 == 0)
			{
				ftm_release(g_slots[idx].ptr);
				g_slots[idx].ptr = NULL;
			}
			else if (do_reallocate(idx))
				return (1);
		}
		CHECK(ftm_check_heap());
		step++;
	}
	return (0);
}

static void	drain(void)
{
	int	idx;

	idx = 0;
	while (idx < SLOTS)
	{
		if (g_slots[idx].ptr != NULL)
		{
			ftm_release(g_slots[idx].ptr);
			g_slots[idx].ptr = NULL;
		}
		idx++;
	}
}

int	main(void)
{
	int	epoch;

	epoch = 0;
	while (epoch < EPOCHS)
	{
		ftm_heap_reset();
		fake_port_reset();
		if (run_epoch())
			return (1);
		drain();
		if (!ftm_check_heap())
			return (1);
		epoch++;
	}
	printf("fuzz: OK (%d ops)\n", EPOCH * EPOCHS);
	return (0);
}