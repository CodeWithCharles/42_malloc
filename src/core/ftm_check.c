/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ftm_check.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/02 16:06:28 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/04 15:48:27 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ftm_internal.h"

#ifdef FTM_DEBUG

static bool	check_free_list(t_zone *zone, size_t expected)
{
	t_block			*block;
	t_block			*previous;
	unsigned char	*limit;
	size_t			seen;

	block = zone->free_list;
	previous = NULL;
	limit = (unsigned char *)zone + zone->total_size;
	seen = 0;
	while (block != NULL && seen <= expected)
	{
		if (!ftm_block_is_free(block))
			return (false);
		if ((unsigned char *)block < (unsigned char *)zone
			|| (unsigned char *)block >= limit)
			return (false);
		if (ftm_free_list_prev(block) != previous)
			return (false);
		previous = block;
		block = ftm_free_list_next(block);
		seen++;
	}
	return (block == NULL && seen == expected);
}

static bool	check_zone(t_zone *zone)
{
	t_block			*block;
	t_block			*previous;
	unsigned char	*cursor;
	size_t			free_blocks;

	block = ftm_zone_first_block(zone);
	cursor = (unsigned char *)block;
	previous = NULL;
	free_blocks = 0;
	while (block != NULL)
	{
		if ((uintptr_t)ftm_block_payload(block) % FTM_ALIGNMENT != 0)
			return (false);
		if (block->prev != previous || (unsigned char *)block != cursor)
			return (false);
		if (ftm_block_is_free(block))
		{
			if (previous != NULL && ftm_block_is_free(previous))
				return (false);
			free_blocks++;
		}
		cursor = ftm_block_end(block);
		previous = block;
		block = block->next;
	}
	if (cursor != (unsigned char *)zone + zone->total_size)
		return (false);
	return (check_free_list(zone, free_blocks));
}

bool	ftm_check_heap(void)
{
	t_heap	*heap;
	t_zone	*zone;
	int		kind;

	heap = ftm_heap_instance();
	kind = 0;
	while (kind < FTM_ZONE_KIND_COUNT)
	{
		zone = heap->zones[kind];
		while (zone != NULL)
		{
			if (!check_zone(zone))
				return (false);
			zone = zone->next;
		}
		kind++;
	}
	return (true);
}

#endif