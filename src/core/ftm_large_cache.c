/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ftm_large_cache.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/04 11:29:48 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/04 11:36:52 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ftm_internal.h"
#include "ftm_port.h"

static void	cache_unlink(t_zone *zone)
{
	t_heap	*heap;

	heap = ftm_heap_instance();
	if (zone->prev != NULL)
		zone->prev->next = zone->next;
	else
		heap->large_cache = zone->next;
	if (zone->next != NULL)
		zone->next->prev = zone->prev;
	heap->large_cache_count--;
}

static t_zone	*cache_pop_oldest(void)
{
	t_zone	*oldest;

	oldest = ftm_heap_instance()->large_cache;
	if (oldest == NULL)
		return (NULL);
	while (oldest->next != NULL)
		oldest = oldest->next;
	cache_unlink(oldest);
	return (oldest);
}

static size_t	fit_limit(size_t needed)
{
	if (needed > SIZE_MAX / FTM_LARGE_CACHE_FIT_FACTOR)
		return (SIZE_MAX);
	return (needed * FTM_LARGE_CACHE_FIT_FACTOR);
}

t_zone	*ftm_large_cache_take(size_t payload_size)
{
	t_heap	*heap;
	t_zone	*zone;
	size_t	needed;
	size_t	limit;

	heap = ftm_heap_instance();
	needed = ftm_zone_total_size(FTM_LARGE, payload_size);
	limit = fit_limit(needed);
	zone = heap->large_cache;
	while (zone != NULL)
	{
		if (zone->total_size >= needed && zone->total_size <= limit)
		{
			cache_unlink(zone);
			heap->large_cache_hits++;
			return (zone);
		}
		zone = zone->next;
	}
	return (NULL);
}

t_zone	*ftm_large_cache_put(t_zone *zone)
{
	t_heap	*heap;
	t_zone	*evicted;

	heap = ftm_heap_instance();
	evicted = NULL;
	if (heap->large_cache_count >= FTM_LARGE_CACHE_MAX_ZONES)
		evicted = cache_pop_oldest();
	zone->prev = NULL;
	zone->next = heap->large_cache;
	if (heap->large_cache != NULL)
		heap->large_cache->prev = zone;
	heap->large_cache = zone;
	heap->large_cache_count++;
	return (evicted);
}

void	ftm_large_cache_flush(void)
{
	t_heap	*heap;
	t_zone	*zone;
	t_zone	*next;

	heap = ftm_heap_instance();
	zone = heap->large_cache;
	while (zone != NULL)
	{
		next = zone->next;
		ftm_zone_destroy(zone);
		zone = next;
	}
	heap->large_cache = NULL;
	heap->large_cache_count = 0;
}