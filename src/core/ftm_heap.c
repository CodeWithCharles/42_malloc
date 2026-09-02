/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ftm_heap.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/02 15:38:11 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/02 15:45:14 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ftm_internal.h"

static t_heap	g_heap;

t_heap	*ftm_heap_instance(void)
{
	return (&g_heap);
}

static t_zone	*heap_push_zone(t_zone_kind kind, size_t payload_size)
{
	t_zone	*zone;
	
	zone = ftm_zone_create(kind, payload_size);
	if (zone == NULL)
		return (NULL);
	zone->next = g_heap.zones[kind];
	if (g_heap.zones[kind] != NULL)
		g_heap.zones[kind]->prev = zone;
	g_heap.zones[kind] = zone;
	g_heap.map_calls++;
	return (zone);
}

static void	heap_init_if_needed(void)
{
	if (g_heap.is_initialized)
		return ;
	g_heap.is_initialized = true;
	heap_push_zone(FTM_TINY, 0);
	heap_push_zone(FTM_SMALL, 0);
}

static t_block	*heap_reserve_block(t_zone_kind kind, size_t payload_size)
{
	t_zone	*zone;
	t_block	*block;

	zone = g_heap.zones[kind];
	while (zone != NULL)
	{
		block =	ftm_zone_find_free(zone, payload_size);
		if (block != NULL)
			return (ftm_block_split(block, payload_size));
		zone = zone->next;
	}
	zone = heap_push_zone(kind, payload_size);
	if (zone == NULL)
		return (NULL);
	block = ftm_zone_find_free(zone, payload_size);
	if (block == NULL)
		return (NULL);
	return (ftm_block_split(block, payload_size));
}

void	*ftm_alloc(size_t size)
{
	t_zone_kind	kind;
	size_t		payload_size;
	t_block		*block;

	heap_init_if_needed();
	if (size == 0)
		size = 1;
	payload_size = ftm_round_up_to_alignment(size);
	if (payload_size == 0)
		return (NULL);
	kind = ftm_size_class(payload_size);
	block = heap_reserve_block(kind, payload_size);
	if (block == NULL)
		return (NULL);
	ftm_block_mark_used(block);
	return (ftm_block_payload(block));
}

void	ftm_heap_reset(void)
{
	t_zone	*zone;
	t_zone	*next;
	int		kind;

	kind = 0;
	while (kind < FTM_ZONE_KIND_COUNT)
	{
		zone = g_heap.zones[kind];
		while (zone != NULL)
		{
			next = zone->next;
			ftm_zone_destroy(zone);
			zone = next;
		}
		g_heap.zones[kind] = NULL;
		kind++;
	}
	g_heap.map_calls = 0;
	g_heap.unmap_calls = 0;
	g_heap.is_initialized = false;
}