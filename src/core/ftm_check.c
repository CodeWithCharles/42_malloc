/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ftm_check.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/02 16:06:28 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/02 16:12:01 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ftm_internal.h"

#ifdef true

static bool	check_zone(t_zone *zone)
{
	t_block			*block;
	t_block			*previous;
	unsigned char	*cursor;
	bool			previous_free;

	block = ftm_zone_first_block(zone);
	cursor = (unsigned char *)block;
	previous = NULL;
	previous_free = false;
	while (block != NULL)
	{
		if ((uintptr_t)ftm_block_payload(block) % FTM_ALIGNMENT != 0)
			return (false);
		if (block->prev != previous)
			return (false);
		if ((unsigned char *)block != cursor)
			return (false);
		if (ftm_block_is_free(block) && previous_free)
			return (false);
		cursor = ftm_block_end(block);
		previous = block;
		previous_free = ftm_block_is_free(block);
		block = block->next;
	}
	return (cursor == (unsigned char *)zone + zone->total_size);
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