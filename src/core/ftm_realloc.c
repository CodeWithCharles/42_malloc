/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ftm_realloc.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cpoulain <cpoulain@student.42lehavre.fr    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/02 17:11:53 by cpoulain          #+#    #+#             */
/*   Updated: 2026/09/04 17:51:28 by cpoulain         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ftm_internal.h"
#include "ftm_port.h"

static void	shrink_in_place(t_zone *zone, t_block *block, size_t new_payload)
{
	ftm_block_split(zone, block, new_payload);
	if (block->next != NULL)
		ftm_block_coalesce_next(zone, block->next);
}

static bool	try_absorb_next(t_zone *zone, t_block *block, size_t new_payload)
{
	t_block	*next;

	next = block->next;
	if (next == NULL || !ftm_block_is_free(next))
		return (false);
	if (block->payload_size + FTM_BLOCK_HEADER_SIZE
		+ next->payload_size < new_payload)
		return (false);
	ftm_free_list_unlink(zone, next);
	block->payload_size += FTM_BLOCK_HEADER_SIZE + next->payload_size;
	block->next = next->next;
	if (next->next != NULL)
		next->next->prev = block;
	ftm_block_split(zone, block, new_payload);
	return (true);
}

static void	*resize_by_moving(void *ptr, t_block *block, size_t size)
{
	void	*fresh;
	size_t	to_copy;

	fresh = ftm_alloc(size);
	if (fresh == NULL)
		return (NULL);
	to_copy = block->payload_size;
	if (size < to_copy)
		to_copy = size;
	ftm_memcpy(fresh, ptr, to_copy);
	ftm_release(ptr);
	return (fresh);
}

static void	*resize_aligned(void *ptr, size_t size)
{
	void	*fresh;
	size_t	old;
	size_t	to_copy;

	old = ftm_usable_size(ptr);
	fresh = ftm_alloc(size);
	if (fresh == NULL)
		return (NULL);
	to_copy = old;
	if (size < to_copy)
		to_copy = size;
	ftm_memcpy(fresh, ptr, to_copy);
	ftm_release(ptr);
	return (fresh);
}

void	*ftm_resize(void *ptr, size_t size)
{
	t_zone	*zone;
	t_block	*block;
	size_t	new_payload;

	if (ptr == NULL)
		return (ftm_alloc(size));
	if (size == 0)
	{
		ftm_release(ptr);
		return (ftm_alloc(0));
	}
	zone = ftm_heap_find_zone(ptr);
	if (zone == NULL)
		return (NULL);
	if (!ftm_pointer_is_allocated(ptr, zone))
	{
		if (ftm_aligned_base(ptr, zone) == NULL)
			return (NULL);
		return (resize_aligned(ptr, size));
	}
	new_payload = ftm_round_up_to_alignment(size);
	if (new_payload == 0)
		return (NULL);
	block = ftm_payload_to_block(ptr);
	if (ftm_size_class(new_payload) == zone->kind)
	{
		if (new_payload <= block->payload_size)
		{
			shrink_in_place(zone, block, new_payload);
			ftm_block_set_request(block, size);
			return (ptr);
		}
		if (try_absorb_next(zone, block, new_payload))
		{
			ftm_block_set_request(block, size);
			return (ptr);
		}
	}
	return (resize_by_moving(ptr, block, size));
}