/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ftm_heap.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/02 15:38:11 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/04 15:34:57 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ftm_internal.h"
#include "ftm_port.h"

static t_heap	g_heap;

t_heap	*ftm_heap_instance(void)
{
	return (&g_heap);
}

static t_zone	*heap_push_zone(t_zone_kind kind, size_t payload_size)
{
	t_zone	*zone;
	
	zone = NULL;
	if (kind == FTM_LARGE)
		zone = ftm_large_cache_take(payload_size);
	if (zone == NULL)
	{
		zone = ftm_zone_create(kind, payload_size);
		if (zone == NULL)
			return (NULL);
		g_heap.map_calls++;
	}
	zone->prev = NULL;
	zone->next = g_heap.zones[kind];
	if (g_heap.zones[kind] != NULL)
		g_heap.zones[kind]->prev = zone;
	g_heap.zones[kind] = zone;
	g_heap.zone_count[kind]++;
	ftm_zone_map_insert(zone);
	return (zone);
}

static void	heap_init_if_needed(void)
{
	if (g_heap.is_initialized)
		return ;
	g_heap.is_initialized = true;
	ftm_debug_load();
	heap_push_zone(FTM_TINY, 0);
	heap_push_zone(FTM_SMALL, 0);
}

static t_block	*heap_reserve_block(t_zone_kind kind, size_t payload_size)
{
	t_zone	*zone;
	t_block	*block;

	zone = g_heap.zones[kind];
	while (zone != NULL && kind != FTM_LARGE)
	{
		block = ftm_zone_find_free(zone, payload_size);
		if (block != NULL)
		{
			ftm_free_list_unlink(zone, block);
			return (ftm_block_split(zone, block, payload_size));
		}
		zone = zone->next;
	}
	zone = heap_push_zone(kind, payload_size);
	if (zone == NULL)
		return (NULL);
	block = ftm_zone_find_free(zone, payload_size);
	if (block == NULL)
		return (NULL);
	ftm_free_list_unlink(zone, block);
	return (ftm_block_split(zone, block, payload_size));
}

void	*ftm_alloc(size_t size)
{
	t_zone_kind	kind;
	size_t		payload_size;
	size_t		requested;
	t_block		*block;

	heap_init_if_needed();
	requested = size;
	if (size == 0)
		size = 1;
	if (ftm_debug()->guard)
		size += FTM_ALIGNMENT;
	payload_size = ftm_round_up_to_alignment(size);
	if (payload_size == 0)
		return (NULL);
	kind = ftm_size_class(payload_size);
	block = heap_reserve_block(kind, payload_size);
	if (block == NULL)
		return (NULL);
	block->request_size = requested;
	ftm_block_mark_used(block);
	ftm_on_alloc(block);
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
		g_heap.zone_count[kind] = 0;
		kind++;
	}
	ftm_large_cache_flush();
	ftm_zone_map_reset();
	g_heap.map_calls = 0;
	g_heap.unmap_calls = 0;
	g_heap.large_cache_hits = 0;
	g_heap.is_initialized = false;
}

t_zone	*ftm_heap_find_zone(void *ptr)
{
	t_zone			*zone;
	unsigned char	*address;
	int				kind;

	if (ftm_zone_map_is_active())
		return (ftm_zone_map_lookup(ptr));
	address = ptr;
	kind = 0;
	while (kind < FTM_ZONE_KIND_COUNT)
	{
		zone = g_heap.zones[kind];
		while (zone != NULL)
		{
			if (address > (unsigned char *)zone
				&& address < (unsigned char *)zone + zone->total_size)
				return (zone);
			zone = zone->next;
		}
		kind++;
	}
	return (NULL);
}

static bool	zone_is_fully_free(t_zone *zone)
{
	t_block	*first;

	first = ftm_zone_first_block(zone);
	return (ftm_block_is_free(first) && first->next == NULL);
}

static void	heap_release_zone_if_free(t_zone *zone)
{
	t_zone	*evicted;

	if (!zone_is_fully_free(zone))
		return ;
	if (zone->kind != FTM_LARGE && g_heap.zone_count[zone->kind] <= 1)
		return ;
	if (zone->prev != NULL)
		zone->prev->next = zone->next;
	else
		g_heap.zones[zone->kind] = zone->next;
	if (zone->next != NULL)
		zone->next->prev = zone->prev;
	g_heap.zone_count[zone->kind]--;
	ftm_zone_map_remove(zone);
	if (zone->kind == FTM_LARGE)
	{
		evicted = ftm_large_cache_put(zone);
		if (evicted == NULL)
			return ;
		zone = evicted;
	}
	ftm_zone_destroy(zone);
	g_heap.unmap_calls++;
}

static void	release_block(t_zone *zone, t_block *block)
{
	ftm_on_free(block);
	ftm_block_mark_free(block);
	ftm_free_list_push(zone, block);
	ftm_block_coalesce_next(zone, block);
	if (block->prev != NULL && ftm_block_is_free(block->prev))
		ftm_block_coalesce_next(zone, block->prev);
	heap_release_zone_if_free(zone);
}

void	ftm_release(void *ptr)
{
	t_zone	*zone;
	void	*base;

	if (ptr == NULL)
		return ;
	zone = ftm_heap_find_zone(ptr);
	if (zone == NULL)
		return ;
	if (ftm_pointer_is_allocated(ptr, zone))
	{
		release_block(zone, ftm_payload_to_block(ptr));
		return ;
	}
	base = ftm_aligned_base(ptr, zone);
	if (base == NULL)
		return ;
	zone = ftm_heap_find_zone(base);
	if (zone != NULL && ftm_pointer_is_allocated(base, zone))
		release_block(zone, ftm_payload_to_block(base));
}