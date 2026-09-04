/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ftm_zone_map.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/04 12:10:38 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/04 16:20:46 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ftm_internal.h"
#include "ftm_port.h"

static uintptr_t	page_of(uintptr_t address)
{
	size_t	page_size;

	page_size = ftm_page_size();
	return (address - address % page_size);
}

static size_t	slot_of(uintptr_t page)
{
	uintptr_t	frame;

	frame = page / ftm_page_size();
	return ((size_t)(frame * 2654435761u) & (FTM_ZONE_MAP_CAPACITY - 1));
}

static bool	map_insert_page(t_zone *zone, uintptr_t page)
{
	t_heap	*heap;
	size_t	slot;
	size_t	probe;
	size_t	tombstone;

	heap = ftm_heap_instance();
	slot = slot_of(page);
	tombstone = FTM_ZONE_MAP_CAPACITY;
	probe = 0;
	while (probe < FTM_ZONE_MAP_CAPACITY)
	{
		if (heap->zone_map[slot].page == 0)
			break ;
		if (heap->zone_map[slot].page == page)
		{
			heap->zone_map[slot].zone = zone;
			return (true);
		}
		if (heap->zone_map[slot].page == FTM_ZONE_MAP_TOMBSTONE
			&& tombstone == FTM_ZONE_MAP_CAPACITY)
			tombstone = slot;
		slot = (slot + 1) & (FTM_ZONE_MAP_CAPACITY - 1);
		probe++;
	}
	if (tombstone != FTM_ZONE_MAP_CAPACITY)
	{
		slot = tombstone;
		heap->zone_map_tombstones--;
	}
	else if (probe == FTM_ZONE_MAP_CAPACITY)
		return (false);
	heap->zone_map_live++;
	heap->zone_map[slot].page = page;
	heap->zone_map[slot].zone = zone;
	return (true);
}

static void	map_remove_page(uintptr_t page)
{
	t_heap	*heap;
	size_t	slot;
	size_t	probe;

	heap = ftm_heap_instance();
	slot = slot_of(page);
	probe = 0;
	while (probe < FTM_ZONE_MAP_CAPACITY)
	{
		if (heap->zone_map[slot].page == 0)
			return ;
		if (heap->zone_map[slot].page == page)
		{
			heap->zone_map[slot].page = FTM_ZONE_MAP_TOMBSTONE;
			heap->zone_map[slot].zone = NULL;
			heap->zone_map_live--;
			heap->zone_map_tombstones++;
			return ;
		}
		slot = (slot + 1) & (FTM_ZONE_MAP_CAPACITY - 1);
		probe++;
	}
}

static void	map_clear(void)
{
	t_heap	*heap;
	size_t	index;

	heap = ftm_heap_instance();
	index = 0;
	while (index < FTM_ZONE_MAP_CAPACITY)
	{
		heap->zone_map[index].page = 0;
		heap->zone_map[index].zone = NULL;
		index++;
	}
	heap->zone_map_live = 0;
	heap->zone_map_tombstones = 0;
}

static bool	map_insert_zone(t_zone *zone)
{
	size_t	page_size;
	size_t	pages;
	size_t	index;

	page_size = ftm_page_size();
	pages = zone->total_size / page_size;
	index = 0;
	while (index < pages)
	{
		if (!map_insert_page(zone, (uintptr_t)zone + index * page_size))
			return (false);
		index++;
	}
	return (true);
}

static void	map_rebuild(void)
{
	t_heap	*heap;
	t_zone	*zone;
	int		kind;

	heap = ftm_heap_instance();
	map_clear();
	kind = 0;
	while (kind < FTM_ZONE_KIND_COUNT)
	{
		zone = heap->zones[kind];
		while (zone != NULL)
		{
			map_insert_zone(zone);
			zone = zone->next;
		}
		kind++;
	}
}

void	ftm_zone_map_insert(t_zone *zone)
{
	t_heap	*heap;
	size_t	pages;

	heap = ftm_heap_instance();
	if (heap->zone_map_disabled)
		return ;
	pages = zone->total_size / ftm_page_size();
	if (heap->zone_map_live + heap->zone_map_tombstones + pages
		> FTM_ZONE_MAP_MAX_LIVE)
		map_rebuild();
	if (heap->zone_map_live + pages > FTM_ZONE_MAP_MAX_LIVE)
	{
		heap->zone_map_disabled = true;
		return ;
	}
	if (!map_insert_zone(zone))
		heap->zone_map_disabled = true;
}

void	ftm_zone_map_remove(t_zone *zone)
{
	size_t	page_size;
	size_t	pages;
	size_t	index;

	page_size = ftm_page_size();
	pages = zone->total_size / page_size;
	index = 0;
	while (index < pages)
	{
		map_remove_page((uintptr_t)zone + index * page_size);
		index++;
	}
}

t_zone	*ftm_zone_map_lookup(void *ptr)
{
	t_heap		*heap;
	uintptr_t	page;
	size_t		slot;
	size_t		probe;

	heap = ftm_heap_instance();
	page = page_of((uintptr_t)ptr);
	slot = slot_of(page);
	probe = 0;
	while (probe < FTM_ZONE_MAP_CAPACITY)
	{
		if (heap->zone_map[slot].page == 0)
			return (NULL);
		if (heap->zone_map[slot].page == page)
			return (heap->zone_map[slot].zone);
		slot = (slot + 1) & (FTM_ZONE_MAP_CAPACITY - 1);
		probe++;
	}
	return (NULL);
}

bool	ftm_zone_map_is_active(void)
{
	return (!ftm_heap_instance()->zone_map_disabled);
}

void	ftm_zone_map_reset(void)
{
	map_clear();
	ftm_heap_instance()->zone_map_disabled = false;
}