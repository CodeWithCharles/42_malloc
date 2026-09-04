/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test_zone.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/02 15:11:56 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/04 15:37:27 by cpoulain         ###   ########.fr       */
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

static int	test_tiny_zone_geometry(void)
{
	t_zone	*zone;
	t_block	*block;
	size_t	usable;

	zone = ftm_zone_create(FTM_TINY, 0);
	CHECK(zone != NULL);
	CHECK(zone->kind == FTM_TINY);
	CHECK(zone->total_size % ftm_page_size() == 0);
	CHECK(zone->next == NULL && zone->prev == NULL);

	block = ftm_zone_first_block(zone);
	CHECK(ftm_block_is_free(block));
	CHECK(block->next == NULL && block->prev == NULL);
	CHECK((uintptr_t)ftm_block_payload(block) % FTM_ALIGNMENT == 0);

	usable = zone->total_size - FTM_ZONE_HEADER_SIZE;
	CHECK(usable >= FTM_MIN_ALLOCS * (FTM_BLOCK_HEADER_SIZE + FTM_TINY_MAX));
	CHECK(block->payload_size == usable - FTM_BLOCK_HEADER_SIZE);

	ftm_zone_destroy(zone);
	return (0);
}

static int	test_find_free(void)
{
	t_zone	*zone;
	t_block	*block;

	zone = ftm_zone_create(FTM_TINY, 0);
	CHECK(zone != NULL);
	block = ftm_zone_first_block(zone);

	CHECK(ftm_zone_find_free(zone, FTM_TINY_MAX) == block);
	CHECK(ftm_zone_find_free(zone, block->payload_size) == block);
	CHECK(ftm_zone_find_free(zone, block->payload_size + 1) == NULL);

	ftm_block_mark_used(block);
	ftm_free_list_unlink(zone, block);
	CHECK(ftm_zone_find_free(zone, 16) == NULL);

	ftm_zone_destroy(zone);
	return (0);
}

static int	test_large_zone(void)
{
	t_zone	*zone;
	t_block	*block;

	zone = ftm_zone_create(FTM_LARGE, 5000);
	CHECK(zone != NULL);
	CHECK(zone->kind == FTM_LARGE);
	CHECK(zone->total_size % ftm_page_size() == 0);

	block = ftm_zone_first_block(zone);
	CHECK(block->payload_size >= 5000);
	CHECK((uintptr_t)ftm_block_payload(block) % FTM_ALIGNMENT == 0);

	ftm_zone_destroy(zone);
	return (0);
}

int	main(void)
{
	fake_port_reset();
	if (test_tiny_zone_geometry() || test_find_free() || test_large_zone())
		return (1);
	CHECK(fake_port_unmap_count() == 3);
	printf("zone: OK\n");
	return (0);
}