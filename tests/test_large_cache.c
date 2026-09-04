/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test_large_cache.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/04 12:38:05 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/04 16:52:52 by cpoulain         ###   ########.fr       */
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

#define EXTRA_ZONES 8
#define LEAK_ROUNDS 64

static void	setup(void)
{
	ftm_heap_reset();
	fake_port_reset();
}

static int	test_zone_is_reused(void)
{
	void	*first;
	void	*second;
	size_t	maps_before_reuse;

	setup();
	first = ftm_alloc(5000);
	CHECK(first != NULL);
	maps_before_reuse = fake_port_map_count();
	ftm_release(first);
	CHECK(ftm_heap_instance()->large_cache_count == 1);
	second = ftm_alloc(5000);
	CHECK(second == first);
	CHECK(fake_port_map_count() == maps_before_reuse);
	CHECK(ftm_heap_instance()->large_cache_hits == 1);
	CHECK(ftm_heap_instance()->large_cache_count == 0);
	ftm_release(second);
	CHECK(ftm_check_heap());
	return (0);
}

static int	test_eviction_is_bounded(void)
{
	void	*ptrs[FTM_LARGE_CACHE_MAX_ZONES + EXTRA_ZONES];
	size_t	index;

	setup();
	index = 0;
	while (index < FTM_LARGE_CACHE_MAX_ZONES + EXTRA_ZONES)
	{
		ptrs[index] = ftm_alloc(5000);
		CHECK(ptrs[index] != NULL);
		index++;
	}
	index = 0;
	while (index < FTM_LARGE_CACHE_MAX_ZONES + EXTRA_ZONES)
	{
		ftm_release(ptrs[index]);
		index++;
	}
	CHECK(ftm_heap_instance()->large_cache_count
		== FTM_LARGE_CACHE_MAX_ZONES);
	CHECK(ftm_check_heap());
	return (0);
}

static int	test_map_tracks_live_zones(void)
{
	void	*small;
	void	*large;

	setup();
	small = ftm_alloc(64);
	large = ftm_alloc(5000);
	CHECK(ftm_zone_map_is_active());
	CHECK(ftm_zone_map_lookup(small) != NULL);
	CHECK(ftm_zone_map_lookup(large) != NULL);
	CHECK(ftm_zone_map_lookup(large)->kind == FTM_LARGE);
	CHECK(ftm_zone_map_lookup(small) == ftm_heap_find_zone(small));
	ftm_release(large);
	CHECK(ftm_zone_map_lookup(large) != NULL);
	CHECK(ftm_heap_instance()->zone_count[FTM_LARGE] == 0);
	CHECK(ftm_heap_instance()->large_cache_count == 1);
	ftm_release(large);
	CHECK(ftm_check_heap());
	ftm_release(small);
	CHECK(ftm_check_heap());
	return (0);
}

static int	test_cache_does_not_leak(void)
{
	void	*ptrs[LEAK_ROUNDS];
	size_t	index;

	setup();
	index = 0;
	while (index < LEAK_ROUNDS)
	{
		ptrs[index] = ftm_alloc(3000 + index * 64);
		CHECK(ptrs[index] != NULL);
		index++;
	}
	index = 0;
	while (index < LEAK_ROUNDS)
	{
		ftm_release(ptrs[index]);
		index++;
	}
	ftm_heap_reset();
	CHECK(fake_port_map_count() == fake_port_unmap_count());
	return (0);
}

int	main(void)
{
	if (test_zone_is_reused() || test_eviction_is_bounded()
		|| test_map_tracks_live_zones() || test_cache_does_not_leak())
		return (1);
	printf("large_cache: OK\n");
	return (0);
}