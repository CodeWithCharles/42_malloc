/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ftm_zone.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/02 15:05:11 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/04 15:32:39 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ftm_internal.h"
#include "ftm_port.h"

size_t  ftm_zone_total_size(t_zone_kind kind, size_t payload_size)
{
	size_t  page_size;

	page_size = ftm_page_size();
	if (kind == FTM_TINY)
		return (FTM_ZONE_SIZE(FTM_TINY_MAX, page_size,
			FTM_BLOCK_HEADER_SIZE, FTM_ZONE_HEADER_SIZE));
	if (kind == FTM_SMALL)
		return (FTM_ZONE_SIZE(FTM_SMALL_MAX, page_size,
			FTM_BLOCK_HEADER_SIZE, FTM_ZONE_HEADER_SIZE));
	return (FTM_ALIGN_UP(FTM_ZONE_HEADER_SIZE + FTM_BLOCK_HEADER_SIZE
		+ payload_size, page_size));
}

t_block *ftm_zone_first_block(t_zone *zone)
{
	return ((t_block *)((unsigned char *)zone + FTM_ZONE_HEADER_SIZE));
}

t_zone  *ftm_zone_create(t_zone_kind kind, size_t payload_size)
{
	t_zone  *zone;
	t_block *block;
	size_t  total_size;

	total_size = ftm_zone_total_size(kind, payload_size);
	zone = ftm_map_pages(total_size);
	if (zone == NULL)
		return (NULL);
	zone->next = NULL;
	zone->prev = NULL;
	zone->total_size = total_size;
	zone->kind = kind;
	zone->free_list = NULL;
	block = ftm_zone_first_block(zone);
	block->payload_size = total_size - FTM_ZONE_HEADER_SIZE
		- FTM_BLOCK_HEADER_SIZE;
	block->next = NULL;
	block->prev = NULL;
	block->flags = FTM_BLOCK_MAGIC;
	ftm_block_mark_free(block);
	ftm_free_list_push(zone, block);
	return (zone);
}

void    ftm_zone_destroy(t_zone *zone)
{
	ftm_unmap_pages(zone, zone->total_size);
}

t_block *ftm_zone_find_free(t_zone *zone, size_t payload_size)
{
	return (ftm_free_list_find(zone, payload_size));
}